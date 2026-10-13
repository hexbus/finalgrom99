"""Build maintained AVR/TI sources and compare with the frozen release. Never flash."""
import argparse, datetime, hashlib, json, os, pathlib, shutil, subprocess, sys
from source_layout import build_recipe, source_mapping
ROOT=pathlib.Path(__file__).resolve().parents[1]
REL=ROOT/'release/2.0-beta1-125ms'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=pathlib.Path)
    ap.add_argument('--verify-release',action='store_true')
    ap.add_argument('--toolchain-bin',type=pathlib.Path)
    ap.add_argument('--xdt99',type=pathlib.Path,help='Also reassemble and verify the menu')
    a=ap.parse_args()
    out=(a.output or ROOT/'output'/('build-'+datetime.datetime.now().strftime('%Y%m%d-%H%M%S-%f'))).resolve()
    if out.exists():raise SystemExit('Choose a new output directory; existing work is never removed.')
    spec=build_recipe(ROOT);mapping=source_mapping(ROOT)
    canonical=spec['files']
    out.mkdir(parents=True);env=os.environ.copy()
    if a.toolchain_bin:env['PATH']=str(a.toolchain_bin.resolve())+os.pathsep+env['PATH']
    changed=[]
    for name,digest in spec['files'].items():
        source=(ROOT/mapping[name]).resolve(); assert source.is_relative_to(ROOT)
        if sha(source)!=canonical[name]:changed.append(mapping[name])
        target=out/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,target)
    if a.verify_release and changed:raise SystemExit('Verified build inputs changed: '+', '.join(changed))
    logs=[]
    def run(cmd):
        result=subprocess.run([str(v) for v in cmd],cwd=out,env=env,capture_output=True,text=True)
        logs.append('$ '+' '.join(map(str,cmd))+'\n'+result.stdout+result.stderr)
        (out/'build.log').write_text('\n'.join(logs),encoding='utf8');result.check_returncode()
        return result.stdout
    version=run(['avr-gcc','--version']).splitlines()[0]
    for cmd in spec['commands']:
        assert cmd[0]=='avr-gcc'
        if '-c' not in cmd:assert '-Wl,--defsym=__TEXT_REGION_LENGTH__=28672' in cmd
        run(cmd)
    run(['avr-objcopy','-O','binary','-R','.eeprom','candidate.elf','candidate.bin'])
    actual=sha(out/'candidate.bin');equal=actual==spec['avr_sha256'];size=(out/'candidate.bin').stat().st_size
    assert size<=28672, 'Application overflow'
    menu=None
    if a.xdt99:
        menuout=out/'menu-check'
        run([sys.executable,ROOT/'tools/rebuild-menu-snapshot.py','--source',out,'--xdt99',a.xdt99.resolve(),'--output',menuout])
        menu=json.loads((menuout/'results.json').read_text())
    result={'compiled':True,'compiler':version,'program_bytes':size,'sha256':actual,'byte_identical_to_release':equal,'changed_inputs':changed,'build_recipe':'tools/firmware-build.json','staged_inputs':len(spec['files']),'compilation_units':sum('-c' in c for c in spec['commands']),'menu_rebuild':menu,'hardware_programmed':False,'cpld_rebuilt':False}
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2));print('Build directory:',out)
    if a.verify_release and not equal:raise SystemExit('Binary differs from release; check compiler/toolchain.')
if __name__=='__main__':main()
