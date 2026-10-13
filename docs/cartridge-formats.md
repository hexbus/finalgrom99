# Cartridge companion binary formats

Developer reference for the formats used by FinalGROM 99 2.0 Beta 1. Start with [Preparing and copying a Gigacart](cartridge-preparation.md) for installation, conversion and editable templates, and the [build guide](building.md) for firmware build commands.

## Implemented binary contract: draft for review

Fields below are little-endian. Pages are 512 bytes; a ROM bank is 8,192 bytes. CRC32 is the reflected IEEE variant used by Python `zlib.crc32`. Identity fields refer to the prepared image, including intentional padding/bootstrap additions, not necessarily the supplied BIN. Exact-size, bounds, identity and byte-preimage checks are part of the producer/loader contract; CRCs do not authenticate an untrusted producer.

### GLP startup plan, policy 4

The compiler is `tools/build-cooperative-profile.py`; the readers are `avr/gigacart/giga-launch.c` and `avr/gigacart/giga-fixed.c`. Historical policies share the signature but are not interchangeable with policy 4.

| Header byte | Field |
| --- | --- |
| 0..7 | ASCII `FGLA0001` |
| 8..11 / 12..15 | Image bytes / total file pages, uint32 |
| 16..17 | Job count, uint16 |
| 18 / 19 / 20 / 21 | Policy 4 / first reusable slot / slot count / GROM configuration |
| 22..23 / 24..27 | Bridge byte count / directory CRC32 |
| 32..63 | Image SHA-256 |
| 64..67 | Fixed-catalog descriptor page |
| 508..511 | CRC32 of header bytes 0..507 |

The directory starts at page 1, with 32 jobs per page. Each 16-byte job holds one-byte operation/slot/sector/count, uint32 source, uint32 payload CRC, then CRC32 of the first 12 job bytes. Operations are 0 = read image sectors, 1 = copy profile payload, 2 = fill byte, 3 = verify image sectors without copying. The last job installs the one-sector launch bridge in slot 0. The directory CRC covers entire padded directory pages.

Policy 4's descriptor begins `FGDM0001`; byte 8 counts fixed ranges, bytes 10..11 hold image bank count. From byte 16, each five-byte range is uint16 first bank, uint16 count, uint8 first physical slot. Bytes 508..511 seal the descriptor. Compiler checks exclude overlapping fixed/reusable ranges and reserved slots. The producer builds the map payload for slots 126/127 and clears slot 125; these are not extra reusable cache space. The executing dynamic bank remains reserved until the adapted program commits its next bank; predictions do not override that reservation.

### GOV sector overlay

The compiler is `tools/build-sector-overlay.py`; `avr/gigacart/giga-overlay.c` implements consumption. The header begins `FGOV0001`, with image bytes at 8, file pages at 12, uint16 bank count at 16 and image SHA-256 at 32..63. A CRC at 508 covers preceding bytes.

Directory page `1 + bank / 31` contains its uint32 page number, then 31 sixteen-byte bank records: uint16 bank, uint16 changed-sector mask, uint32 first replacement-record page, and eight reserved zero bytes. Each directory page has its CRC at 508. Set bits enumerate sector replacements in ascending order.

Each replacement uses two pages. The first begins `FGOP0001`, followed by uint16 bank at 8, sector at 10, zero at 11, original-sector CRC at 12 and replacement-sector CRC at 16; its page CRC is at 508. The next page is the complete replacement sector. Runtime checks the old bytes' CRC before applying the replacement. Fixed startup payloads must agree with the same overlay.

### GPR predictions

Compiler: `tools/build-giga-predict.py`. Reader: `avr/gigacart/giga-predict.c`.

The binary format is little-endian:

| Region | Layout |
| --- | --- |
| 64-byte header | `FGPR0001`; 16-bit record/word counts at 8/10; body CRC32 at 12; image SHA-256 at 16..47; zero reserved tail |
| 1,024-byte directory | 256 pairs of 16-bit start/end record indexes; end exclusive; one bucket per 64 banks |
| Records, from byte 1,088 | Eight bytes: first bank, last bank, pool word offset, one-byte successor count, zero reserved byte |
| Successor pool | 16-bit logical bank numbers |
| Padding | Zero through the next 512-byte boundary, excluded from body CRC |

Opening validates identity, size, CRC and padding before launch. The reader checks record and pool bounds as consumed. Model identity is compared with the startup profile; the package verifier checks the complete original ROM hash. Runtime checks do not replace that installation check.

Missing predictions cause waits; they do not authorize reading an unavailable bank. The reader keeps at most eight distinct successor suggestions per lookup. Coverage and scheduling performance need separate validation.

### GIR/GIS/BIX location indexes

FinalGROM generates these from the destination card's allocation. They are not portable converter output. The `FGGIDX01` header describes the filesystem and allocation; records are pairs of little-endian uint32 starting cluster and consecutive-cluster count. The complete read/write and validation implementation is in `avr/gigacart/giga.c`. This document does not define a separate desktop GIR producer. The [preparation guide](cartridge-preparation.md) explains what to copy and when to recreate a location index.
