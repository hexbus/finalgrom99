# Gigacart source templates

Players should start with [large-cartridge setup](../../docs/prepared-cartridges.md). Developers should follow [Rebuild or develop an adapter](../../docs/development/adapters.md). This directory contains inputs for those developer workflows.

The three `.template.json` files below are generic editable examples of the current tool inputs, not runnable cartridges or ready-to-use adapters. Copy them to a working directory before editing. Replace every `REPLACE_...` value and review every example bank, patch and slot assignment. These generic files include no game ROM or launch bridge.

- `recipe.template.json`: source for GLP startup and GOV overlay generation.
- `prediction.template.json`: source for the optional GPR read-ahead model.
- `catalog.template.json`: exact-ROM adapter entry for the converter.

The [portable tool exercise](../../tools/test-cartridge-build-tools.py) fills these templates with synthetic data and checks the complete build, export and conversion path. Its output is not executable TI software and must stay on the computer.

## Actual Beta 1 examples

The `beta1/` subdirectory contains the recipes, launch bridges, available prediction models and verified final companions for seven actual beta preparations: BABUFFER, CINFRAME, DRAGON, GAMES1M, GAMES2M, SPACE and STLSRV2. Each example's `final/` directory contains the GLP/GOV and available GPR files, matching the hashes in `rebuild.json`.

The converter uses `beta1/catalog.json` by default, including `dragon-beta1` alongside the other six adapters. Supply the exact supported image; it attaches the matching final companions. See the [converter walkthrough](../../docs/prepared-cartridges.md#assemble-a-set-from-a-supported-image).

See [Cartridge adapter guide](../../docs/development/adapters.md) for image requirements, rebuild commands, testing status and package-building instructions. The package builder generates a manifest and checksums for each completed set.

`bridge.bin` is the supplied TI startup helper, packed into GLP by the builder. Read [what it does and where it came from](../../docs/development/adapters.md#what-is-bridgebin). The assembly source and settings are included, with an optional command that reproduces all seven bridges exactly. Players do not copy this file separately.

**Cartridge images are user-supplied**, except for the [separate Cinema example download](../../docs/prepared-cartridges.md#copy-a-complete-set). The source tree includes no full game or video images; generated ZIPs belong under ignored `output/releases/`. These examples reproduce companions for exact already-prepared beta images, not arbitrary original ROMs. Empty runtime patch lists in some examples mean that their prepared images already contain the required cooperation. Final GLP/GOV files include cartridge-derived startup and overlay bytes; they are reference outputs, not generic profiles for other images.
