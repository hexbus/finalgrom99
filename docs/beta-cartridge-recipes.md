# Beta cartridge recipe examples

The [Beta 1 examples](../templates/gigacart/beta1/) include a converter catalog, recipes and reference companions for seven supported beta images. Adapter authors can study the bank layouts and patches or rebuild the companions with the supplied tools. Players installing a complete prepared set can simply follow the [copying guide](cartridge-preparation.md).

**Rebuilding requires the exact already-prepared beta image.** Supply an image whose SHA-256 matches the recipe or converter catalog. The source-tree examples rebuild its GLP/GOV and, where supplied, GPR; they do not transform a stock BIN into that image or prove an unknown ROM safe to stream. Each recipe identifies its required image by SHA-256. An image with the same title or size is not sufficient.

## Included preparations

Each example specifies the required image identity, SRAM layout and companion files. Full cartridge images are not supplied by the repository.

| Example directory | Required prepared image | What the example demonstrates |
| --- | --- | --- |
| `BABUFFER` | `BABUFFER.GCR`, 5,079,040 bytes | One fixed bank, 80 reusable slots, prediction model; no runtime overlay patches. |
| `CINFRAME` | `CINFRAME.GCS`, 134,217,728 bytes | A 128 MiB video image using one fixed bank, 80 reusable slots and predictions. |
| `DRAGON` | `DRAGON.GCS`, 134,217,728 bytes | Dragon's Lair: 44 fixed banks, 80 reusable slots, joystick/keyboard bridge, overlay and 821 prediction records. |
| `GAMES1M` | `GAMES1M.GCR`, 1,056,768 bytes | Three fixed banks, compilation patches and 80 reusable slots; no GPR. |
| `GAMES2M` | `GAMES2M.GCR`, 2,105,344 bytes | Four fixed banks, compilation patches and 80 reusable slots; no GPR. |
| `SPACE` | `SPACE.GCR`, 2,105,344 bytes | Spaceballs: two fixed banks, a small overlay and predictions. |
| `STLSRV2` | `STLSRV2.GCR`, 2,105,344 bytes | Stealth Runner: 46 fixed banks, 78 reusable slots, overlay and a larger prediction model. |

The compilation images include an extra 8 KiB bank; their filenames do not specify their exact byte lengths. BABUFFER and CINFRAME have empty runtime patch lists because these recipes expect images already containing the required cooperation. An empty list is not a generic adaptation for another video or game.

Use the filenames and extensions shown above when following the examples. Dragon's Lair produces a `DRAGON.GCS` set. [Historical provenance](preservation.md#release-and-cartridge-provenance) records the source sets and verification evidence.

## Converter adapters

