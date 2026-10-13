"""Compile generic policy-4 startup and sector overlays from a checked recipe.

The recipe supplies the cartridge adaptation, fixed reservations and bridge.
This compiler cannot establish that every cartridge bank switch is adapted.
Original images remain read-only; no SD card or firmware is modified.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import zlib

ROOT=Path(__file__).resolve().parents[1]

def compile_profile(image,patches,ranges,first_slot,count,bridge):
    banks=len(image)//8192
    if not image or len(image)%8192 or len(image)>134217728:
        raise ValueError('Image must contain complete 8 KiB banks, at most 128 MiB')
    if not 1<=len(ranges)<=8 or not 1<=count<=80 or not 1<=first_slot<125 or first_slot+count>125:
        raise ValueError('Invalid fixed catalog or dynamic pool')
    if not 2<=len(bridge)<=512 or bridge[:2]!=b'\x99\x99':
        raise ValueError('Expected a FinalGROM bridge, at most one sector')
    resident={};physical=set();last=-1
    for r in ranges:
        b,n,slot=r['bank'],r['count'],r['first']
        if not 0<=b<b+n<=banks or b<=last or not 1<=slot<slot+n<=126:
            raise ValueError('Invalid, overlapping or unsorted fixed range')
        for i in range(n):
            s=slot+i
            if s>=125 or s in physical or first_slot<=s<first_slot+count:
                raise ValueError('Fixed slot overlaps dynamic pool or reserved SRAM')
            resident[b+i]=s;physical.add(s)
        last=b+n-1
    spec=importlib.util.spec_from_file_location('sector_overlay',ROOT/'tools/build-sector-overlay.py')
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    overlay=module.compile_overlay(image,patches)
    patched={b:bytearray(image[b*8192:(b+1)*8192]) for b in resident}
    for p in patches:
        if p['bank'] in patched:
            data=bytes.fromhex(p['replacement']);offset=p['offset']
            patched[p['bank']][offset:offset+len(data)]=data
    jobs=[];payload=[]
    def job(op,slot,sector,n,source,data):
        assert len(data)==n*512
        jobs.append([op,slot,sector,n,source,zlib.crc32(data)])
    def copied(slot,data):
        assert len(data)==8192
        job(1,slot,0,16,len(payload),data)
        payload.extend(data[i:i+512] for i in range(0,8192,512))
    for b,slot in sorted(resident.items(),key=lambda item:item[1]):
        job(3,0,0,16,b*16,image[b*8192:(b+1)*8192]);copied(slot,bytes(patched[b]))
    job(2,125,0,16,0,bytes(8192))
    mapping=bytearray(16384)
    for b,slot in resident.items():mapping[b]=128|slot
    copied(126,bytes(mapping[:8192]));copied(127,bytes(mapping[8192:]))
    descriptor=bytearray(512);descriptor[:8]=b'FGDM0001';descriptor[8]=len(ranges)
    struct.pack_into('<H',descriptor,10,banks)
    for i,r in enumerate(ranges):struct.pack_into('<HHB',descriptor,16+i*5,r['bank'],r['count'],r['first'])
    struct.pack_into('<I',descriptor,508,zlib.crc32(descriptor[:508]))
    config_index=len(payload);payload.append(descriptor)
    job(1,0,0,1,len(payload),bridge.ljust(512,b'\0'));payload.append(bridge.ljust(512,b'\0'))
    first=1+(len(jobs)+31)//32;directory=bytearray((first-1)*512)
    for i,(op,slot,sector,n,source,check) in enumerate(jobs):
        if op==1:source+=first
        struct.pack_into('<BBBBIII',directory,i*16,op,slot,sector,n,source,check,0)
        struct.pack_into('<I',directory,i*16+12,zlib.crc32(directory[i*16:i*16+12]))
    header=bytearray(512);header[:8]=b'FGLA0001'
    struct.pack_into('<IIHBBBBH',header,8,len(image),first+len(payload),len(jobs),4,first_slot,count,0,len(bridge))
    struct.pack_into('<I',header,24,zlib.crc32(directory));header[32:64]=hashlib.sha256(image).digest()
    struct.pack_into('<I',header,64,first+config_index);struct.pack_into('<I',header,508,zlib.crc32(header[:508]))
    return bytes(header+directory+b''.join(payload)),overlay

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('recipe',type=Path);p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();recipe=json.loads(a.recipe.read_text(encoding='utf-8'));base=a.recipe.resolve().parent
    name=recipe['name']
    if not 1<=len(name)<=8 or not name.isascii() or not name.isalnum() or name.upper()!=name:
        raise ValueError('Use an uppercase alphanumeric FAT stem, at most eight characters')
    image=(base/recipe['image']).read_bytes();bridge=(base/recipe['bridge']).read_bytes()
    for data,key in ((image,'image_sha256'),(bridge,'bridge_sha256')):
        if hashlib.sha256(data).hexdigest()!=recipe[key]:raise ValueError('Identity mismatch: '+key)
    glp,gov=compile_profile(image,recipe['patches'],recipe['fixed_ranges'],recipe['dynamic_first'],recipe['dynamic_count'],bridge)
    a.output.mkdir(parents=True,exist_ok=False)
    (a.output/(name+'.GLP')).write_bytes(glp);(a.output/(name+'.GOV')).write_bytes(gov)
    report=dict(name=name,image_sha256=recipe['image_sha256'],startup_bytes=len(glp),overlay_bytes=len(gov),
        startup_sha256=hashlib.sha256(glp).hexdigest(),overlay_sha256=hashlib.sha256(gov).hexdigest(),
        hardware_tested=False,adapter_coverage_proven=False,
        recipe_sha256=hashlib.sha256(a.recipe.read_bytes()).hexdigest())
    (a.output/'manifest.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))

if __name__=='__main__':main()
