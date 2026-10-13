# Preparing and copying a Gigacart

A cartridge image, often called a **ROM**, is a file containing a game or program. An ordinary supported cartridge is simple to use: copy its image files to the SD card, then choose it in FinalGROM. A very large cartridge needs a little more help.

## How large cartridges fit

Think of the SD card as a bookshelf and FinalGROM's SRAM as a small desk. The bookshelf holds lots of books, but the desk only has room for 1 MB of pages. The TI reads the pages on the desk. A game that fits can have all its pages put there before it starts.

A bigger game needs to take turns: keep some pages on the desk, put others away, and bring in the next ones from the bookshelf. These pieces of the cartridge are called **banks**. Reading the SD card takes time, so the game needs a way to say, "I need this bank; I'll wait until it is ready."

**Preparation gives a particular cartridge that request-and-wait behavior.** It also supplies instructions telling FinalGROM how to start it and which banks must stay in memory. An optional prediction file suggests which banks to bring in early. The board still has 1 MB of SRAM; it reuses that space while the full image stays on SD. Waiting can cause pauses.

FinalGROM cannot work out all these changes just by looking at any large ROM. Someone must first make and test an adaptation for that exact cartridge version. Once that is done, the player can use the finished set without understanding the programming.

## The whole journey, from ROM to running cartridge

1. **Start with a particular ROM.** Its exact version matters, because preparation may change specific instructions inside it.
2. **An adapter author prepares it.** They create any required image changes, a launch helper and a recipe describing the startup layout and patches. They may also create a prediction model. This is the programming work.
3. **The computer builds a matching set.** The prepared image becomes the GCR/GCS file. The tools produce GLP startup instructions, GOV patches, and optionally GPR predictions. These files belong together.
4. **The player copies that set to SD.** With the matched firmware and menu installed, the player selects the cartridge. There is no recipe to edit and no programming to do.
5. **FinalGROM runs it using its existing SRAM.** It creates its own map of where the image is on this card, loads the initial banks, starts the cartridge and services later bank requests.

The **recipe** is the builder's set of instructions, not a file the player puts on SD. **`bridge.bin`** is a tiny program that helps start the prepared cartridge. It was written in TI assembly language and assembled into bytes. The supplied examples already include it; the builder packs it inside GLP. Players do not copy `bridge.bin` separately. See [where the bridge comes from](beta-cartridge-recipes.md#what-is-bridgebin-and-where-did-it-come-from) for its source and an optional rebuild.

## Which route do I use?

