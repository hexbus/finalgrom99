# Cartridge file formats

[Start here](../README.md) · [Adapter workflow](../development/adapters.md) · [Architecture](../development/architecture.md)

This reference is for people writing adapters or tools. For what to copy to SD, use the [player file table](../prepared-cartridges.md#which-files-go-on-the-card). Source paths below are relative to the repository root.

## JSON inputs

| Input | Tool | Output |
| --- | --- | --- |
| `recipe.json` | `build-cooperative-profile.py` | GLP, GOV and a manifest |
| `prediction.json` | `build-giga-predict.py` | Optional GPR |
| `catalog.json` | `convert-cartridge.py` | Image and referenced companions in an `sd-card/` folder |

The [templates](../../templates/gigacart/README.md) provide editable structures. They require reviewed cartridge-specific values. There is no GIR/GIS/BIX template: those indexes describe the destination card and are created by FinalGROM.

### Recipe and prediction fields

The recipe's `image` path is relative to the recipe file. It refers to the exact **prepared image before runtime GOV patches**, including any required padding or bootstrap edits. `image_sha256` binds that image; `bridge` and `bridge_sha256` identify a reviewed TI launch routine. Generic templates omit the bridge; Beta 1 examples include it.

`fixed_ranges` maps logical ROM banks to physical SRAM slots: `bank` is the first logical bank, `count` its length, and `first` the first resident slot. `dynamic_first` and `dynamic_count` select the reusable pool. The compiler checks overlaps and reserved slots, but only cartridge analysis can establish which banks must remain resident.

Each patch has a logical `bank`, a byte `offset` within its 8 KiB bank, and equal-length hexadecimal `expected` and `replacement` strings. These must be reviewed program changes, not guessed values. The builder checks original bytes and produces both startup payloads and runtime overlays consistently. The recipe's patches do not rewrite the image file on disk.

The prediction model uses the same prepared-image SHA-256. Each interval's `first` and `last` are inclusive logical bank numbers; `successors` lists likely next banks in priority order. The loader uses at most eight distinct suggestions per lookup. These predictions do not replace the safe request-and-wait changes in the cartridge program.

### Converter catalog fields

The catalog's `input_sha256` identifies the original user input, and `output_sha256` identifies the prepared image. `image_edits` contains any reviewed preparation edits made to the image itself; it is separate from runtime GOV patches. It may remain empty only when those image bytes are unchanged.

Each image edit is an object with an absolute byte `offset` and Base64 `before` and `after` strings. Edits apply in order; each preimage must match the image at that step. These are Base64 strings, unlike the recipe's hexadecimal patch strings. Preserve original-to-prepared edits separately: exporting companions does not recover them.

`companions` records paths relative to the catalog directory and the SHA-256 of each generated file. GLP and GOV are required; remove the GPR entry if the adapter has no model. `name` is the shared uppercase alphanumeric stem, at most eight characters; `extension` is `.GCR` or `.GCS`. Set `requirements` to the matched firmware/hardware and `evidence` to actual verification and hardware results, including unresolved issues. A catalog label is not itself qualification.

Ship the completed catalog with its referenced companion files, not the unfinished template. The built-in catalog supplies the seven exact beta-image entries. Additional original-ROM revisions need their own preserved input hashes, reviewed edits and verification evidence before inclusion.

## Binary companions

**Status: implemented Beta 1 contract, draft reference for review.** The source readers and writers remain important when checking behavior not covered here.


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

FinalGROM generates these from the destination card's allocation. They are not portable converter output. The `FGGIDX01` header describes the filesystem and allocation; records are pairs of little-endian uint32 starting cluster and consecutive-cluster count. The complete read/write and validation implementation is in `avr/gigacart/giga.c`. This document does not define a separate desktop GIR producer. The [player file guide](../prepared-cartridges.md#which-files-go-on-the-card) explains what to copy and when to recreate a location index.
