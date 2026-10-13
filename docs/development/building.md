# Build the firmware

[Start here](../README.md) Â· [How it works](architecture.md) Â· [Cartridge adapters](adapters.md)

This guide builds the AVR application and checks it against the shipped Beta 1 release. You do not need it to install the supplied updates or run cartridges; use [Install and play](../getting-started.md) for that.

## What you need

- A checkout of this repository.
- Python 3.9 or newer, available as `python`.
- AVR GCC **15.2.0** and matching AVR binutils, including `avr-objcopy`, on your PATH. This compiler version is required for the exact-byte comparison.
- The xdt99 assembler suite if you also want to rebuild the TI menu. `--xdt99` takes the directory containing `xas99.py`.

Examples use PowerShell and run from the repository root. Replace `C:/tools/xdt99` with your installation path. Output directories must be new; build tools do not overwrite an earlier run.

## Verify the baseline

Check the supplied source and release hashes, then compile the AVR and reassemble the menu:

```powershell
python tools/verify-baseline.py
python tools/build-firmware.py --verify-release --xdt99 C:/tools/xdt99
```

**Expected result:** the build reports `byte_identical_to_release: true`, a 28,428-byte AVR application, and a successful 2,310-byte menu comparison. Outputs and logs are saved in a new directory under `output/`; the tool prints its location. It does not flash hardware.

The generated AVR image is `candidate.bin`. The installed release files remain in [release/2.0-beta1-125ms](../../release/2.0-beta1-125ms/), with their identities in [provenance.json](../../release/2.0-beta1-125ms/provenance.json).

Omit `--xdt99` to build only the AVR. Use `--toolchain-bin C:/tools/avr/bin` to select a compiler directory explicitly. An AVR-only pass does not also verify a newly assembled menu.

If verification fails, first check the reported changed inputs and compiler version. Keep the frozen release files unchanged; a failed comparison should not be fixed by replacing the expected release binary.

## Build a change

Edit the maintained source files, then omit `--verify-release`:

```powershell
python tools/build-firmware.py --xdt99 C:/tools/xdt99
```

The builder reports modified inputs and still enforces the 28 KiB AVR application limit. A successful compilation does not establish hardware compatibility. If the menu changes, also review its size, checksum and the AVR's status-text offsets.

## How the build finds its sources

[firmware-build.json](../../tools/firmware-build.json) selects 70 staged inputs and 19 compilation units, with hashes and compiler/link commands. [source-layout.json](../../tools/source-layout.json) maps release input names to maintained paths. [source_layout.py](../../tools/source_layout.py) resolves those paths and validates the recipe.

The builder copies inputs into its output directory before compiling. Edit the repository sources, not those temporary copies. Adding compilation units requires updating the maintained recipe deliberately.

The recipe's `input_renames` maps frozen `cinema-session.inc` to maintained `video-session.inc`; the current build flag is `FGVIDEO`. Frozen release records retain their original names and flags. The original 113-input recipe is historical evidence, not the current build procedure. Historical diagnostic flags are unsupported.

### Required build support

Keep [compilation-session.inc](../../avr/compilation-session.inc) and its `prepareCompilation()` wrapper. Removing the wrapper changes the generated AVR bytes and prevents exact Beta 1 reproduction, even though the wrapper itself is absent from the final machine code. This establishes build significance, not runtime execution or a demonstrated behavior difference. The exact compiler decision remains unresolved.

## Check cartridge tools and bridges

The tools have a portable exercise using synthetic data:

```powershell
python tools/test-cartridge-build-tools.py --output output/companion-example
```

It checks profile/overlay generation, predictions, conversion, recipe export and verification. Its image is not executable TI software and should remain on the computer.

For the seven real launch helpers, follow [Rebuild the bridges](adapters.md#rebuild-the-bridges). For real companions, follow [Rebuild one example](adapters.md#rebuild-one-example); that requires the exact cartridge image separately.

## CPLD build and verification

The released programmable-logic sources, pins and configuration are in [vhdl/gigacart](../../vhdl/gigacart/). A fresh fit requires Xilinx ISE **14.7**, targeting **XC95144XL-10-TQ100**. Stage that directory's files into a new build directory and use its XST project and vector-export configuration. The original basic CPLD source is a separate upstream reference.

The AVR builder does not run the CPLD fitter. Checking the PLD file and source hashes is different from regenerating the PLD or measuring hardware timing. Existing fit reports are retained in the [release evidence](../../release/2.0-beta1-125ms/evidence/cpld/).

Initial programming of a new board also requires a suitable bootloader and the ATmega 328P's 8 MHz clock configuration. The application builder does not install these. See the [original board-programming description](../../doc/readme.html) for context; upgrading an already programmed board uses [Install and play](../getting-started.md).

## Contributing changes

Keep Markdown documentation and file indexes current. No documentation build or PDF tooling is required. Check the affected build outputs and describe hardware testing separately from host checks.

The upstream comparison point is `26c223f6b9321d97356ffcd9719d2665f1070237`. Use focused commits. When testing mail-format patches, create the destination checkout with `core.autocrlf=false` and use `git am --keep-cr`: preserved CRLF bytes participate in exact hash checks. Rebuild after applying the patch.
