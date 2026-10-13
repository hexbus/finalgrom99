"""Resolve the self-contained maintained build without consulting an archive."""
import hashlib
import json
from pathlib import Path


def build_recipe(root: Path) -> dict:
    root = root.resolve()
    recipe = json.loads((root / 'tools/firmware-build.json').read_text())
    release = root / 'release/2.0-beta1-125ms'
    for filename, field in [('avr-snapshot.json', 'frozen_snapshot_sha256'),
                            ('provenance.json', 'frozen_provenance_sha256')]:
        if hashlib.sha256((release / filename).read_bytes()).hexdigest() != recipe[field]:
            raise ValueError(f'Frozen release metadata changed: {filename}')
    frozen = json.loads((release / 'avr-snapshot.json').read_text())
    if recipe['schema_version'] != 1 or recipe['avr_sha256'] != frozen['avr_sha256']:
        raise ValueError('Unsupported recipe or changed Beta 1 target')
    renames = recipe.get('input_renames', {})
    if (renames.keys() - frozen['files'].keys()
            or set(renames.values()) - recipe['files'].keys()
            or len(set(renames.values())) != len(renames)
            or renames.keys() & recipe['files'].keys()
            or any(Path(name).name != name or '\\' in name or name in ('.', '..')
                   for name in renames.values())):
        raise ValueError('Invalid maintained input renames')
    origins = {new: old for old, new in renames.items()}
    if {origins.get(name, name) for name in recipe['files']} - frozen['files'].keys():
        raise ValueError('Unknown staged source names in maintained recipe')
    objects = set()
    links = []
    for command in recipe['commands']:
        if not command or command[0] != 'avr-gcc':
            raise ValueError('Unexpected build command')
        if '-c' in command:
            source = command[command.index('-c') + 1]
            if source not in recipe['files']:
                raise ValueError(f'Compilation source is not staged: {source}')
            objects.add(command[command.index('-o') + 1])
        else:
            links.append(command)
    if len(links) != 1 or objects != {v for v in links[0] if v.endswith('.o')}:
        raise ValueError('Link inputs differ from compiled objects')
    if '-Wl,--defsym=__TEXT_REGION_LENGTH__=28672' not in links[0]:
        raise ValueError('Missing application size limit')
    return recipe


def source_mapping(root: Path) -> dict[str, str]:
    root = root.resolve()
    release = root / 'release/2.0-beta1-125ms'
    mapping = json.loads((release / 'source-map.json').read_text())
    relocations = json.loads((root / 'tools/source-layout.json').read_text())
    unknown = relocations.keys() - mapping.keys()
    if unknown:
        raise ValueError(f'Unknown release source names: {sorted(unknown)}')
    mapping.update(relocations)
    recipe = build_recipe(root)
    origins = {new: old for old, new in recipe.get('input_renames', {}).items()}
    mapping = {name: mapping[origins.get(name, name)] for name in recipe['files']}
    for name, relative in mapping.items():
        source = (root / relative).resolve()
        if not source.is_relative_to(root) or not source.is_file():
            raise ValueError(f'Missing or out-of-tree source: {name}: {relative}')
    return mapping
