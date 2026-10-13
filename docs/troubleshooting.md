# Troubleshooting

[Start here](README.md) · [Install and play](getting-started.md) · [Large cartridges](prepared-cartridges.md)

Start with the symptom below. Keep the complete matching cartridge set together, and confirm that both the AVR and CPLD have the intended firmware. A menu label alone does not identify the installed pair.

## Firmware update problems

Updates start automatically when FinalGROM finds an update file in the SD root. Follow the [one-file-at-a-time update steps](getting-started.md#2-update-the-avr).

| During an update | What to check |
| --- | --- |
| LED still busy | Do not interrupt programming. Successful completion leaves the LED off. |
| Repeating groups of one flash | Card, card reader, or filesystem access failed. Check the FAT32 card and connection. |
| Repeating groups of two flashes | The updater could not read the update file. Check its downloaded contents, name, and placement in the root. |
| Higher flash counts during a CPLD update | Record the count. It indicates a CPLD programming/XSVF-player error. Recheck the matched PLD file and original instructions before retrying. |
| Update repeats at every restart | Remove the completed update file from the card before normal use. |

These flash meanings apply to the updater; runtime indications can differ. A display freeze during a CPLD update does not by itself mean failure. See the [original update instructions](../doc/MANUAL.md#how-to-update-the-finalgrom-99) for recovery details.

## The cartridge is missing or will not load

Check these in order:

1. Use the matched firmware and menu from [Install and play](getting-started.md).
2. Confirm the image is supported by the adapter. Size or title alone is insufficient.
3. Restore the image and matching companions. Check their shared basename and supplied extension. Both GLP and GOV are required.
4. Let FinalGROM create the location index for this card. Do not copy an index from a different card.
5. If you have Python, run the [file verification command](prepared-cartridges.md#assemble-a-set-from-a-supported-image) against the prepared image on the computer.

Missing or invalid startup files can prevent launch. Some overlay checks occur only when the affected bank is loaded, so an error can appear later. Record the exact message and whether failure happens at selection, startup, or during play.

## The converter does not recognize my image

The built-in catalog recognizes exact prepared Beta 1 images. It is not a universal stock-ROM converter. Do not change a catalog hash to make a mismatching image pass: the companion data still belongs to the original image.

Obtain the matching input or a reviewed adapter for your revision. Select a supplied adapter pack with `--catalog`; see [supported-image setup](prepared-cartridges.md#assemble-a-set-from-a-supported-image).

If the error instead says the output already exists, choose a new output directory. The tools avoid overwriting earlier results.

## Playback pauses or stutters

Some waiting is part of loading banks from SD. A prediction file can reduce it by suggesting likely next banks, but it cannot guarantee native-ROM timing.

1. Restore the supplied GPR if the set includes one. Missing or rejected predictions can increase pauses.
2. Compare the same sequence on a second FAT32 card, using the same complete set and freshly generated indexes.
3. Record the SD card make/model, exact cartridge set, console configuration and a short video of the affected sequence.

Stuttering alone does not prove that the index or prediction file failed. SD latency, card allocation and program behavior can all matter. Two Dragon's Lair testers reported different amounts of jitter; the cause has not been established.

## Rebuild a card-location index

FinalGROM checks an existing index against the image's current allocation before reuse. Recognized stale indexes can be rebuilt. Fragmentation, unsuitable file size, write failures, or an unexpected file at the index name can still prevent recovery.

Back up the card. Remove only the generated GIR/GIS/BIX index for the affected image, then select the image again. The [file table](prepared-cartridges.md#which-files-go-on-the-card) distinguishes indexes from required companions. FinalGROM cannot recreate GLP, GOV or GPR from an unknown ROM.

## Known playback issues

These are reported symptoms, not confirmed root causes. Detailed setups and evidence are in [Test reports](reference/test-reports.md).

| Title or area | Current observations |
| --- | --- |
| Dragon's Lair | Very slight stutter in one report; much heavier jitter in another. Comparable installed-file identities and card tests remain pending. |
| Stealth Runner | Opening speech corruption and minor sprite glitches reported; later speech and controls reported to work. |
| Bad Apple | Speech/vocals have garbling or synchronization concerns. A music-only version reportedly completed a run. Exact variants matter. |
| Spaceballs | Dialogue clarity concerns remain. Low-fidelity source audio has been suggested, but does not establish the cause of every distortion. |
| Loading | One PAL tester reported one freeze in roughly 20–30 loads; the exact title and stage were not identified. |
| CINFRAME | An earlier version had a stray-tone report after eight hours. The current preparation silences unused channels each frame; another long run is useful. |
| Game compilations | Spot checks passed; individual-title results and complete playthroughs remain limited. |

## Report a problem

Use the fork's issue tracker if enabled, or the beta testing discussion where you obtained the files. Include:

- Cartridge filename and image/companion revision or hashes, if available.
- Release files used for both firmware updates and `FGMENU.BIN`.
- PAL/NTSC, console model, expansions, speech synthesizer, and video modifications.
- SD card make/model and whether another card changes the result.
- Exact steps, error text or flash count, and whether cold start and reset differ.
- A recording of the same failing scene and any comparison using a native cartridge.

Separate observations from suspected causes. File verification establishes matching data; it does not measure hardware timing or prove all game paths work.
