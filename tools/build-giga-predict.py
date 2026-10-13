"""Compile an image-bound interval/successor JSON model to FGPR0001.

This predicts requests; it does not adapt an unmodified cartridge to wait.
"""
import argparse
import json
from pathlib import Path
import struct
import zlib


def compile_model(spec):
    sha=bytes.fromhex(spec['image_sha256'])
    if len(sha)!=32:raise ValueError('Expected a SHA-256 image identity')
    clips={}
    for row in spec['intervals']:
        first,last=row['first'],row['last']
        targets=row['successors']
        if not (0<=first<=last<16384) or any(not 0<=n<16384 for n in targets):
            raise ValueError('Bank outside 14-bit range')
        if len(set(targets))!=len(targets) or len(targets)>255:
            raise ValueError('Duplicate or excessive successors')
        key=first,last
        if key in clips:raise ValueError('Combine identical intervals before compiling')
        clips[key]=tuple(targets)
    if not clips:raise ValueError('Empty model')
    pool=[];intern={};records=[]
    for (first,last),targets in sorted(clips.items()):
        if targets not in intern:intern[targets]=len(pool);pool.extend(targets)
        records.append((first,last,intern[targets],len(targets)))
    if len(records)>65535 or len(pool)>65535:raise ValueError('Model exceeds format limits')
    directory=[]
    for bucket in range(256):
        matches=[i for i,r in enumerate(records) if r[0]<bucket*64+64 and r[1]>=bucket*64]
        directory.append((min(matches),max(matches)+1) if matches else (0,0))
    body=b''.join(struct.pack('<HH',*r) for r in directory)
    body+=b''.join(struct.pack('<HHHBB',*r,0) for r in records)
    body+=struct.pack('<'+'H'*len(pool),*pool)
    header=bytearray(64);header[:8]=b'FGPR0001'
    struct.pack_into('<HHI',header,8,len(records),len(pool),zlib.crc32(body))
    header[16:48]=sha
    blob=bytes(header)+body
    return blob.ljust((len(blob)+511)//512*512,b'\0')


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('model',type=Path);p.add_argument('output',type=Path)
    a=p.parse_args();blob=compile_model(json.loads(a.model.read_text()))
    with a.output.open('xb') as f:f.write(blob)
    print(f'Wrote {len(blob)} bytes; original cartridge unchanged.')


if __name__=='__main__':main()
