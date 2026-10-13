"""Rebuild an exported AVR source snapshot into a new directory; never flash."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    source, output = a.source.resolve(), a.output.resolve()
    spec = json.loads((source / 'snapshot.json').read_text())
    assert not output.exists(), 'Use a new build directory'
    for name, digest in spec['files'].items():
        path = (source / name).resolve()
        assert path.is_relative_to(source) and sha(path) == digest, name
    output.mkdir(parents=True)
    for name in spec['files']:
        target = output / name
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source / name, target)
    log = []
    for command in spec['commands']:
        assert command[0] == 'avr-gcc'
        if '-c' not in command:
            assert '-Wl,--defsym=__TEXT_REGION_LENGTH__=28672' in command
        run = subprocess.run(command, cwd=output, capture_output=True, text=True)
        log.append('$ ' + ' '.join(command) + '\n' + run.stdout + run.stderr)
        (output / 'build.log').write_text('\n'.join(log))
        assert run.returncode == 0, run.stderr
    subprocess.run(['avr-objcopy', '-O', 'binary', '-R', '.eeprom', 'candidate.elf', 'candidate.bin'],
                   cwd=output, check=True)
    actual = sha(output / 'candidate.bin')
    assert actual == spec['avr_sha256'], 'Rebuild differs: check the pinned AVR toolchain'
    assert (output / 'candidate.bin').stat().st_size <= 28672
    report = dict(passed=True, byte_identical=True, sha256=actual, hardware_programmed=False)
    (output / 'results.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