| What you have | What you do next |
| --- | --- |
| A complete prepared cartridge set | Follow the copying steps below. No Python or assembler is needed. |
| An exact supported beta image, including Dragon's Lair | Use the [built-in converter](#convert-a-supported-image) to attach its matching companions, or the [recipe rebuild guide](beta-cartridge-recipes.md#rebuild-one-example) to rebuild them. |
| An original ROM and a qualified adapter pack for that exact version | Use the [converter instructions](#convert-a-supported-image). Revisions outside the built-in catalog need their own reviewed pack. |
| An unsupported original ROM, or just a generic template | It needs adaptation work first. Filling in filenames or renaming the ROM cannot make it safe to stream. |

The [beta packages](beta-cartridge-recipes.md#build-a-complete-package) already contain prepared sets. The sample recipes demonstrate rebuilding their companions; they are not a complete original-ROM conversion pipeline.

**Above 512 KiB does not automatically require preparation.** Supported ordinary ROM-only images up to 1 MiB fit in SRAM. Larger cartridges need preparation to load banks from SD into that same SRAM. See [size limits and compatibility](release-notes.md#larger-cartridges-on-the-existing-1-mib-board), including mixed ROM/GROM limits.

## Players: installing a prepared cartridge

**Copy the complete supplied cartridge set and select its image.** The steps below are all a player needs; the later sections are for people rebuilding or developing adaptations.

1. Install the matched Beta 1 AVR and CPLD updates following the release instructions. Put that release's `FGMENU.BIN` in the SD root. A menu version alone does not confirm both devices were updated.
2. Copy the supplied GCR or GCS image and its matching GLP, GOV and supplied GPR together into one folder on a supported FAT32 card. Keep their shared basename. Subdirectories are supported; uppercase FAT 8.3 names avoid alias confusion.
3. When moving a set to another card, leave out generated GIR/GIS/BIX location indexes. Keep GLP, GOV and GPR. FinalGROM creates the destination card's index when the image is selected; there is no separate manual Prepare step.
4. Select the cartridge image in the FinalGROM menu. Initial indexing may take time. A correctly prepared cartridge can still pause while loading a bank; prediction reduces waiting but does not guarantee native-ROM timing.

For example, an already-prepared Dragon's Lair GCS set has this layout. The names illustrate an existing prepared set; this repository does not include the game files.

```text
SD root/
  FGMENU.BIN
  DRAGON/
    DRAGON.GCS
    DRAGON.GLP
    DRAGON.GOV
    DRAGON.GPR
```

Use the image extension supplied by the adapter. A GCS image generates GIS; a GCR image generates GIR. Renaming just one member breaks companion lookup. Do not mix companions from different preparations, even if the game title is the same.

| File | Purpose | What the player does |
| --- | --- | --- |
| GCR / GCS | Prepared cartridge image | Copy the supplied image |
| GLP | Startup plan and initial resident data | Copy its matching file; required |
| GOV | Reviewed program changes applied in SRAM | Copy its matching file; required even if no patches |
| GPR | Predictions of likely next banks | Include when supplied; optional to the loader |
| GIR / GIS / BIX | Locations of image data on this particular card | Let FinalGROM generate it |

Build recipes, catalogs, manifests and reports stay on the computer. Ordinary images supported by the native loader continue using their ordinary ROM/GROM files; these companions are for the prepared large-cartridge path.

## If a prepared set does not work smoothly

Restore the complete matching set first. Missing or invalid GLP/GOV can prevent loading; some overlay checks happen later when a bank is loaded. Missing or rejected GPR disables predictions and can cause more pauses. Stuttering alone does not establish which file or subsystem is responsible.

FinalGROM checks an existing location index against the image's current allocation before reuse. Recognized stale indexes can be rebuilt, but unsuitable size, fragmentation, an unrecognized file at the index name, or write failures can prevent recovery. Back up the card before removing an unwanted generated index, then select the cartridge again. FinalGROM cannot recreate GLP, GOV or GPR from an unknown ROM.

For comparisons, try the same complete set on another freshly formatted FAT32 card, omitting the generated location indexes. Record card make/model, both firmware versions and the same intro or gameplay sequence. Matching bytes and a valid index do not guarantee identical SD latency.

## Convert a supported image

The converter can package a reviewed adaptation for an **exact ROM revision**. It selects by SHA-256, not by filename or size. A single-BIN RPK is also accepted. Unknown revisions are refused rather than guessed.

**The converter includes a [built-in catalog](../templates/gigacart/beta1/catalog.json) for all seven Beta 1 examples**, including Dragon's Lair. Give it the exact image identified by that catalog. It checks the hash, copies the image and attaches the matching supplied GLP/GOV/GPR. These entries preserve the image bytes; the GOV contains the runtime patches. They do not convert arbitrary stock ROM revisions or discover new adaptations.

With Python 3.9 or newer, run from the repository root. Replace the input path with your Dragon image and choose a new output directory:

```powershell
python tools/convert-cartridge.py "C:/ROMs/DRAGON.GCS" --inspect
python tools/convert-cartridge.py "C:/ROMs/DRAGON.GCS" --output output/dragon
python tools/verify-prepared-set.py output/dragon/sd-card/DRAGON.GCS
```

Inspection should report `dragon-beta1`. A differently named BIN or single-BIN RPK with identical bytes also matches; a different revision is refused. The converter creates `DRAGON.GCS`, `DRAGON.GLP`, `DRAGON.GOV` and `DRAGON.GPR` in `sd-card/`. No separate bridge, prediction build or archived script is needed. For another included title, supply its matching image; the catalog chooses its name and extension.

Copy the contents of `sd-card` together to your card, with matched firmware and `FGMENU.BIN` already installed. The converter does not supply the menu or firmware. Keep `manifest.json` on the PC. File verification does not prove every game path or hardware timing.

For a separately supplied adapter pack, add `--catalog "C:/Adapters/catalog.json"` to inspection and conversion. If several variants match, use its recommended `--adapter` ID. Explicit selection still checks the input hash. Unsupported originals need adaptation work; the converter never invents patches from a title or filename.

## Adapter authors: editable templates

The [template directory](../templates/gigacart/README.md) contains JSON inputs for the tools already used to make GLP, GOV and GPR. These are source templates, not empty binary companions for players to fill in. Copy them into a work directory, remove `.template` from their names and replace every placeholder. The example bank layout is illustrative and has no gameplay qualification.

For real worked examples, use the [seven Beta 1 cartridge recipes](beta-cartridge-recipes.md). Those contain actual layouts, patches and launch bridges rather than placeholders, and require the exact prepared images identified by their hashes.

| Template | Consumed by | Result |
| --- | --- | --- |
| `recipe.template.json` | `build-cooperative-profile.py` | GLP and GOV plus a manifest |
| `prediction.template.json` | `build-giga-predict.py` | Optional GPR |
| `catalog.template.json` | `convert-cartridge.py` | Packaged image and existing companions |

There is deliberately no GIR/GIS/BIX template: those files describe allocation on the destination card. Binary field details have their authoritative home in [Cartridge companion formats](cartridge-formats.md).

### Recipe and prediction fields

The recipe's `image` path is relative to the recipe file. It refers to the exact **prepared image before runtime GOV patches**, including any required padding or bootstrap edits. `image_sha256` binds that image; `bridge` and `bridge_sha256` identify a reviewed TI launch routine. Generic templates omit the bridge; Beta 1 examples include it.

`fixed_ranges` maps logical ROM banks to physical SRAM slots: `bank` is the first logical bank, `count` its length, and `first` the first resident slot. `dynamic_first` and `dynamic_count` select the reusable pool. The compiler checks overlaps and reserved slots, but only cartridge analysis can establish which banks must remain resident.

Each patch has a logical `bank`, a byte `offset` within its 8 KiB bank, and equal-length hexadecimal `expected` and `replacement` strings. These must be reviewed program changes, not guessed values. The builder checks original bytes and produces both startup payloads and runtime overlays consistently. The recipe's patches do not rewrite the image file on disk.

The prediction model uses the same prepared-image SHA-256. Each interval's `first` and `last` are inclusive logical bank numbers; `successors` lists likely next banks in priority order. The loader uses at most eight distinct suggestions per lookup. These predictions do not replace the safe request-and-wait changes in the cartridge program.

With reviewed `recipe.json`, `prediction.json`, the image and bridge in `output/adapter`, the component workflow is:

```powershell
python tools/build-cooperative-profile.py output/adapter/recipe.json `
  --output output/adapter/built
python tools/build-giga-predict.py output/adapter/prediction.json `
  output/adapter/built/GAME.GPR
Copy-Item output/adapter/GAME.GCR output/adapter/built/GAME.GCR
python tools/verify-prepared-set.py output/adapter/built/GAME.GCR
```

Use the recipe's actual name and image extension; skip the prediction step if no model exists. Builders require new output destinations. A successful build proves format consistency, not that an arbitrary cartridge has been safely adapted.

### Converter catalog fields

The catalog's `input_sha256` identifies the original user input, and `output_sha256` identifies the prepared image. `image_edits` contains any reviewed preparation edits made to the image itself; it is separate from runtime GOV patches. It may remain empty only when those image bytes are unchanged.

Each image edit is an object with an absolute byte `offset` and Base64 `before` and `after` strings. Edits apply in order; each preimage must match the image at that step. These are Base64 strings, unlike the recipe's hexadecimal patch strings. Preserve original-to-prepared edits separately: exporting companions does not recover them.

`companions` records paths relative to the catalog directory and the SHA-256 of each generated file. GLP and GOV are required; remove the GPR entry if the adapter has no model. `name` is the shared uppercase alphanumeric stem, at most eight characters; `extension` is GCR or GCS. Set `requirements` to the matched firmware/hardware and `evidence` to actual verification and hardware results, including unresolved issues. A catalog label is not itself qualification.

Ship the completed catalog with its referenced companion files, not the unfinished template. The built-in catalog supplies the seven exact beta-image entries. Additional original-ROM revisions need their own preserved input hashes, reviewed edits and verification evidence before inclusion.

## Preserve an existing working preparation

An existing prepared set can be exported as a reproducible recipe. Put the prepared image and matching GLP/GOV/GPR together first, then run:

```powershell
python tools/export-prepared-recipe.py "C:/Prepared/DRAGON.GCS" `
  --output output/dragon-recipe
```

The exporter verifies the set and requires byte-identical recompilation before writing `recipe.json`, `bridge.bin`, optional `prediction.json` and `rebuild.json`. The recipe points back to the existing image; it does not copy it. To relocate the recipe, update that path while preserving the image hash. Rebuild with the component commands above, using the actual name and extension.

This export does not recover the original-ROM conversion. Retain original-input hashes and preparation edits separately. Keep generated exports under ignored `output/` until selected for inclusion; see the [Beta 1 examples](beta-cartridge-recipes.md) for what is included in the repository.
