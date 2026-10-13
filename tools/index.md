# Tools index

These programs run on the development computer. Start with [cartridge preparation](../docs/cartridge-preparation.md) to prepare an SD folder, the [build guide](../docs/building.md) to build firmware, or the [recipe examples](../docs/beta-cartridge-recipes.md) to develop an adapter.

There are **four unchanged upstream utilities and 17 added tools or support files**. Origin follows the [TI index's upstream reference and definitions](../ti/index.md#how-origin-is-classified); it does not establish sole authorship or hardware compatibility.

## Original utilities

These unchanged utilities retain Python 2 syntax; the added preparation tools use Python 3.

| File | Purpose |
| --- | --- |
| [invert.py](invert.py) | Reverse the order of 8 KiB banks and write an `_i` output file. |
| [mkea5cart.py](mkea5cart.py) | Wrap Editor/Assembler option 5 files in a cartridge loader. |
| [pad.py](pad.py) | Append padding to the input file up to an 8 KiB boundary. Modifies that file. |
| [showcart.py](showcart.py) | Inspect ROM/GROM headers and display menu entries. |

## Added cartridge preparation tools

These tools require a reviewed adaptation; they cannot adapt an arbitrary ROM automatically. See [templates](../templates/gigacart/README.md) for editable inputs and [cartridge formats](../docs/cartridge-formats.md) for binary layouts.

| File | Input and result |
| --- | --- |
| [convert-cartridge.py](convert-cartridge.py) | Match an exact image against the adapter catalog and export its prepared SD set. |
| [verify-prepared-set.py](verify-prepared-set.py) | Check consistency of an image and its GLP/GOV/optional GPR companions. Read-only. |
| [build-cooperative-profile.py](build-cooperative-profile.py) | Compile a checked recipe and startup bridge into GLP/GOV companions. |
| [build-sector-overlay.py](build-sector-overlay.py) | Compile exact-byte patch records into a GOV sector overlay. |
| [build-giga-predict.py](build-giga-predict.py) | Compile an image-bound prediction model into GPR. |
| [learn-cartridge-prediction.py](learn-cartridge-prediction.py) | Learn optional successors from bank traces and score separate validation traces. |
| [export-prepared-recipe.py](export-prepared-recipe.py) | Recover a reproducible recipe from an already-prepared, verified set. |
| [rebuild-beta-bridges.py](rebuild-beta-bridges.py) | Assemble the seven supplied TI bridges and verify their bytes and recipe hashes. |
| [package-beta-cartridges.py](package-beta-cartridges.py) | Build local ZIP packages from the recorded prepared images and companions. Requires the images separately. |
| [test-cartridge-build-tools.py](test-cartridge-build-tools.py) | Exercise preparation with synthetic data and check the supplied catalog. Its synthetic image is not executable TI software. |

## Added firmware build and verification tools

Use the [build guide](../docs/building.md) for prerequisites and commands. The snapshot helpers support the main builder; most developers should start with `build-firmware.py`.

| File | Role |
| --- | --- |
| [build-firmware.py](build-firmware.py) | Stage maintained sources under the release recipe's filenames, build the AVR, and optionally verify the release and reassemble the menu. |
| [rebuild-avr-snapshot.py](rebuild-avr-snapshot.py) | Build an exported AVR source snapshot in a new directory. |
| [rebuild-menu-snapshot.py](rebuild-menu-snapshot.py) | Reassemble the exported TI menu and check its bytes and expected offsets. |
| [verify-baseline.py](verify-baseline.py) | Check frozen release artifacts and mapped source hashes without compiling. |
| [source_layout.py](source_layout.py) | Shared path-resolution module used by the builder and baseline verifier. |
| [source-layout.json](source-layout.json) | Map retained relocated inputs to their maintained paths. |
| [firmware-build.json](firmware-build.json) | Maintained input hashes and compiler/link commands, targeting exact Beta 1 bytes without archived sources. See the [build guide](../docs/building.md#verify-the-baseline). |

Generated files belong under ignored `output/`; update this index when adding a tool.
