"""Identify a cartridge and export its qualified adaptation to a new SD folder.

Standard library only. Never modifies input, firmware, or the SD card.
Unknown binaries need a reviewed adapter; filename hints cannot prove safety.
"""
import argparse,base64,hashlib,json,re,sys,zipfile
from pathlib import Path

def sha(data):return hashlib.sha256(data).hexdigest()

def read_image(path):
    if path.suffix.lower()!='.rpk':return path.read_bytes()
    with zipfile.ZipFile(path) as z:
        files=[i for i in z.infolist() if i.filename.lower().endswith('.bin')]
        if len(files)!=1:raise ValueError('RPK must contain exactly one ROM binary; provide the intended binary explicitly')
        if files[0].file_size>134217728:raise ValueError('RPK ROM exceeds 128 MiB')
        return z.read(files[0])

def member(root,name):
    p=(root/name).resolve()
    if not p.is_relative_to(root.resolve()):raise ValueError('Catalog member escapes its directory')
    return p

def identify(raw,name,catalog):
    digest=sha(raw)
    hint=re.search(r'([89])(?:\.bin)?$',name.lower())
    headers=[]
    for bank in range((len(raw)+8191)//8192):
        h=raw[bank*8192:bank*8192+16]
        if len(h)==16 and h[0]==0xaa:headers.append(bank)
    return dict(bytes=len(raw),banks_8k=(len(raw)+8191)//8192,sha256=digest,
        filename_banking_hint=hint[1] if hint else None,header_banks=headers,
        matches=[e['id'] for e in catalog['adapters'] if e['input_sha256']==digest],
        interpretation='Exact identity selects an audited adapter. Size, header and filename are hints only.')

def convert(raw,entry,root):
    if not re.fullmatch(r'[A-Z0-9]{1,8}',entry['name']) or entry['extension'] not in ('.GCR','.GCS'):
        raise ValueError('Invalid output cartridge filename')
    if not {'.GLP','.GOV'}<=set(entry['companions'])<= {'.GLP','.GOV','.GPR'}:
        raise ValueError('Invalid companion extension set')
    if sha(raw)!=entry['input_sha256']:raise ValueError('Selected adapter does not match this exact input revision')
    image=bytearray(raw)
    for patch in entry['image_edits']:
        at=patch['offset'];old=base64.b64decode(patch['before'],validate=True);new=base64.b64decode(patch['after'],validate=True)
        if at<0 or at>len(image) or image[at:at+len(old)]!=old:raise ValueError('Image patch preimage mismatch')
        image[at:at+len(old)]=new
    if sha(image)!=entry['output_sha256']:raise ValueError('Adapted image identity mismatch')
    files={entry['name']+entry['extension']:bytes(image)}
    for ext,desc in entry['companions'].items():
        data=member(root,desc['path']).read_bytes()
        if sha(data)!=desc['sha256']:raise ValueError('Companion identity mismatch: '+ext)
        files[entry['name']+ext]=data
    glp=files[entry['name']+'.GLP']
    if glp[:8]!=b'FGLA0001' or glp[32:64]!=hashlib.sha256(image).digest():raise ValueError('Startup profile is not bound to this output image')
    return files

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('image',type=Path);p.add_argument('--catalog',type=Path,default=Path(__file__).resolve().parents[1]/'templates/gigacart/beta1/catalog.json',
                   help='Adapter catalog; defaults to the seven supplied Beta 1 examples')
    p.add_argument('--adapter',help='Explicit variant override; identity checks still apply')
    p.add_argument('--output',type=Path,help='New local directory to create; copy its sd-card contents yourself')
    p.add_argument('--inspect',action='store_true',help='Report identity and choices without writing')
    a=p.parse_args()
    try:
        raw=read_image(a.image);catalog=json.loads(a.catalog.read_text());info=identify(raw,a.image.name,catalog)
        if a.inspect:print(json.dumps(info,indent=2));return
        matches=[e for e in catalog['adapters'] if e['id'] in info['matches']]
        if a.adapter:
            matches=[e for e in catalog['adapters'] if e['id']==a.adapter]
            if not matches:raise ValueError('Unknown adapter ID')
        else:
            defaults=[e for e in matches if e.get('default',False)]
            if defaults:matches=defaults
        if len(matches)!=1:
            raise ValueError('No qualified adapter for this binary' if not matches else 'Choose --adapter from: '+', '.join(e['id'] for e in matches))
        if a.output is None:raise ValueError('Specify --output or --inspect')
        entry=matches[0];files=convert(raw,entry,a.catalog.parent)
        # Validate everything before creating the output. Never overwrite a package.
        a.output.mkdir(parents=True,exist_ok=False);sd=a.output/'sd-card';sd.mkdir()
        for name,data in files.items():(sd/name).write_bytes(data)
        report=dict(adapter=entry['id'],input=info,requirements=entry['requirements'],evidence=entry['evidence'],
            input_scope=entry.get('input_scope', 'See adapter requirements and evidence.'),
            files={n:dict(bytes=len(d),sha256=sha(d)) for n,d in files.items()},
            instructions='Copy sd-card contents together to card root. Use the matched Beta 1 firmware/CPLD and FGMENU.BIN. Let FinalGROM create its own GIR/GIS location index. Do not copy this manifest to the card.')
        (a.output/'manifest.json').write_text(json.dumps(report,indent=2)+'\n')
        print(json.dumps(report,indent=2))
    except (ValueError,OSError,KeyError,zipfile.BadZipFile) as e:
        p.exit(2,str(e)+'\n')
if __name__=='__main__':main()
