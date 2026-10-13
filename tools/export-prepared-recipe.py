"""Recover reproducible companion recipes from a verified prepared set.

This preserves an existing adaptation. It does not discover a safe adaptation
in an original ROM. The input image and its companion files remain unchanged.
"""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import struct


def module(name):
    spec = importlib.util.spec_from_file_location(name, Path(__file__).with_name(name + '.py'))
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


def export(image_path, output):
    image_path, output = Path(image_path).resolve(), Path(output).resolve()
    verification = module('verify-prepared-set').verify(image_path)
    if output.exists():
        raise ValueError('Use a new output directory')
    image = image_path.read_bytes()
    glp = image_path.with_suffix('.GLP').read_bytes()
    gov = image_path.with_suffix('.GOV').read_bytes()
    descriptor = struct.unpack_from('<I', glp, 64)[0] * 512
    ranges = []
    for i in range(glp[descriptor + 8]):
        bank, count, first = struct.unpack_from('<HHB', glp, descriptor + 16 + i * 5)
        ranges.append(dict(bank=bank, count=count, first=first))
    last_job = 512 + (struct.unpack_from('<H', glp, 16)[0] - 1) * 16
    bridge_page = struct.unpack_from('<I', glp, last_job + 4)[0]
    bridge_length = struct.unpack_from('<H', glp, 22)[0]
    bridge = glp[bridge_page * 512:bridge_page * 512 + bridge_length]
    patches = []
    for bank in range(len(image) // 8192):
        at = (1 + bank // 31) * 512 + 4 + bank % 31 * 16
        _, mask, page = struct.unpack_from('<HHI', gov, at)
        for sector in range(16):
            if mask & (1 << sector):
                start = bank * 8192 + sector * 512
                patches.append(dict(bank=bank, offset=sector * 512,
                                    expected=image[start:start + 512].hex(),
                                    replacement=gov[(page + 1) * 512:(page + 2) * 512].hex()))
                page += 2
    try:
        image_reference = Path(os.path.relpath(image_path, output)).as_posix()
    except ValueError:
        image_reference = image_path.as_posix()
    recipe = dict(name=image_path.stem, image=image_reference,
                  image_sha256=hashlib.sha256(image).hexdigest(), bridge='bridge.bin',
                  bridge_sha256=hashlib.sha256(bridge).hexdigest(), patches=patches,
                  fixed_ranges=ranges, dynamic_first=glp[19], dynamic_count=glp[20])
    compiled_glp, compiled_gov = module('build-cooperative-profile').compile_profile(
        image, patches, ranges, glp[19], glp[20], bridge)
    if (compiled_glp, compiled_gov) != (glp, gov):
        raise ValueError('Prepared set does not reproduce with this compiler')
    prediction_path = image_path.with_suffix('.GPR')
    model = None
    if prediction_path.exists():
        data = prediction_path.read_bytes()
        n = struct.unpack_from('<H', data, 8)[0]
        pool = 1088 + n * 8
        intervals = []
        for i in range(n):
            first, last, start, count, _ = struct.unpack_from('<HHHBB', data, 1088 + i * 8)
            successors = list(struct.unpack_from('<' + 'H' * count, data, pool + start * 2))
            intervals.append(dict(first=first, last=last, successors=successors))
        model = dict(image_sha256=hashlib.sha256(image).hexdigest(), intervals=intervals)
        if module('build-giga-predict').compile_model(model) != data:
            raise ValueError('Prediction does not reproduce with this compiler')
    output.mkdir(parents=True)
    (output / 'recipe.json').write_text(json.dumps(recipe, indent=2) + '\n', encoding='utf-8')
    (output / 'bridge.bin').write_bytes(bridge)
    if model is not None:
        (output / 'prediction.json').write_text(json.dumps(model, indent=2) + '\n', encoding='utf-8')
    expected = {}
    for ext in ('.GLP', '.GOV') + (('.GPR',) if model is not None else ()):
        p = image_path.with_suffix(ext)
        expected[p.name] = hashlib.sha256(p.read_bytes()).hexdigest()
    report = dict(image=image_path.name, byte_identical_recompile=True, companions=expected,
                  verification=verification,
                  limitation='Rebuilds companions for the already-prepared image; not an original-ROM adapter or new gameplay validation.')
    (output / 'rebuild.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    return report


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('image', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(export(args.image, args.output), indent=2))
