"""Package the recorded prepared Beta 1 sets into new local ZIP files.

Inputs are read-only. Reference companions live beside the example recipes.
Requires the exact prepared images, not original ROMs. Does not publish or flash.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import re
from urllib.parse import quote, urlsplit
import zipfile


ROOT = Path(__file__).resolve().parents[1]
BASELINE = ROOT / 'release/2.0-beta1-125ms'
SAMPLES = ROOT / 'templates/gigacart/beta1'
VERSION = '2.0-beta1-125ms-prepared-r5'


def sha(data):
    return hashlib.sha256(data).hexdigest()


def encoded(value):
    return (json.dumps(value, indent=2) + '\n').encode('utf-8')


def require(condition, message):
    if not condition:
        raise ValueError(message)


def checked(path, expected):
    data = path.read_bytes()
    require(sha(data) == expected, 'Identity mismatch: ' + str(path))
    return data


def load_tool(name):
    spec = importlib.util.spec_from_file_location(name, ROOT / 'tools' / (name + '.py'))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def package_guides():
    """Keep guide links local; link source references to the public repository."""
    docs = ROOT / 'docs'
    guides = {}
    for path in sorted(docs.rglob('*.md')):
        def link(match):
            target = match.group(1)
            parsed = urlsplit(target)
            if parsed.scheme or parsed.netloc or not parsed.path:
                return match.group(0)
            resolved = (path.parent / parsed.path).resolve()
            require(resolved.is_relative_to(ROOT) and resolved.exists(),
                    'Missing guide reference: ' + str(path) + ': ' + target)
            if resolved.is_relative_to(docs):
                return match.group(0)
            kind = 'tree' if resolved.is_dir() else 'blob'
            relative = quote(resolved.relative_to(ROOT).as_posix())
            suffix = ('#' + parsed.fragment) if parsed.fragment else ''
            return f'](https://github.com/hexbus/finalgrom99/{kind}/beta1/{relative}{suffix})'
        text = re.sub(r'\]\(([^)]+)\)', link, path.read_text(encoding='utf-8'))
        guides['guides/' + path.relative_to(docs).as_posix()] = text.encode('utf-8')
    require('guides/README.md' in guides, 'Missing guide entry point')
    return guides


def package(images, output):
    require(not output.exists(), 'Use a new output directory')
    audit_path = BASELINE / 'evidence/imported/tool-audit/current-six-companion-rebuilds.json'
    audit = json.loads(audit_path.read_text())
    additional_path = SAMPLES / 'additional-companion-rebuilds.json'
    additional = json.loads(additional_path.read_text())
    records = audit + additional
    names = [Path(record['image']).stem for record in records]
    require(len(names) == len(set(names)), 'Duplicate cartridge in audits')
    provenance = json.loads((BASELINE / 'provenance.json').read_text())
    menu = checked(BASELINE / 'FGMENU.BIN', provenance['artifacts']['FGMENU.BIN'])
    guides = package_guides()
    # Check all inputs before creating any packages, including fresh recipe rebuilds.
    payloads = []
    for record in records:
        name = Path(record['image']).stem
        sample = SAMPLES / name
        recipe = json.loads((sample / 'recipe.json').read_text())
        identity = record['verification']['image_sha256']
        require(recipe['name'] == name and recipe['image_sha256'] == identity,
                'Recipe does not match recorded image: ' + name)
        image = checked(images / record['image'], identity)
        require(len(image) == record['verification']['image_bytes'], 'Image size: ' + name)
        bridge = checked(sample / 'bridge.bin', recipe['bridge_sha256'])
        glp, gov = load_tool('build-cooperative-profile').compile_profile(
            image, recipe['patches'], recipe['fixed_ranges'],
            recipe['dynamic_first'], recipe['dynamic_count'], bridge)
        compiled = {name + '.GLP': glp, name + '.GOV': gov}
        model = sample / 'prediction.json'
        if model.exists():
            compiled[name + '.GPR'] = load_tool('build-giga-predict').compile_model(
                json.loads(model.read_text()))
        require(set(compiled) == set(record['companions']), 'Companion set: ' + name)
        files = {'sd-card/FGMENU.BIN': menu, 'sd-card/' + record['image']: image}
        for filename, digest in record['companions'].items():
            data = checked(sample / 'final' / filename, digest)
            require(compiled[filename] == data, 'Recipe rebuild differs: ' + filename)
            files['sd-card/' + filename] = data
        files.update(guides)
        instructions = (
            f'FinalGROM {VERSION} - {name}\n\n'
            'This is a repack of the existing prepared beta set, not a new game revision.\n'
            'Use the matched 2.0 Beta 1, 125 ms AVR/CPLD firmware already installed.\n'
            'Firmware updates are not included and no update is performed by this ZIP.\n'
            'Unzip on your computer. Copy the CONTENTS of sd-card to your FAT32 card root,\n'
            'including the supplied FGMENU.BIN. Keep the matching image and companions together.\n'
            f'Select {record["image"]} in FinalGROM. It creates its own card-location index.\n'
            'No GIR/GIS/BIX files are included; do not copy those from another card.\n'
            'Keep manifest.json, CHECKSUMS.sha256, this file and guides on your computer.\n\n'
            'Start with guides/README.md, then guides/getting-started.md to install and play.\n'
            'See guides/prepared-cartridges.md for the files and guides/troubleshooting.md\n'
            'for problems. Developer instructions are under guides/development/.\n'
            'Guide-to-guide links work locally; source and template links open GitHub.\n'
            'Existing beta limitations remain; these packages have no new hardware qualification.\n'
            'This local archive has not been published.\n'
        )
        files['README.txt'] = instructions.encode('utf-8')
        manifest = dict(version=VERSION, cartridge=name, image=record['image'],
                        image_sha256=identity, image_bytes=len(image),
                        required_firmware=provenance['artifacts'],
                        source=record.get('source', 'private-testers R2; frozen six-set companion audit'),
                        reference_example='templates/gigacart/beta1/' + name,
                        recipe_rebuild_byte_identical=True, hardware_tested=False,
                        files={n: dict(bytes=len(d), sha256=sha(d)) for n, d in sorted(files.items())})
        files['manifest.json'] = encoded(manifest)
        files['CHECKSUMS.sha256'] = ''.join(
            f'{sha(d)}  {n}\n' for n, d in sorted(files.items())).encode('ascii')
        payloads.append((name, files, manifest))
    output.mkdir(parents=True, exist_ok=False)
    packages = []
    for name, files, manifest in payloads:
        filename = f'finalgrom-{VERSION}-{name}.zip'
        target = output / filename
        with zipfile.ZipFile(target, 'x', compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
            for member, data in sorted(files.items()):
                info = zipfile.ZipInfo(member, date_time=(2026, 10, 13, 0, 0, 0))
                info.compress_type = zipfile.ZIP_DEFLATED
                info.create_system = 3
                info.external_attr = 0o100644 << 16
                archive.writestr(info, data, compress_type=zipfile.ZIP_DEFLATED, compresslevel=9)
        # Independently reopen every member and compare the bytes actually stored.
        with zipfile.ZipFile(target) as archive:
            require(set(archive.namelist()) == set(files), 'ZIP members differ: ' + name)
            for member, data in files.items():
                require(archive.read(member) == data, 'ZIP payload differs: ' + member)
        packages.append(dict(cartridge=name, filename=filename, bytes=target.stat().st_size,
                             sha256=sha(target.read_bytes()), image=manifest['image'],
                             image_sha256=manifest['image_sha256'],
                             files=manifest['files'], download_url=None))
        print('PASS ' + filename)
    index = dict(version=VERSION, publication_status='local-only; no public download',
                 frozen_audit_sha256=sha(audit_path.read_bytes()),
                 additional_audit_sha256=sha(additional_path.read_bytes()), packages=packages)
    (output / 'packages.json').write_bytes(encoded(index))
    (output / 'CHECKSUMS.sha256').write_text(''.join(
        f'{p["sha256"]}  {p["filename"]}\n' for p in packages), encoding='ascii')
    return index


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--images', type=Path, required=True, help='Directory of exact prepared beta images')
    parser.add_argument('--output', type=Path, required=True, help='New local package directory')
    args = parser.parse_args()
    try:
        package(args.images.resolve(), args.output.resolve())
    except (OSError, ValueError, KeyError, AssertionError, zipfile.BadZipFile) as error:
        parser.exit(2, str(error) + '\n')


if __name__ == '__main__':
    main()
