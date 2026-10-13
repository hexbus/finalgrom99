"""Reassemble the Beta 1 launch bridges and check the supplied binaries.

Optional developer check: requires an existing xdt99 xas99.py installation.
Normal companion rebuilds use the supplied bridge.bin and need no assembler.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[1]
SAMPLES = ROOT / 'templates/gigacart/beta1'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--assembler', type=Path, required=True, help='Path to xdt99/xas99.py')
    parser.add_argument('--output', type=Path, required=True, help='New output directory')
    args = parser.parse_args()
    try:
        assembler = args.assembler.resolve(strict=True)
        config = json.loads((SAMPLES / 'bridge-builds.json').read_text())
        args.output.mkdir(parents=True, exist_ok=False)
        results = []
        for name, defines in config['variants'].items():
            source_name = config.get('source_overrides', {}).get(name, config['source'])
            source = ROOT / source_name
            source_hash = hashlib.sha256(source.read_bytes()).hexdigest()
            target = (args.output / (name + '.bin')).resolve()
            command = [sys.executable, str(assembler), *config['flags'], '-o', str(target),
                       str(source), '-D', *[f'{key}={value}' for key, value in defines.items()]]
            subprocess.run(command, check=True)
            actual = target.read_bytes()
            recipe = json.loads((SAMPLES / name / 'recipe.json').read_text())
            digest = hashlib.sha256(actual).hexdigest()
            if actual != (SAMPLES / name / 'bridge.bin').read_bytes() or digest != recipe['bridge_sha256']:
                raise ValueError('Bridge does not match the reference example: ' + name)
            results.append(dict(name=name, source=source_name, source_sha256=source_hash,
                                defines=defines, bytes=len(actual), sha256=digest,
                                byte_identical=True))
            print('PASS ' + name + ': assembled bridge matches the supplied binary')
        report = dict(assembler_sha256=hashlib.sha256(assembler.read_bytes()).hexdigest(),
                      flags=config['flags'], bridges=results, hardware_tested=False)
        (args.output / 'results.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    except (OSError, ValueError, KeyError, subprocess.CalledProcessError) as error:
        parser.exit(2, str(error) + '\n')


if __name__ == '__main__':
    main()
