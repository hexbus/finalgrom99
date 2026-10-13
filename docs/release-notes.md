# Beta 1 capabilities and limitations

[Start here](README.md) · [Install and play](getting-started.md) · [Known playback issues](troubleshooting.md#known-playback-issues)

This branch contains **FinalGROM 2.0 Beta 1 with the 125 ms startup delay**. It runs on the existing FinalGROM 99 board. The major addition is support for selected prepared cartridges larger than its 1 MiB of SRAM, including 128 MiB images.

Use the matched AVR, CPLD and menu in [release/2.0-beta1-125ms](../release/2.0-beta1-125ms/). The [manifest](../release/2.0-beta1-125ms/provenance.json) identifies the exact files. Documentation and source-name cleanup do not constitute new hardware qualification.

## Cartridge size limits

| Image | How it runs |
| --- | --- |
| Supported ordinary ROM-only image up to 1 MiB | Loaded entirely into SRAM, including images above 512 KiB; size alone does not require preparation |
| Mixed ROM/GROM image | Shares SRAM between ROM and GROM, leaving less room for ROM than the ROM-only maximum |
| Supported prepared image above 1 MiB, up to 128 MiB | Keeps the full image on SD and reuses SRAM slots as banks are requested |

The original FinalGROM already advertised up to 1 MiB; 512 KiB was not its documented capacity limit. Support above 1 MiB through cartridge-specific preparation is the new extension. The ordinary loader now uses explicit SRAM-bank writes and rejects read errors and oversized loads, but that does not establish universal compatibility with every banking scheme.

Prepared cartridges need an adaptation for the exact image revision. [Set up a large cartridge](prepared-cartridges.md) explains the supplied files and catalog. Loading pauses can remain.

## Feature status

| Feature | Beta 1 status |
| --- | --- |
| Ordinary ROM, supported banked ROM, GROM and ROM/GROM pairs | Retained |
| Folders, multiple menu choices, supported autostart and ordinary single-image automatic launch | Retained |
| Physical reset/menu operation | Retained |
| Prepared large cartridges | Added, using companions, optional predictions and a card-generated location index |
| Supplied TI-99/4 and JB/QI compatibility changes | Included, with both AVR and CPLD components |
| Mini Memory 4 KiB cartridge RAM | Retained; short BASIC SAVE/OLD passes reported for standard Mini Memory and S MINIMEM |
| Mini Memory contents saved across power-off | Not implemented |
| Larger general-purpose banked RAM, writable GRAM, combined RAM/GRAM | Omitted for space and lack of demonstrated use in the selected set |
| Per-cartridge help and program-controlled reload/dump | Omitted |
| Larger-RAM Mini Memory Plus variant, MBX and SuperSpace banking | Not supported |
| Prepared full-GROM GCG images | Not supported |
| Automatic adaptation of arbitrary large ROMs | Not implemented |
| Menu presentation | Restored color stripe with BETA, separate credits and 2.0 logo |

## Startup delay: 125 ms

The AVR waits 125 ms before initializing the SD card, compared with the earlier 5 ms startup wait. This repeats on AVR restart, not on each bank request. SD and menu loading add their own time; 125 ms is not the total time until the cartridge is ready.

The delay-only revision left the CPLD, menu artwork, cartridge files and formats, Mini Memory and stock 1.3 restore files unchanged. That comparison is between beta startup-delay revisions, not a claim that 2.0 leaves all original firmware behavior unchanged. The [startup evidence](../release/2.0-beta1-125ms/evidence/imported/hardware-startup.json) records the result.

Five reported cold-start cycles showed FinalGROM consistently on the tested PicoPEB setup. A preceding 250 ms trial avoided crashes but still showed PicoPEB. This is an improvement on that reported setup, not a universal timing guarantee or a fix for the separately recorded GROM-tracking hazard.

## Testing status

Reported testing includes prepared 2 MiB games, 128 MiB images, video playback and compilation spot checks. Broader testing is needed. [Troubleshooting](troubleshooting.md#known-playback-issues) holds the current symptom summary, and [Test reports](reference/test-reports.md) retains setup details, uncertainties and comparisons.

Use 32K expansion for titles that require it; CINFRAME and Spaceballs can also run without it. A speech synthesizer is needed to assess speech. Please report your console, expansions, SD card, exact files and affected sequence using the [reporting checklist](troubleshooting.md#report-a-problem).
