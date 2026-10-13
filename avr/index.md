# AVR source index

This is the microcontroller firmware. Read the [firmware guide](../docs/firmware-guide.md) for the runtime and the [build guide](../docs/building.md) for compilation. Origin follows the [TI index's upstream reference and definitions](../ti/index.md#how-origin-is-classified). Retained or compiled files are not necessarily active; see [release notes](../docs/release-notes.md) for supported behavior.

## Original files at the directory root

| Files | Origin | Role |
| --- | --- | --- |
| [finalgrom.c](finalgrom.c) | Modified | Main loop and cartridge selection dispatch. |
| [loader.c](loader.c), [loader.h](loader.h) | Modified | SD browsing, menu construction and cartridge loading. Includes the session helpers below. |
| [wire.c](wire.c) / [wire.h](wire.h) | C modified; header unchanged | Low-level cartridge signals and transfers. |
| [menu.c](menu.c), [grom.c](grom.c), [help.c](help.c) | Modified | Embedded TI program byte arrays. Assembly sources and current roles are in the [TI index](../ti/index.md). |
| [Makefile](Makefile) | Modified | AVR make rules; exact release reproduction uses the maintained builder. |
| [finalgrom.hex](finalgrom.hex) | Unchanged | Preserved upstream firmware binary, not the 2.0 release artifact. |

## Added files at the directory root

These are shared helpers and required support for reproducing the released build.

| Files | Role |
| --- | --- |
| [crc32.c](crc32.c), [crc32.h](crc32.h) | Shared checksum implementation for companion readers. |
| [sd-receive.inc](sd-receive.inc) | Buffered SPI payload receive included by the SD driver. |
| [offline-sector.inc](offline-sector.inc) | Sector writer used while the cartridge is offline. |
| [compilation-session.inc](compilation-session.inc) | Required build support. Retains the wrapper needed for exact Beta 1 bytes and includes the shared sector writer. |

The [compilation-session retention decision](../docs/preservation.md#compilation-session-retention-decision) explains why build-significant support remains even when its wrapper is absent from the final machine code.

## Large-cartridge source directories

| Directory | Origin and contents |
| --- | --- |
| [gigacart/](gigacart/index.md) | Added cooperative loader and its shared interfaces. The index covers all 25 source/header files. |

The [maintained recipe](../tools/firmware-build.json) selects build inputs and commands; [source-layout.json](../tools/source-layout.json) resolves relocated inputs. The frozen release manifest remains historical evidence.

## Archived additions

Earlier controllers, the added diagnostics and unused root-level inputs now live in a separate reference repository. See the [archive record](../docs/preservation.md#reference-repository) for recovery and evidence. They are not needed to build the maintained firmware. Historical diagnostic flags are not supported configurations of this source set.

## Original supporting directories

These supporting directories predate 2.0 and remain in the maintained tree.

| Directory | Files and changes from upstream |
| --- | --- |
| [boot/](boot/) | [bootload.c](boot/bootload.c) and [Makefile](boot/Makefile) are unchanged. Its private [lib/](boot/lib/) copy also remains unchanged. |
| [lib/](lib/) | Petit FatFs and SD support. [diskio.c](lib/diskio.c) and [pff.c](lib/pff.c) are modified; the headers and [00readme.txt](lib/00readme.txt) are unchanged. |
| [jtag/](jtag/) | CPLD programming support. [micro.c](jtag/micro.c) and [ports.c](jtag/ports.c) are modified; [lenval.c](jtag/lenval.c) and all three headers are unchanged. |
| [test/](test/) | Unchanged host harness: [Makefile](test/Makefile), [sim.c](test/sim.c), [mocktypes.h](test/mocktypes.h), [loader.c](test/loader.c), [menu.c](test/menu.c) and [help.c](test/help.c). Preservation is not a claim of a fresh test run. |

Keep this index current when adding or moving files. Commands belong in the [build guide](../docs/building.md); runtime explanations belong in the [firmware guide](../docs/firmware-guide.md).
