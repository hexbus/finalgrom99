"""Reassemble the SD menu from its exported source; verify the shipped bytes.

Creates a new local build directory. Does not update AVR constants or flash.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import zlib


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source', type=Path, required=True, help='Exported avr-2.0 directory')
    p.add_argument('--xdt99', type=Path, required=True, help='xdt99 directory containing xas99.py')
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    source, xdt99, output = a.source.resolve(), a.xdt99.resolve(), a.output.resolve()
    expected = (source / 'FGMENU.BIN').read_bytes()
    output.mkdir(parents=True, exist_ok=False)
    for file in (source / 'ti').glob('*.a99'):
        shutil.copy2(file, output / file.name)
    command = [sys.executable, str(xdt99 / 'xas99.py'), '-b', '-R', '-I', str(xdt99 / 'lib'),
               '-E', 'menu.equ', '-o', 'FGMENU.BIN', 'menu.a99']
    result = subprocess.run(command, cwd=output, capture_output=True, text=True)
    (output / 'build.log').write_text(result.stdout + result.stderr, encoding='utf-8')
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    actual = (output / 'FGMENU.BIN').read_bytes()
    if actual != expected:
        raise ValueError('Menu differs from the shipped copy; do not combine changed menu bytes with the frozen AVR')
    symbols = {n.lower(): int(v, 16) for n, v in re.findall(r'^(\w+):\s+equ\s+>([0-9a-fA-F]+)',
                (output / 'menu.equ').read_text(), re.M)}
    report = dict(passed=True, byte_identical=True, bytes=len(actual), sha256=hashlib.sha256(actual).hexdigest(),
                  crc32=f'{zlib.crc32(actual):08x}', status_offset=symbols['giga_status'] - 0x6000,
                  detail_offset=symbols['giga_detail'] - 0x6000,
                  assembler_sha256=hashlib.sha256((xdt99 / 'xas99.py').read_bytes()).hexdigest(),
                  hardware_programmed=False)
    (output / 'results.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
