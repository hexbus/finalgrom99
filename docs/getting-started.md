# Install Beta 1 and play

[Start here](README.md) Ã‚Â· [Large cartridges](prepared-cartridges.md) Ã‚Â· [Troubleshooting](troubleshooting.md)

This guide is for an existing, working FinalGROM 99. You need a computer with an SD reader, a supported FAT32 SD card, and your user-supplied cartridge images. Full game and video images are not included in this repository. You do not need Python, an assembler, or a hardware programmer.

Read the [Beta 1 capabilities and limitations](release-notes.md) before upgrading: some original FinalGROM features are not included. Building or initially programming a new board is a separate [developer task](development/building.md).

## 1. Get the three matching release files

Use these files together from [release/2.0-beta1-125ms](../release/2.0-beta1-125ms/):

| File | What it updates |
| --- | --- |
| `UPDATE.AVR` | The microcontroller that reads the SD card and loads cartridges |
| `UPDATE.PLD` | The programmable logic that connects the TI to cartridge memory |
| `FGMENU.BIN` | The FinalGROM menu |

Download the actual files, rather than saving a GitHub web page with one of these names. The [release manifest](../release/2.0-beta1-125ms/provenance.json) records their SHA-256 checksums for an exact comparison.

If both devices already have this matched release installed, skip to step 4. Seeing the Beta menu alone does not confirm both firmware updates were applied.

## 2. Update the AVR

Back up your SD card first. Use one update file at a time so it is clear which device is being updated.

1. Switch off the TI. On the computer, put `UPDATE.AVR` in the SD card's root directory. Make sure `UPDATE.PLD` is not also present there.
2. Insert the card into FinalGROM and turn on the TI. The update starts automatically; you do not select it from the menu.
3. Wait for the activity LED to finish and stay off. **Do not interrupt power while programming.** Repeating flashes indicate an error; see [update problems](troubleshooting.md#firmware-update-problems).
4. After successful completion, remove the SD card before resetting or restarting FinalGROM. Switch off the TI and remove `UPDATE.AVR` from the card using the computer. Leaving it there would run the update again.

The original update procedure describes an AVR update as taking a few seconds. Use the LED result, not an assumed deadline.

## 3. Update the CPLD

1. With the TI off, copy `UPDATE.PLD` to the SD root. Check that `UPDATE.AVR` has been removed.
2. Insert the card and turn on the TI. The update starts automatically. The display may freeze during this operation.
3. Wait for successful completion with the LED staying off. The original procedure gives about 15 seconds for the CPLD update; repeating flashes are an error.
4. Remove the card before resetting or restarting FinalGROM. Switch off the TI, then remove `UPDATE.PLD` from the card on the computer.

These steps follow the [original update procedure](../doc/MANUAL.md#how-to-update-the-finalgrom-99) and the included [AVR updater](../avr/boot/bootload.c) and [CPLD updater](../avr/loader.c). They describe an existing programmed board, not bootloader installation.

## 4. Copy the menu and cartridge files

Put `FGMENU.BIN` in the SD root. Neither `UPDATE.AVR` nor `UPDATE.PLD` should remain there for normal use.

- **Ordinary supported cartridge:** copy its ROM/GROM image files, keeping any matching pair together. Folders are supported.
- **Complete prepared large cartridge:** copy the image and all its supplied companion files together. Follow the [Dragon's Lair folder example](prepared-cartridges.md#copy-a-complete-set).
- **Only a large image, without companions:** use the [supported-image walkthrough](prepared-cartridges.md#assemble-a-set-from-a-supported-image). An unknown image needs an adapter before it can run this way.

The complete file-role table is in [Which files go on the card?](prepared-cartridges.md#which-files-go-on-the-card).

## 5. Start the cartridge

1. With the TI off, insert FinalGROM and the SD card, then turn on the TI.
2. Wait until FinalGROM's activity LED is no longer busy. Open the TI menu and select `FINALGROM 99`.
3. Choose the cartridge image. Use the on-screen choices to move through folders and pages.

An ordinary single-image card may start its image automatically. A prepared image can take longer on its first selection while FinalGROM builds its card-location index. There is no separate Prepare button.

**Expected result:** the selected program starts. Keep the SD card inserted for prepared cartridges, because more banks may be needed during play. For ordinary images already loaded entirely into SRAM, the card is no longer needed.

To choose another cartridge, return the TI to its menu with `FCTN-=` and press FinalGROM's reset button, or power-cycle the console. If loading fails or playback pauses heavily, use [Troubleshooting](troubleshooting.md).
