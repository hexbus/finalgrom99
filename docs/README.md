# FinalGROM 2.0: start here

FinalGROM runs cartridge images from an SD card. Version 2.0 adds a way to run selected cartridges larger than the board's 1 MB of memory, using the existing board.

For an ordinary supported cartridge that fits in memory, you still copy its image files to the SD card and select it from the menu. A larger cartridge needs a **prepared set**: its image and matching helper files. FinalGROM loads parts of that image as the program needs them. Keep the SD card inserted while it runs; loading pauses can occur.

## Choose your next step

| I want to… | Read this |
| --- | --- |
| Install Beta 1 and play a cartridge | [Install and play](getting-started.md) |
| Use a large cartridge, or add missing helper files to a supported image | [Set up a large cartridge](prepared-cartridges.md) |
| Fix a loading problem or report stuttering | [Troubleshooting](troubleshooting.md) |
| Check what this beta supports or omits | [Beta 1 capabilities and limitations](release-notes.md) |

**You do not need to build firmware or edit a recipe to use a complete prepared set.** Read the installation guide, copy the supplied files, and select the image.

## What is included?

**Cartridge images are user-supplied.** The repository includes matched firmware and menu files, source code, tools, and seven adapter examples with their companion files. It does not include full game or video cartridge images. You must supply the exact image required by an example or obtain a complete prepared set separately.

The included catalog recognizes specific prepared image versions. It cannot automatically adapt a different ROM just because the title or filename matches.

## For developers

| I want to… | Read this |
| --- | --- |
| Rebuild an example or adapt a different cartridge | [Cartridge adapters](development/adapters.md) |
| Compile and verify the firmware | [Build the firmware](development/building.md) |
| Understand the AVR, CPLD, SRAM and bank requests | [How it works](development/architecture.md) |
| Look up JSON fields or binary layouts | [File formats](reference/file-formats.md) |
| Examine reported hardware results and open questions | [Test reports](reference/test-reports.md) |

The source indexes identify individual files: [AVR](../avr/index.md), [TI assembly](../ti/index.md), and [computer tools](../tools/index.md).

## A few useful words

- **Image / ROM:** a file containing a cartridge's program and data.
- **Bank:** a small piece of that image. The prepared-cartridge tools use 8 KB banks.
- **SRAM:** the cartridge's working memory, which the TI reads while running.
- **Prepared set:** an adapted image and its matching helper, or *companion*, files.
- **Adapter:** cartridge-specific instructions and code that make a particular image work with bank loading.
- **Recipe:** a developer's instructions for building companion files. It stays on the computer.

Technical pages use KiB and MiB for exact binary sizes: 1 KiB is 1,024 bytes and 1 MiB is 1,048,576 bytes. These are the sizes commonly called KB and MB here.
