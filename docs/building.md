# Building and preparing the patch

The active AVR sources reproduce the shipped 125 ms release. The fixed release identity, original source mapping and compiler command list are in `release/2.0-beta1-125ms/`. Keep those baseline files unchanged while developing the active sources. The [firmware guide](firmware-guide.md) explains the runtime; the [AVR index](../avr/index.md) and [tools index](../tools/index.md) identify individual files and their origins.

## Verify the baseline

Use Python and AVR GCC 15.2.0 with matching AVR binutils. Run from the repository root, replacing the example xdt99 path with its installation directory:

```powershell
python tools/build-firmware.py --verify-release --xdt99 C:/tools/xdt99
```

Supply the directory containing `xas99.py` for `--xdt99`, or omit that option to build only the AVR. `--toolchain-bin` selects a compiler installation. Each run creates a new directory under `output/`; it neither deletes older runs nor flashes hardware.

`--verify-release` checks each maintained input against `tools/firmware-build.json` and compares the resulting AVR with the shipped binary. The recipe also pins the frozen snapshot and provenance hashes. Original source hashes remain unchanged in the frozen snapshot. The menu check assembles the TI source and compares it with `FGMENU.BIN`. A changed menu also requires reviewing the AVR's size, checksum and status offsets.

For development, omit `--verify-release`. Modified inputs are reported and the 28 KiB application limit remains enforced. Adding translation units requires evolving the build recipe deliberately; keep a separate immutable release manifest.

`tools/firmware-build.json` selects 70 staged inputs and 19 compilation units, with the verified source hashes and compiler/link commands. `tools/source-layout.json` records retained relocations from the frozen source map. The shared `tools/source_layout.py` resolves these inputs, checks frozen metadata and validates the object list. The builder stages original filenames so existing includes resolve. Edit maintained sources, not staged copies.

The build needs no archive checkout, ignored references, symlinks or external source paths. `avr/compilation-session.inc` remains required build support. Archived controllers and diagnostics are excluded; historical diagnostic flags are unsupported in this source set. The original 113-input recipe remains frozen evidence and can be reproduced in the reference repository described in [maintainer provenance](preservation.md#reference-repository).

The portable companion-tool example uses synthetic data only:

```powershell
python tools/test-cartridge-build-tools.py --output output/companion-example
```

Choose a new output directory on each run. Its synthetic image is not executable TI software.

## CPLD and evidence limits

`vhdl/gigacart/` contains released logic, pins, XST configuration and vector-export commands. Fresh fitting requires Xilinx ISE 14.7 for XC95144XL-10-TQ100. Stage those files into a new build directory. Historical fit reports remain under the release evidence; those reports do not establish a new fit or physical timing qualification. The basic CPLD source is retained upstream reference code.

The [startup test record](release-notes.md#startup-delay-125-ms) describes the reported PicoPEB result and its limits. A matching build does not establish hardware compatibility.

## Documentation and patch workflow

Maintain the README, `docs/`, and source-directory indexes as Markdown. No documentation build step is required.

Compare changes with the pinned upstream commit `26c223f6b9321d97356ffcd9719d2665f1070237`. Use a development branch and focused commits. Configure remotes for the destination repository.

When testing a mail-format patch, create the destination checkout with `core.autocrlf=false` and apply with `git am --keep-cr`. Some preserved source/evidence files retain CRLF bytes; default mail handling can strip them and break exact hash verification. Rebuild and verify the destination checkout after applying the patch.

Before submission, review omitted stock features, inactive source inputs, CPLD build portability and affected behavior. A matching AVR rebuild is only one submission check. Exclude ignored references and private inputs.
