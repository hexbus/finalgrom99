# Rebuild or develop a cartridge adapter

[Start here](../README.md) · [Supported-image setup](../prepared-cartridges.md) · [File formats](../reference/file-formats.md)

An adapter makes one specific cartridge revision work with FinalGROM's bank-loading protocol. This page is for developers. Players with a complete set only need [Install and play](../getting-started.md).

## Choose the task

| Task | Starting material | Result |
| --- | --- | --- |
| Attach existing companions | Exact image recognized by the catalog | Use the [converter walkthrough](../prepared-cartridges.md#assemble-a-set-from-a-supported-image); no recipe editing |
| Rebuild an example | Exact prepared image plus supplied recipe and bridge | GLP/GOV and optional GPR matching the reference outputs |
| Develop a new adaptation | Original ROM, knowledge of its banking and program flow, and TI assembly skills | Reviewed image changes, launch code, recipe and evidence for that revision |

The supplied recipes rebuild companions for already-prepared images. They do not recover the full original-ROM adaptation process. Editing filenames or replacing hashes cannot make an unknown cartridge safe to stream.

## Rebuild one example

Run from the repository root with Python 3.9 or newer. This Spaceballs example uses a new working directory and copies your prepared image into it; the source image is left unchanged. Replace `C:/Prepared/SPACE.GCR` with its actual location.

```powershell
New-Item -ItemType Directory output/space-recipe -ErrorAction Stop
Copy-Item templates/gigacart/beta1/SPACE/* output/space-recipe/ -Recurse
New-Item -ItemType Directory output/space-recipe/input
Copy-Item C:/Prepared/SPACE.GCR output/space-recipe/input/SPACE.GCR
python tools/build-cooperative-profile.py output/space-recipe/recipe.json `
  --output output/space-recipe/built
python tools/build-giga-predict.py output/space-recipe/prediction.json `
  output/space-recipe/built/SPACE.GPR
Copy-Item output/space-recipe/input/SPACE.GCR output/space-recipe/built/
python tools/verify-prepared-set.py output/space-recipe/built/SPACE.GCR
```

Check that each command succeeds before continuing. Use the corresponding directory, basename and extension for another example. Skip the prediction command for GAMES1M and GAMES2M. Do not substitute a stock ROM or another beta revision when an identity check fails.

Compare the generated companion hashes with `rebuild.json` before copying the set to SD. This command reports each comparison and stops on a mismatch:

```powershell
$work = 'output/space-recipe'
$expected = Get-Content "$work/rebuild.json" -Raw | ConvertFrom-Json
foreach ($item in $expected.companions.PSObject.Properties) {
  $actual = (Get-FileHash "$work/built/$($item.Name)" -Algorithm SHA256).Hash
  if ($actual -ne $item.Value) { throw "Mismatch: $($item.Name)" }
  Write-Output "Matched: $($item.Name)"
}
```

The recipe and prediction compilers plus the verifier are included in this repository. No archived scripts or emulator are required for these companion rebuilds. The exact prepared image is still required; its original-ROM preparation steps are not recovered by these examples.

## What the example files contain

Each directory under [templates/gigacart/beta1](../../templates/gigacart/beta1/) contains:

| File | Purpose |
| --- | --- |
| `recipe.json` | Image/bridge identities, fixed-bank layout, reusable slots and runtime patches |
| `bridge.bin` | Supplied TI launch helper, embedded into GLP by the builder |
| `prediction.json` | Optional source model for GPR; absent from the compilation examples |
| `rebuild.json` | Expected hashes and verification results |
| `final/` | Reference GLP/GOV and available GPR outputs |

Recipes refer to `input/NAME.GCR` or `input/NAME.GCS` relative to their own directory. Create that directory in your working copy, as in the example. Full images are supplied separately. Example `input/` and `built/` directories are ignored; `final/` is tracked.

### Example layouts

Sizes are bytes; bank and slot numbers refer to 8 KiB units. The [player catalog](../prepared-cartridges.md#included-images) gives output names.

| Example | Image bytes | Fixed banks | Reusable slots | Notes |
| --- | ---: | ---: | ---: | --- |
| BABUFFER | 5,079,040 | 1 | 80 | Predictions; image already contains cooperation |
| CINFRAME | 134,217,728 | 1 | 80 | Predictions; image already contains cooperation |
| DRAGON | 134,217,728 | 44 | 80 | Overlay and 821 prediction records |
| GAMES1M | 1,056,768 | 3 | 80 | Compilation patches; no GPR |
| GAMES2M | 2,105,344 | 4 | 80 | Compilation patches; no GPR |
| SPACE | 2,105,344 | 2 | 80 | Small overlay and predictions |
| STLSRV2 | 2,105,344 | 46 | 78 | Overlay and predictions |

The compilation images include an extra 8 KiB startup bank. Empty runtime patch lists in BABUFFER and CINFRAME do not mean that an unmodified original image will work. Bridges, patch data and GLP/GOV payloads may contain cartridge-derived bytes; the examples are not a blanket license to redistribute a cartridge.

## What is bridge.bin?

It is the small TI assembly program that launches a prepared cartridge from the menu. The builder embeds it in GLP. At launch, FinalGROM loads it into SRAM slot 0. It restores saved console scratchpad state, installs a wait routine there, requests the starting bank, and enters the program once that bank is ready.

The supplied binaries were extracted from verified startup profiles and reproduced from the included assembly sources. Normal companion rebuilds reuse them and need no assembler. New cartridges may need different launch code.

| Source | Variants | Verified size |
| --- | --- | --- |
| [player-boot-mailbox.a99](../../ti/player-boot-mailbox.a99) | All examples except DRAGON; starting bank, entry address and optional chooser | CINFRAME 150 bytes; BABUFFER 364; other four 158 |
| [dragon-choice.a99](../../ti/dragon-choice.a99) | DRAGON, using `MAILBOX=1` | 438 bytes |

BABUFFER offers a music/speech choice; DRAGON offers joystick/keyboard selection. All bridges fit the compiler's 512-byte payload sector. [bridge-builds.json](../../templates/gigacart/beta1/bridge-builds.json) records their sources and parameters.

### Rebuild the bridges

Install xdt99 and replace the example assembler path. Run from the repository root, with a new output directory:

```powershell
python tools/rebuild-beta-bridges.py --assembler C:/tools/xdt99/xas99.py `
  --output output/bridge-check
```

Expected result: all seven binaries match, with results written to `results.json`. This checks source reproduction, not new hardware behavior.

## Develop a new adaptation

This requires analysis and testing of the cartridge program. The tools validate file layouts and expected bytes; they cannot decide where a game may safely wait.

1. Identify and preserve the exact original image and its hash. Work on a copy.
2. Determine its banking scheme, entry point, resident-code needs and bank-change paths. Arrange safe execution from console RAM while missing banks are loaded.
3. Implement and review the request/wait changes and a suitable launch bridge. Keep original-image edits separate from patches applied to banks at runtime.
4. Record fixed-bank ranges, reusable slots and reviewed runtime patches in a recipe. Bind the image and bridge by hash.
5. Build GLP/GOV. Add a prediction model only when useful; correctness must not depend on predicting every request.
6. Verify the complete set, test the affected execution paths and hardware, then package the adapter with honest coverage and known limits.

Copy the [editable templates](../../templates/gigacart/README.md) into a work directory and replace every placeholder. Their example layouts are illustrative, not tested adaptations. The [JSON field reference](../reference/file-formats.md#json-inputs) explains the fields and path rules.

With a reviewed image, bridge, `recipe.json` and optional `prediction.json` under `output/adapter`, run:

```powershell
python tools/build-cooperative-profile.py output/adapter/recipe.json `
  --output output/adapter/built
python tools/build-giga-predict.py output/adapter/prediction.json `
  output/adapter/built/GAME.GPR
Copy-Item output/adapter/GAME.GCR output/adapter/built/GAME.GCR
python tools/verify-prepared-set.py output/adapter/built/GAME.GCR
```

Use the recipe's actual name and image extension; omit the prediction command when no model exists. A catalog then binds the original input, any image edits, prepared output and companion hashes. Ship its referenced companions with it. Field meanings belong in the [format reference](../reference/file-formats.md).

## Export an existing working set

Put the prepared image and matching companions together, then run:

```powershell
python tools/export-prepared-recipe.py "C:/Prepared/DRAGON.GCS" `
  --output output/dragon-recipe
```

The exporter verifies the set and requires byte-identical recompilation before writing a recipe, bridge, optional prediction model and rebuild report. The recipe points to the existing image; it does not copy it. If you relocate the recipe, update that path without changing the image hash.

Export does not recover original-ROM preparation edits. Preserve those and the original input hash separately. Keep generated work under ignored `output/` until selected for inclusion.

## Build complete packages

This optional tool builds ZIPs for all seven examples. It requires their exact prepared images in one directory; it does not download them. Run with Python 3.9 or newer from the repository root:

```powershell
python tools/package-beta-cartridges.py --images C:/Prepared `
  --output output/releases/beta-repack
```

Each ZIP contains an `sd-card/` folder with the image, matching companions and release menu, plus instructions, Markdown guides, a manifest and checksums. Firmware updates and generated card indexes are excluded. Revision R5 uses the reorganized guides; cartridge data and firmware are unchanged.

The builder checks image identities, rebuilds companions, compares their reference hashes, verifies the menu and reads each ZIP back. It writes a package index and archive checksums beside the ZIPs. Keep that newly generated index with the packages: guide edits and compression versions can change ZIP hashes without changing cartridge data. The recorded R3 index in the templates describes historical packages, not new builds.

## Verification and hardware status

All seven companion examples reproduce their reference hashes: see the [six-example audit](../../release/2.0-beta1-125ms/evidence/imported/tool-audit/current-six-companion-rebuilds.json) and [Dragon's Lair audit](../../templates/gigacart/beta1/additional-companion-rebuilds.json). All bridges reproduce from source, and conversion has been checked against all seven exact images.

These checks establish file identity. [Test reports](../reference/test-reports.md) record hardware observations and unresolved issues; [Troubleshooting](../troubleshooting.md#known-playback-issues) summarizes symptoms. New original-ROM revisions require their own adaptation and validation.
