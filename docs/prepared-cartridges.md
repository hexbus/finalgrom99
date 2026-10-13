# Set up a large cartridge

[Start here](README.md) · [Install and play](getting-started.md) · [Troubleshooting](troubleshooting.md)

Already have the complete set? Go to [Copy a complete set](#copy-a-complete-set). Only have its supported image? Use [Assemble a set](#assemble-a-set-from-a-supported-image).

## Why there are extra files

FinalGROM has 1 MB of SRAM: the working memory the TI reads. A larger image stays on SD, and FinalGROM brings small pieces called **banks** into SRAM as needed. The adapted program can wait safely while this happens. Some banks stay resident; others reuse the remaining memory.

The helper, or **companion**, files tell FinalGROM how to start the program, apply its prepared changes, and optionally load likely next banks early. They must match the image. Creating an adaptation is programming work; the tools cannot discover the required changes in an unknown ROM.

Supported ordinary ROM-only images up to 1 MB can fit entirely in SRAM. Being larger than 512 KB does not by itself require these files. See [size limits](release-notes.md#cartridge-size-limits) for mixed ROM/GROM images and the prepared-image limit.

## Which files go on the card?

Keep the supplied basename, such as `DRAGON`, the same across the set. Use the supplied image extension.

| File | In plain English | Copy to SD? |
| --- | --- | --- |
| `FGMENU.BIN` | The menu from the matched firmware release | Yes, in the card root |
| `.GCR` or `.GCS` | The prepared cartridge image | Yes |
| `.GLP` | Startup plan and initial resident data | Yes; required |
| `.GOV` | Prepared changes applied to banks in memory | Yes; required even when no patches are needed |
| `.GPR` | Suggestions about which banks to load next | Include when supplied; the loader can run without predictions |
| `.GIR`, `.GIS`, `.BIX` | A map of image data on this particular card | Let FinalGROM create these; do not transfer them from another card |
| Recipes, catalogs, `bridge.bin`, manifests and reports | Developer inputs and verification records | No; keep them on the computer |

A `.GCS` image uses a `.GIS` location index; a `.GCR` image uses `.GIR`. The startup helper `bridge.bin` is already packed inside GLP. Players do not copy it separately.

## Copy a complete set

First complete [Install and play](getting-started.md) so both devices and the menu use the matched Beta 1 release.

1. Copy the image and matching GLP, GOV and supplied GPR into one folder on a supported FAT32 card. Short uppercase names avoid filename alias confusion.
2. Leave out generated GIR/GIS/BIX indexes when moving the set from another card. Keep the other companions.
3. Select the image in the FinalGROM menu. Initial indexing may take time; it happens automatically.

For the supplied Dragon's Lair preparation, the card should look like this:

```text
SD root/
  FGMENU.BIN
  DRAGON/
    DRAGON.GCS
    DRAGON.GLP
    DRAGON.GOV
    DRAGON.GPR
```

**Expected result:** the cartridge starts after its initial load. Keep the SD card inserted throughout play. A correctly prepared set can still pause while loading banks; predictions reduce waiting but do not guarantee native-cartridge timing.

Do not mix companions from different preparations or rename just one member. For a separately supplied ZIP with an `sd-card/` folder, copy that folder's contents to the card. Keep its guides and manifest on the computer.

## Assemble a set from a supported image

You need Python 3.9 or newer, a checkout of this repository, and the exact image recognized by the catalog. Full game images are not included.

The tool is named `convert-cartridge.py`, but the built-in entries **keep your image bytes unchanged and attach the matching companions**. They recognize the already-prepared Beta 1 images. A stock ROM with the same title may not match.

Run these commands in PowerShell from the repository root. Replace the input path with your image and choose an output directory that does not already exist:

```powershell
python tools/convert-cartridge.py "C:/ROMs/DRAGON.GCS" --inspect
python tools/convert-cartridge.py "C:/ROMs/DRAGON.GCS" --output output/dragon
python tools/verify-prepared-set.py output/dragon/sd-card/DRAGON.GCS
```

1. **Inspect:** the result should list `dragon-beta1` as a match. Matching uses SHA-256, a fingerprint of the file's contents, not its name or size. If there is no match, see [image not recognized](troubleshooting.md#the-converter-does-not-recognize-my-image).
2. **Assemble:** the second command writes the four `DRAGON` files into `output/dragon/sd-card/`, with a separate `manifest.json` on the computer.
3. **Verify:** the last command should report `"verified": true`. This checks file consistency, not every gameplay sequence.
4. Copy the four files into a `DRAGON` folder on the card, as shown above. The converter does not supply firmware or `FGMENU.BIN`; obtain those through the installation guide.

A differently named BIN with identical bytes can match. A single-BIN RPK is also accepted. For another included image, supply its input path; the catalog chooses the output name and extension.

### Included images

These are the output names in the [built-in catalog](../templates/gigacart/beta1/catalog.json). Each entry records the required input fingerprint.

| Example | Output image | Preparation |
| --- | --- | --- |
| Bad Apple variant | `BABUFFER.GCR` | Music/speech choice and predictions |
| Video | `CINFRAME.GCS` | 128 MB image and predictions |
| Dragon's Lair | `DRAGON.GCS` | 128 MB image, joystick/keyboard choice and predictions |
| Games compilation | `GAMES1M.GCR` | Added startup bank; no prediction file |
| Games compilation | `GAMES2M.GCR` | Added startup bank; no prediction file |
| Spaceballs | `SPACE.GCR` | Prediction file supplied |
| Stealth Runner | `STLSRV2.GCR` | Prediction file supplied |

An example is not a promise of complete-game testing. Read [known playback issues](troubleshooting.md#known-playback-issues) before judging a symptom as a new failure.

For a separately supplied, reviewed adapter pack, add `--catalog "C:/Adapters/catalog.json"` to inspection and conversion. If it offers multiple matching variants, use its recommended `--adapter` ID. The image fingerprint is still checked.

## If your image is not supported

A different large ROM needs an adapter for its exact revision. Changing the filename or filling in a template cannot create the required request-and-wait code. [Cartridge adapters](development/adapters.md) explains that development work, the examples, and rebuilding companions.
