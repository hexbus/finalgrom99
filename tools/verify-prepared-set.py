"""Read-only consistency check for policy-4 GLP/GOV/GPR cartridge sets.

Checks the complete image identity, patch preimages and startup SRAM contents.
This is not an emulator, timing test, or proof of cartridge adaptation coverage.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zlib


def check(condition, message):
    if not condition:
        raise ValueError(message)


def u16(blob, offset):
    return struct.unpack_from('<H', blob, offset)[0]


def u32(blob, offset):
    return struct.unpack_from('<I', blob, offset)[0]


def sealed(page, label):
    check(len(page) == 512 and zlib.crc32(page[:508]) == u32(page, 508), label + ' CRC')


def verify(image_path):
    image_path = Path(image_path)
    image = image_path.read_bytes()
    size = len(image)
    check(size > 0 and size % 8192 == 0 and size <= 134217728, 'image size')
    banks = size // 8192
    identity = hashlib.sha256(image).digest()
    glp = image_path.with_suffix('.GLP').read_bytes()
    gov = image_path.with_suffix('.GOV').read_bytes()
    for blob, magic in ((glp, b'FGLA0001'), (gov, b'FGOV0001')):
        check(len(blob) >= 512 and len(blob) % 512 == 0, 'sidecar alignment')
        check(blob[:8] == magic, 'sidecar magic')
        sealed(blob[:512], magic.decode())
        check(u32(blob, 8) == size and u32(blob, 12) * 512 == len(blob), 'sidecar length')
        check(blob[32:64] == identity, 'sidecar image identity')

    check(u16(gov, 16) == banks, 'overlay bank count')
    patch = {}
    cursor = 1 + (banks + 30) // 31
    for page_num in range(1, cursor):
        page = gov[page_num * 512:(page_num + 1) * 512]
        sealed(page, 'overlay directory')
        check(u32(page, 0) == page_num, 'overlay directory sequence')
        for item in range(31):
            bank = (page_num - 1) * 31 + item
            if bank >= banks:
                check(page[4 + item * 16:508] == bytes(508 - 4 - item * 16), 'directory padding')
                break
            actual, mask, first, reserved1, reserved2 = struct.unpack_from('<HHIII', page, 4 + item * 16)
            check(actual == bank and reserved1 == reserved2 == 0, 'overlay directory entry')
            check(first == (cursor if mask else 0), 'overlay record ordering')
            for sector in range(16):
                if not mask & (1 << sector):
                    continue
                record = gov[cursor * 512:(cursor + 1) * 512]
                payload = gov[(cursor + 1) * 512:(cursor + 2) * 512]
                sealed(record, 'overlay record')
                check(record[:8] == b'FGOP0001', 'overlay record magic')
                rb, rs, reserved, old_crc, new_crc = struct.unpack_from('<HBBII', record, 8)
                check((rb, rs, reserved) == (bank, sector, 0), 'overlay record address')
                offset = bank * 8192 + sector * 512
                check(zlib.crc32(image[offset:offset + 512]) == old_crc, 'patch preimage')
                check(len(payload) == 512 and zlib.crc32(payload) == new_crc, 'patch payload')
                patch[bank, sector] = payload
                cursor += 2
    check(cursor * 512 == len(gov), 'overlay trailing data')

    count = u16(glp, 16)
    policy, first, dynamic, grom = glp[18:22]
    check(policy == 4 and grom == 0 and 1 <= dynamic <= 80 and 1 <= first < first + dynamic <= 125,
          'policy-4 dynamic pool')
    bridge_length = u16(glp, 22)
    check(2 <= bridge_length <= 512 and count > 0, 'bridge/job count')
    directory_end = (1 + (count + 31) // 32) * 512
    directory = glp[512:directory_end]
    check(zlib.crc32(directory) == u32(glp, 24), 'startup directory CRC')
    check(directory[count * 16:] == bytes(len(directory) - count * 16), 'startup directory padding')
    sram = bytearray(1048576)
    written = set()
    verified = set()
    jobs = []
    for index in range(count):
        record = directory[index * 16:(index + 1) * 16]
        check(zlib.crc32(record[:12]) == u32(record, 12), 'startup record CRC')
        op, slot, sector, sectors, source, crc, _ = struct.unpack('<BBBBIII', record)
        check(op in (0, 1, 2, 3) and slot < 128 and 0 < sectors <= 16 - sector, 'startup bounds')
        length = sectors * 512
        if op in (0, 3):
            data = image[source * 512:source * 512 + length]
        elif op == 1:
            check(source * 512 >= directory_end, 'startup payload overlaps directory')
            data = glp[source * 512:source * 512 + length]
        else:
            check(source < 256, 'fill value')
            data = bytes([source]) * length
        check(len(data) == length and zlib.crc32(data) == crc, 'startup payload CRC')
        if op == 3:
            verified.update(range(source, source + sectors))
        else:
            offset = slot * 8192 + sector * 512
            sram[offset:offset + length] = data
            written.update(range(slot * 16 + sector, slot * 16 + sector + sectors))
        jobs.append((op, slot, sector, sectors))
    check(jobs[-1] == (1, 0, 0, 1) and sram[:2] == b'\x99\x99', 'final bridge job')
    check(sram[bridge_length:512] == bytes(512 - bridge_length), 'bridge padding')
    desc_page = u32(glp, 64)
    check(desc_page * 512 >= directory_end, 'descriptor address')
    desc = glp[desc_page * 512:(desc_page + 1) * 512]
    sealed(desc, 'fixed descriptor')
    check(desc[:8] == b'FGDM0001' and 1 <= desc[8] <= 8 and u16(desc, 10) == banks, 'fixed descriptor')
    expected_map = bytearray(16384)
    occupied = set()
    last = -1
    fixed_banks = 0
    for index in range(desc[8]):
        bank, n, slot = struct.unpack_from('<HHB', desc, 16 + index * 5)
        check(last < bank < bank + n <= banks and 1 <= slot < slot + n <= 125, 'fixed range')
        last = bank + n - 1
        for offset in range(n):
            b, s = bank + offset, slot + offset
            check(s not in occupied and not first <= s < first + dynamic, 'fixed slot conflict')
            occupied.add(s)
            check(set(range(b * 16, b * 16 + 16)) <= verified, 'fixed bank lacks image verification')
            check(set(range(s * 16, s * 16 + 16)) <= written, 'fixed slot incomplete')
            expected = b''.join(patch.get((b, sector), image[b * 8192 + sector * 512:b * 8192 + (sector + 1) * 512]) for sector in range(16))
            check(sram[s * 8192:(s + 1) * 8192] == expected, 'startup does not match patched image')
            expected_map[b] = 128 | s
            fixed_banks += 1
    check(sram[126 * 8192:] == expected_map, 'startup mapping table')
    check(set(range(125 * 16, 128 * 16)) <= written and sram[125 * 8192:126 * 8192] == bytes(8192),
          'empty slot/map initialization')

    prediction = image_path.with_suffix('.GPR')
    records = 0
    if prediction.exists():
        gpr = prediction.read_bytes()
        check(len(gpr) >= 1536 and len(gpr) % 512 == 0 and gpr[:8] == b'FGPR0001', 'prediction header')
        check(gpr[16:48] == identity and gpr[48:64] == bytes(16), 'prediction identity/reserved')
        records, words = struct.unpack_from('<HH', gpr, 8)
        check(records > 0, 'empty prediction')
        pool = 1088 + records * 8
        end = pool + words * 2
        check(end <= len(gpr) and len(gpr) == (end + 511) // 512 * 512, 'prediction size')
        check(zlib.crc32(gpr[64:end]) == u32(gpr, 12) and gpr[end:] == bytes(len(gpr) - end), 'prediction CRC/padding')
        intervals = []
        for i in range(records):
            begin, finish, base, n, reserved = struct.unpack_from('<HHHBB', gpr, 1088 + 8 * i)
            check(0 <= begin <= finish < banks and base + n <= words and reserved == 0, 'prediction record bounds')
            targets = [u16(gpr, pool + 2 * (base + j)) for j in range(n)]
            check(all(b < banks for b in targets) and len(set(targets)) == n, 'prediction successors')
            intervals.append((begin, finish))
        check(intervals == sorted(set(intervals)), 'prediction record ordering')
        for bucket in range(256):
            actual = struct.unpack_from('<HH', gpr, 64 + bucket * 4)
            matches = [i for i, (begin, finish) in enumerate(intervals) if begin < bucket * 64 + 64 and finish >= bucket * 64]
            expected = (min(matches), max(matches) + 1) if matches else (0, 0)
            check(actual == expected, 'prediction bucket coverage')
    return dict(image=image_path.name, image_bytes=size, image_sha256=identity.hex(),
                verified=True, startup_jobs=count, fixed_banks=fixed_banks,
                dynamic_slots=dynamic, patched_sectors=len(patch), prediction_records=records,
                hardware_tested_by_this_check=False)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('image', type=Path)
    args = parser.parse_args()
    print(json.dumps(verify(args.image), indent=2))
