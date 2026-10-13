# FinalGROM 99 2.0 beta: changes and known issues

**Larger-cartridge support is a central feature of FinalGROM 2.0.** It retains ordinary ROM/GROM loading and adds a prepared-cartridge path for selected images larger than the board's 1 MiB of SRAM, including 128 MiB images. It uses the existing board and SD card; no extra RAM board or console wiring is required.

## Larger cartridges on the existing 1 MiB board

There are two ways to run a larger cartridge. Sizes below use binary units: 512 KiB and 1 MiB are the cartridge sizes commonly called 512 KB and 1 MB.

| Cartridge size | How it runs | What the user supplies |
| --- | --- | --- |
| Above 512 KiB, up to 1 MiB | Supported ROM-only images fit entirely in SRAM and use the ordinary banked-ROM path. Size alone does not require streaming. | The ordinary image files for a supported banking scheme. |
| Above 1 MiB, up to the current 128 MiB prepared-image limit | A cartridge-specific adaptation keeps needed banks resident and loads others from SD into reusable SRAM slots. | A complete prepared GCR/GCS set with matching companions. |

The original FinalGROM already advertised support up to 1 MiB: 512 KiB was not its documented capacity limit. Support above 1 MiB through preparation is the new capacity extension. Ordinary loading has also changed to use explicit SRAM-bank writes and reject read errors or images that exceed the available region; these changes do not establish universal compatibility above 512 KiB. Mixed ROM/GROM images reserve SRAM for GROM and have less room for ROM than the ROM-only maximum.

Preparation changes the cartridge's bank requests so it can wait safely outside the bank being replaced. FinalGROM loads the required bank into SRAM, applies its prepared patches and lets the program continue. Optional predictions load likely next banks early to reduce waiting. The whole large image stays on SD; it does not have to fit in SRAM at once.

**This supports selected adapted cartridges, not every large ROM automatically.** Preparation must match the exact image and banking behavior. Players can copy an already-prepared set; they do not need to run a development pipeline. Loading pauses can remain, so this is not a guarantee of native-ROM timing. The [preparation guide](cartridge-preparation.md) explains what to copy, the built-in adapters, and requirements for additional ROM revisions.

## Startup delay: 125 ms

The Beta 1 baseline waits 125 ms before initializing the SD card to improve startup behavior on the tested PicoPEB setup. Five reported cold shutdown/start cycles consistently showed FinalGROM in the PicoPEB-connected test. The preceding 250 ms trial avoided crashes but still showed PicoPEB.

Only the AVR startup wait changes, from the original 5 ms to 125 ms before SD initialization. SD/menu loading adds its own time; 125 ms is not the total time until the cartridge is ready. This delay repeats on AVR restart, not on each bank request. The CPLD, menu artwork, cartridge files and formats, Mini Memory and stock 1.3 restore files are unchanged. If the matched 125 ms firmware is already installed, no startup-delay update is needed. When upgrading, use the matched AVR, CPLD and menu identified by the [release manifest](../release/2.0-beta1-125ms/provenance.json).

This is a verified improvement on one reported setup, not a universal timing guarantee or a repair of the separate GROM-tracking hazard. Please report cold starts and reset recovery with your own console, expansions and SD card.

## Added or retained

- Prepared large-cartridge support, including tested 2 MiB titles and 128 MiB Gigacart images. This does not make every large BIN automatically compatible.
- Companion startup/patch files, optional prediction files and an SD location index created on the destination card.
- The supplied /4 and JB/QI compatibility changes, with separate AVR and CPLD parts.
- Ordinary ROM, banked ROM, GROM and ROM/GROM pairs; folders; multiple menu choices; supported autostart; ordinary single-image automatic launch; and reset/menu buttons.
- Mini Memory's 4 KiB cartridge RAM. Standard Mini Memory and S MINIMEM have user-reported short BASIC SAVE/OLD passes. RAM contents are not saved to SD across power-off.
- Restored color stripe with BETA, separate credits and a 2.0 logo.

## Removed, deferred or outside scope

- The larger general-purpose banked-RAM mode, writable GRAM and combined RAM/GRAM mode were omitted for space and lack of demonstrated use in the selected set. Mini Memory's limited RAM mode is retained.
- Per-cartridge help and the original program-controlled reload/dump facilities are omitted. Physical reset/menu operation remains available.
- Persistent Mini Memory SD saving, the supplied larger-RAM Mini Memory Plus variant, MBX, SuperSpace banking and prepared full-GROM GCG images are not supported.
- There is no general automatic adapter for arbitrary new large ROMs.

## Known limits and requested testing

Reported tests include working Cinema controls/playback, Spaceballs, Games compilation spot checks and improved patched Stealth speech. Broader testing is needed; this is not an all-games or all-hardware guarantee.

[Lapo Serafini's PAL report](beta-feedback.md#lapo-serafini-pal-console-report) records successful play, compilation spot checks and compatibility improvements, alongside opening-speech corruption, sprite glitches, one load freeze and audio concerns. Build identities and causes remain unconfirmed.

BABUFFER vocals still have synchronization/garbling issues. STLSRV2 speech has improved but needs more testing. Music-only Bad Apple has completed a run. CINFRAME silences unused sound channels each frame; another long run is useful after the earlier version's eight-hour stray-tone report. Games compilations need results for individual games.

Use 32K expansion when testing titles that require it; Cinema and Spaceballs can also run without it. A speech synthesizer is needed to assess speech. Report PAL/NTSC, video-chip changes, expansions and SD card details. Other configurations need their own evidence.

Dragon's Lair is included in the [cartridge examples](beta-cartridge-recipes.md). See [playback feedback](beta-feedback.md#dragons-lair-playback-reports) for reported results.