The converter's default [catalog](../templates/gigacart/beta1/catalog.json) includes `babuffer-beta1`, `cinframe-beta1`, `dragon-beta1`, `games1m-beta1`, `games2m-beta1`, `space-beta1` and `stlsrv2-beta1`. Each entry selects the exact image hash and references that example's `final/` companions. Follow the [converter walkthrough](cartridge-preparation.md#convert-a-supported-image) to produce a complete matching set without rebuilding companions. The recipes below remain available for studying or rebuilding the adaptation.

The catalog accepts the exact listed beta image bytes, not arbitrary stock revisions. Its image edits are empty because these entries keep the input unchanged; runtime patches remain in GOV.

## Files in each example

- `recipe.json`: exact image and bridge identities, fixed-bank layout, reusable slots and runtime byte patches. Its image path is `input/NAME.GCR` or `input/NAME.GCS`, relative to the recipe.
- `bridge.bin`: the small TI startup program described [below](#what-is-bridgebin-and-where-did-it-come-from). It is already supplied and checked by the recipe's SHA-256.
- `prediction.json`: the GPR source model, when the beta set has one. The compilation examples intentionally omit it.
- `rebuild.json`: expected companion hashes and the matching verification report. The field `hardware_tested_by_this_check` is false.
- `final/`: known-good GLP/GOV and available GPR files. Compare a rebuild with these finished outputs or the hashes in `rebuild.json`.

Overlay patches are stored as runs of changed bytes, with their expected original values. This avoids copying entire mostly unchanged ROM sectors into the recipes. Rebuilding these smaller patches still produces the exact original sector overlays and startup payloads. See [template fields](cartridge-preparation.md#recipe-and-prediction-fields) for their meaning.

The source tree includes recipe inputs and the selected final companions, but no full cartridge images or card-local location indexes. Example `input/` and `built/` directories are ignored by Git; the `final/` directories contain the supplied reference outputs. Bridges, patch bytes and final GLP/GOV payloads contain actual preparation data, including cartridge-derived bytes. Their presence is not a blanket license for cartridge redistribution.

## What is bridge.bin, and where did it come from?

**`bridge.bin` is the TI startup helper that launches a prepared cartridge from the FinalGROM menu.** The profile builder embeds it in GLP; players do not copy it separately. At launch, FinalGROM loads it into cartridge SRAM slot 0. It restores the saved console scratchpad, installs a wait routine in scratchpad RAM, requests the starting bank and enters the program when that bank is ready. Later bank requests are handled by the prepared image, overlays and firmware.

The TI assembly source is included in `ti/`. The [build settings](../templates/gigacart/beta1/bridge-builds.json) record the source, assembler flags and cartridge-specific parameters:

| Source | Variants | Verified bridge sizes |
| --- | --- | --- |
| [player-boot-mailbox.a99](../ti/player-boot-mailbox.a99) | All examples except DRAGON; starting bank, entry address and optional chooser | CINFRAME: 150 bytes; BABUFFER: 364; other four: 158 |
| [dragon-choice.a99](../ti/dragon-choice.a99) | DRAGON with `MAILBOX=1` | 438 bytes |

BABUFFER offers a music/speech choice; Dragon offers joystick/keyboard selection. All bridges fit the compiler's 512-byte payload sector. The supplied binaries have been reproduced byte for byte from these sources.

**Normal companion rebuilds use the supplied bridge unchanged** and need no assembler. A different cartridge may need different launch code; a matching filename or size is insufficient. For an optional source check, install xdt99 and replace the assembler path below:

```powershell
python tools/rebuild-beta-bridges.py --assembler C:/tools/xdt99/xas99.py `
  --output output/bridge-check
```

Use a new output directory. This command assembles all seven bridges, checks them against the supplied binaries and recipe hashes, and records `results.json`. It does not change firmware or establish new hardware behavior.

## Build a complete package

The package builder creates one ZIP per supported image. Package revision R4 bundles the Markdown guides directly; firmware and cartridge data are unchanged. Each ZIP contains an `sd-card/` directory with the image, matching companions and frozen `FGMENU.BIN`, plus installation instructions, Markdown guides, a manifest and checksums. Firmware updates and generated GIR/GIS/BIX indexes are excluded.

ZIPs and full cartridge images are not included in a clone; this procedure builds packages from images supplied by the user. If a complete set is supplied separately, follow the [installation guide](cartridge-preparation.md#players-installing-a-prepared-cartridge).

To build packages, collect the seven exact images listed above in one directory. Run from the repository root with Python 3.9 or newer, replacing `C:/Prepared` with that directory. Choose a new output directory:

```powershell
python tools/package-beta-cartridges.py --images C:/Prepared `
  --output output/releases/beta-repack
```

The builder checks image identities against the recorded audits, rebuilds companions, compares them with `final/`, checks the menu against release provenance, and reads each ZIP back for verification. It writes `packages.json` and archive checksums beside the ZIPs. Keep that generated index with the packages distributed: guide content and compression-library versions can change archive hashes without changing cartridge data.

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

## Verification and hardware status

All seven companion rebuilds match their recorded reference hashes; see [verification provenance](preservation.md#release-and-cartridge-provenance). All seven startup bridges reproduce from the included assembly sources. The converter has also been checked against all seven exact images and produces the matching companion files.

These checks establish file identity. Hardware observations and unresolved issues are recorded in [release notes](release-notes.md) and the [beta feedback log](beta-feedback.md). In particular, the Dragon's Lair development report describes **very slight stutter**, while a separate tester reports heavier jitter. See the [playback reports](beta-feedback.md#dragons-lair-playback-reports) for the distinction.

Additional original-ROM revisions require their own adaptation and validation. CINFRAME is the filename used by the included video example.
