# Gigacart source templates

Start with [Preparing and copying a Gigacart](../../docs/cartridge-preparation.md). Its plain-English introduction explains the whole journey from ROM to SD card, then separates player installation from adapter development.

The three `.template.json` files below are generic editable examples of the current tool inputs, not runnable cartridges or ready-to-use adapters. Copy them to a working directory before editing. Replace every `REPLACE_...` value and review every example bank, patch and slot assignment. These generic files include no game ROM or launch bridge.

- `recipe.template.json`: source for GLP startup and GOV overlay generation.
- `prediction.template.json`: source for the optional GPR read-ahead model.
- `catalog.template.json`: exact-ROM adapter entry for the converter.

The [portable tool exercise](../../tools/test-cartridge-build-tools.py) fills these templates with synthetic data and checks the complete build, export and conversion path. Its output is not executable TI software and must stay on the computer.

## Actual Beta 1 examples

The `beta1/` subdirectory contains the recipes, launch bridges, available prediction models and verified final companions for seven actual beta preparations: BABUFFER, CINFRAME, DRAGON, GAMES1M, GAMES2M, SPACE and STLSRV2. Each example's `final/` directory contains the GLP/GOV and available GPR files, matching the hashes in `rebuild.json`.

The converter uses `beta1/catalog.json` by default, including `dragon-beta1` alongside the other six adapters. Supply the exact supported image; it attaches the matching final companions. See the [converter walkthrough](../../docs/cartridge-preparation.md#convert-a-supported-image).

See [Beta cartridge recipe examples](../../docs/beta-cartridge-recipes.md) for image requirements, rebuild commands, testing status and package-building instructions. The package builder generates a manifest and checksums for each completed set.

`bridge.bin` is the supplied TI startup helper, packed into GLP by the builder. Read [what it does and where it came from](../../docs/beta-cartridge-recipes.md#what-is-bridgebin-and-where-did-it-come-from). The assembly source and settings are included, with an optional command that reproduces all seven bridges exactly. Players do not copy this file separately.

The source tree includes no full cartridge images; generated ZIPs belong under ignored `output/releases/`. These examples reproduce companions for exact already-prepared beta images, not arbitrary original ROMs. Empty runtime patch lists in some examples mean that their prepared images already contain the required cooperation. Final GLP/GOV files include cartridge-derived startup and overlay bytes; they are reference outputs, not generic profiles for other images.
