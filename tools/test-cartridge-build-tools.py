"""Exercise the shipped host tools using synthetic data, no game or emulator.

The example is deliberately NOT an executable TI cartridge. Never put it on SD.
Python standard library only; output goes to a new directory chosen by the user.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import zlib

TOOLS = Path(__file__).resolve().parent
TEMPLATES = TOOLS.parent / 'templates' / 'gigacart'


def template(name):
    return json.loads((TEMPLATES / (name + '.template.json')).read_text(encoding='utf-8'))


def load(name):
    spec = importlib.util.spec_from_file_location(name, TOOLS / (name + '.py'))
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


def sha(data):
    return hashlib.sha256(data).hexdigest()


def json_file(path, data):
    path.write_text(json.dumps(data, indent=2) + '\n', encoding='utf-8')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    image = bytes((i * 13 + i // 512) % 256 for i in range(4 * 8192))
    bridge = b'\x99\x99' + bytes(30)
    (out / 'DEMO.GCR').write_bytes(image)
    (out / 'bridge.bin').write_bytes(bridge)
    recipe = template('recipe')
    recipe.update(image_sha256=sha(image), bridge_sha256=sha(bridge))
    recipe['patches'][0].update(expected=image[16:18].hex(), replacement='dead')
    recipe['patches'].append(dict(bank=1, offset=511,
                                 expected=image[8192 + 511:8192 + 513].hex(), replacement='beef'))
    json_file(out / 'recipe.json', recipe)
    commands = []

    def run(script, *arguments, success=True):
        command = [sys.executable, str(TOOLS / script), *map(str, arguments)]
        result = subprocess.run(command, cwd=out, capture_output=True, text=True)
        commands.append(dict(script=script, arguments=list(map(str, arguments)), returncode=result.returncode))
        assert (result.returncode == 0) == success, result.stdout + result.stderr

    run('build-cooperative-profile.py', out / 'recipe.json', '--output', out / 'built')
    shutil.copy2(out / 'DEMO.GCR', out / 'built/DEMO.GCR')
    run('build-sector-overlay.py', out / 'DEMO.GCR', out / 'recipe.json', out / 'standalone.GOV')
    assert (out / 'standalone.GOV').read_bytes() == (out / 'built/DEMO.GOV').read_bytes()
    model = template('prediction')
    model['image_sha256'] = sha(image)
    json_file(out / 'model.json', model)
    run('build-giga-predict.py', out / 'model.json', out / 'built/DEMO.GPR')
    run('verify-prepared-set.py', out / 'built/DEMO.GCR')
    run('export-prepared-recipe.py', out / 'built/DEMO.GCR', '--output', out / 'exported')
    run('build-cooperative-profile.py', out / 'exported/recipe.json', '--output', out / 'rebuilt')
    run('build-giga-predict.py', out / 'exported/prediction.json', out / 'rebuilt/DEMO.GPR')
    for ext in ('.GLP', '.GOV', '.GPR'):
        assert (out / 'built' / ('DEMO' + ext)).read_bytes() == (out / 'rebuilt' / ('DEMO' + ext)).read_bytes()
    for name, banks in [('train', [0, 1, 0, 1, 0, 2, 0]), ('validate', [0, 2, 0, 1])]:
        (out / (name + '.csv')).write_text('seconds,event,bank\n' + ''.join(f'{i},select,{b}\n' for i, b in enumerate(banks)))
    run('learn-cartridge-prediction.py', out / 'DEMO.GCR', '--train', out / 'train.csv',
        '--validate', out / 'validate.csv', '--top', 1, '--output', out / 'learned')
    score = json.loads((out / 'learned/validation.json').read_text())
    assert (score['validation_transitions'], score['successor_matches']) == (3, 2)
    binary = (out / 'learned/MODEL.GPR').read_bytes()
    n, w, crc = struct.unpack_from('<HHI', binary, 8)
    assert (n, w) == (2, 2) and zlib.crc32(binary[64:1088 + n * 8 + w * 2]) == crc
    decoded = []
    for i in range(n):
        begin, end, start, count, reserved = struct.unpack_from('<HHHBB', binary, 1088 + 8 * i)
        targets = struct.unpack_from('<' + 'H' * count, binary, 1088 + n * 8 + start * 2)
        assert reserved == 0
        decoded.append((begin, end, targets))
    assert decoded == [(0, 0, (1,)), (1, 2, (0,))]
    catalog = template('catalog')
    entry = catalog['adapters'][0]
    entry.update(id='demo', input_sha256=sha(image), output_sha256=sha(image),
                 requirements='Synthetic data only', evidence='Not a runnable game')
    for desc in entry['companions'].values():
        desc['sha256'] = sha((out / desc['path']).read_bytes())
    json_file(out / 'catalog.json', catalog)
    run('convert-cartridge.py', out / 'DEMO.GCR', '--catalog', out / 'catalog.json', '--inspect')
    run('convert-cartridge.py', out / 'DEMO.GCR', '--catalog', out / 'catalog.json', '--output', out / 'converted')
    for p in (out / 'converted/sd-card').iterdir():
        assert p.read_bytes() == (out / 'built' / p.name).read_bytes()
    run('convert-cartridge.py', out / 'DEMO.GCR', '--catalog', out / 'catalog.json', '--output', out / 'converted', success=False)
    # The default catalog must cover every shipped example and reference its exact companions.
    samples = TEMPLATES / 'beta1'
    shipped = json.loads((samples / 'catalog.json').read_text())['adapters']
    assert len({e['id'] for e in shipped}) == len(shipped)
    assert {e['name'] for e in shipped} == {p.parent.name for p in samples.glob('*/recipe.json')}
    for entry in shipped:
        reference = json.loads((samples / entry['name'] / 'rebuild.json').read_text())
        assert entry['input_sha256'] == entry['output_sha256'] == reference['verification']['image_sha256']
        assert entry['image_edits'] == []
        assert {entry['name'] + ext for ext in entry['companions']} == set(reference['companions'])
        for ext, desc in entry['companions'].items():
            assert sha((samples / desc['path']).read_bytes()) == desc['sha256'] == reference['companions'][entry['name'] + ext]
    # Run outside the repository: the default catalog must resolve by script location.
    run('convert-cartridge.py', out / 'DEMO.GCR', '--output', out / 'unknown-default', success=False)
    run('convert-cartridge.py', out / 'DEMO.GCR', '--adapter', 'dragon-beta1',
        '--output', out / 'wrong-dragon', success=False)
    assert not (out / 'unknown-default').exists() and not (out / 'wrong-dragon').exists()
    run('learn-cartridge-prediction.py', out / 'DEMO.GCR', '--train', out / 'train.csv',
        '--validate', out / 'train.csv', '--output', out / 'invalid-learning', success=False)
    run('build-cooperative-profile.py', out / 'recipe.json', '--output', out / 'built', success=False)
    run('export-prepared-recipe.py', out / 'built/DEMO.GCR', '--output', out / 'exported', success=False)
    compiler = load('build-cooperative-profile')
    good = dict(image=image, patches=[], ranges=[dict(bank=0, count=1, first=1)], first_slot=2, count=80, bridge=bridge)
    rejected = 0
    for change in ({'image': b'x'}, {'ranges': []}, {'count': 81}, {'first_slot': 100},
                   {'ranges': [dict(bank=0, count=1, first=2)]},
                   {'ranges': [dict(bank=0, count=1, first=125)]},
                   {'bridge': bytes(32)}, {'bridge': bridge + bytes(512)},
                   {'patches': [dict(bank=0, offset=0, expected='ff', replacement='00')]},
                   {'patches': [dict(bank=0, offset=0, expected='00', replacement='01')] * 2}):
        try:
            compiler.compile_profile(**(good | change))
        except (AssertionError, ValueError):
            rejected += 1
        else:
            raise AssertionError('Invalid recipe accepted')
    assert (out / 'DEMO.GCR').read_bytes() == image
    json_file(out / 'results.json', dict(passed=True, independent_decode=True, learned_successors='2/3',
              byte_identical_companion_roundtrip=True, invalid_recipes_rejected=rejected,
              cli_calls=commands, original_input_unchanged=True, game_roms_required=False,
              hardware_tested=False, runnable_ti_cartridge=False, source_templates_exercised=True, shipped_adapters_checked=len(shipped)))
    print('PASS portable compiler, overlay, predictor, learner, converter, recipe export and verifier example')


if __name__ == '__main__':
    main()
