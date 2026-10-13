"""Compile exact-byte patch recipes to a read-only FGOV0001 sector sidecar.

No image/card writes. The recipe producer must establish overwrite safety and
adapter coverage; this compiler establishes identity, byte and range checks.
"""
import argparse
import binascii
import hashlib
import json
from pathlib import Path
import struct

def seal(data):
    data=bytearray(data)
    assert len(data)==512
    struct.pack_into('<I',data,508,binascii.crc32(data[:508]))
    return bytes(data)

def compile_overlay(image,patches):
    assert image and len(image)%8192==0 and len(image)<=134217728
    banks=len(image)//8192;changed={};covered=set()
    for patch in patches:
        bank,offset=patch['bank'],patch['offset']
        old,new=bytes.fromhex(patch['expected']),bytes.fromhex(patch['replacement'])
        assert 0<=bank<banks and 0<=offset<8192 and 0<len(new)==len(old)<=8192-offset
        base=bank*8192+offset
        assert image[base:base+len(old)]==old, f'Preimage mismatch: bank {bank}, offset {offset:x}'
        for n,value in enumerate(new):
            absolute=base+n
            assert absolute not in covered, f'Overlapping patches at {absolute:x}'
            covered.add(absolute)
            sector,index=divmod(absolute,512)
            if sector not in changed:changed[sector]=bytearray(image[sector*512:(sector+1)*512])
            changed[sector][index]=value
    tables=[bytearray(512) for _ in range((banks+30)//31)]
    for i,page in enumerate(tables):struct.pack_into('<I',page,0,i+1)
    payload=[];first=1+len(tables)
    for bank in range(banks):
        sectors=[s for s in range(16) if bank*16+s in changed]
        mask=sum(1<<s for s in sectors)
        struct.pack_into('<HHIII',tables[bank//31],4+16*(bank%31),bank,mask,
                         first+len(payload) if mask else 0,0,0)
        for sector in sectors:
            absolute=bank*16+sector;data=bytes(changed[absolute])
            record=bytearray(512);record[:8]=b'FGOP0001'
            struct.pack_into('<HBBII',record,8,bank,sector,0,
                binascii.crc32(image[absolute*512:(absolute+1)*512]),binascii.crc32(data))
            payload.extend([seal(record),data])
    header=bytearray(512);header[:8]=b'FGOV0001'
    struct.pack_into('<IIH',header,8,len(image),first+len(payload),banks)
    header[32:64]=hashlib.sha256(image).digest()
    return b''.join([seal(header),*[seal(t) for t in tables],*payload])

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('image',type=Path);p.add_argument('recipe',type=Path);p.add_argument('output',type=Path)
    a=p.parse_args()
    assert a.output.resolve() not in (a.image.resolve(),a.recipe.resolve())
    image=a.image.read_bytes();recipe=json.loads(a.recipe.read_text())
    assert hashlib.sha256(image).hexdigest()==recipe['image_sha256']
    blob=compile_overlay(image,recipe['patches'])
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(blob)
    print(json.dumps(dict(bytes=len(blob),sha256=hashlib.sha256(blob).hexdigest(),
        original_image_unchanged=True,coverage_verified=False),indent=2))

if __name__=='__main__':main()
