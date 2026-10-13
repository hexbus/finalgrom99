"""Verify frozen release identity and mapped source bytes without local references."""
import hashlib,json
from pathlib import Path
from source_layout import build_recipe, source_mapping
root=Path(__file__).resolve().parents[1];rel=root/'release/2.0-beta1-125ms'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    provenance=json.loads((rel/'provenance.json').read_text())
    spec=build_recipe(root)
    mapping=source_mapping(root)
    for name,expected in spec['files'].items():
        assert sha(root/mapping[name])==expected, 'Changed verified build input: '+name
    for name,expected in provenance['artifacts'].items():
        assert sha(rel/name)==expected, 'Changed release artifact: '+name
    for name,expected in provenance['cpld_sources'].items():
        assert sha(root/name)==expected, 'Changed CPLD input: '+name
    print(f'PASS: {len(spec["files"])} mapped AVR inputs, {len(provenance["cpld_sources"])} CPLD files and matched AVR/CPLD/menu artifacts')
if __name__=='__main__':main()
