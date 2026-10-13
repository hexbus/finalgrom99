# FinalGROM 99

The *TI 99/4A FinalGROM Cartridge*, or FinalGROM 99 for short, is a
cartridge for the TI 99/4A home computer that allows you to run ROM and GROM
cartridge images from an SD card.  It succeeds the [FlashROM 99][3] released
in 2016.

![The FinalGROM 99 Cartridge](doc/finalgrom99.jpg)

The FinalGROM 99 supports ROM images, GROM images, and mixed images of up to
1 MB in size that use the write-to-ROM bank switching scheme.  The cartridge
does not require the Peripheral Expansion Box and runs on both PAL and NTSC
consoles, including modified consoles with an F18A.  It will also run on
v2.2 consoles and enables those to run ROM-only programs.

The 2.0 beta retains Mini Memory's 4 KiB cartridge RAM.  The original
larger RAM/GRAM modes and program-controlled reload/dump facilities are not
included in this beta; see [release notes](docs/release-notes.md) for the
full feature list and limitations.  Firmware can be updated by SD card.

The FinalGROM 99 is released as Open Source Hardware under the
[CERN OHL license][5] and the [GNU GPL license][6].  Both hardware design
files and firmware sources are available on [GitHub][2].

The project homepage with detailed instructions is located at [GitHub][1].


FinalGROM 2.0 beta
-----------------

This repository contains the **2.0 Beta 1 firmware with a 125 ms startup
delay**, based on the original FinalGROM 99 project.  It adds support for
selected prepared cartridges larger than the board's 1 MiB of SRAM,
including 128 MiB images, by loading banks from SD as needed.  Ordinary
supported ROM/GROM images continue to use the normal loading path.

**Cartridge images are user-supplied**, except for the separate
[complete Cinema example download](docs/prepared-cartridges.md#copy-a-complete-set).
The Git source tree provides firmware, tools, templates and companion files;
full game and video images are not stored in it.

Large cartridges require an adaptation for the exact image revision;
renaming an arbitrary ROM is not sufficient.  Keep the SD card inserted
while a prepared cartridge is running.  This is beta firmware, with known
limitations and further compatibility testing needed.

**New to 2.0? Start with the [documentation guide](docs/README.md).**

- [Install and play](docs/getting-started.md): update an existing board and run a cartridge.
- [Set up a large cartridge](docs/prepared-cartridges.md): copy a prepared set or attach companions to a supported image.
- [Troubleshooting](docs/troubleshooting.md) and [Beta 1 limitations](docs/release-notes.md).
- [Developer guides](docs/README.md#for-developers): adapters, firmware builds, architecture and formats.


Using the FinalGROM 99
----------------------

Using the FinalGROM 99 is simple and doesn't require any special hardware or
software.

To begin, copy some cartridge dumps or other images files onto an SD or SDHC
card.  You can create directories to organize your programs.  Switch off the
TI 99 and plug in the FinalGROM 99, then insert the SD card into the
FinalGROM 99.  Switch on the TI 99 and wait until the activity indicator on
the FinalGROM 99 is no longer lit.

Press any key to bring up the TI menu screen.  You should see the `FINALGROM
99` entry.  Select it to start the image browser, where you can page through
the list of available image entries with `,`, `.`, `SPACE`, and number keys.
When you descend into a folder, the FinalGROM will reload its contents from
the SD card.  To go back up to the parent folder, select the special folder
`/ ..`.

![Image selection](doc/selection.png)

Select the image you want to run.  The screen will show a loader animation
while the image loads.  Once the image has been loaded, it will start
automatically.  For ordinary images loaded entirely into SRAM, the SD card
is no longer required and may be removed.  Prepared large cartridges keep
loading banks during execution, so their SD card must remain inserted.

If only one image is found on the SD card, it is loaded immediately without
the need to select it first.  This method ensures maximum compatibility.
Note that ROM images loaded as single-image SD card will not run on v2.2
consoles!

If you want to run a different program from the SD card, reset the TI 99 by
pressing `FCTN-=` and then reset the FinalGROM 99 by pushing the reset
button.  If you do not reset the FinalGROM 99, the TI menu will show only
the previously selected image.  Alternatively, you can power cycle the
console, which will reset both TI 99 and FinalGROM 99.

For detailed usage information please refer to the [FinalGROM 99 homepage][1].


Building the FinalGROM 99
-------------------------

The [GitHub repository][2] contains all hardware design files and software
sources required to build the FinalGROM 99.

Building the FinalGROM 99 is slightly more involved than building its
predecessor [FlashROM 99][3], as the FinalGROM 99 uses surface-mounted
devices (SMD), of which two are programmable.  For starters, this
[video tutorial][12] shows how to solder SMD ICs by hand.


### Hardware

To build the FinalGROM 99, you need the following components:

- 1x FinalGROM 99 PCB
- 1x Xilinx XC95144XL-10TQ100 (TQFP-100)
- 1x Atmel ATmega 328P (TQPF-32)
- 1x Cypress SRAM CY62158EV30LL-45ZSXI (TSOP-44)
- 1x On Semiconductor Voltage Converter NCP1117ST33T3G (SOT-223)
- 12x 100 nF ceramic capacitors (SMD 0603)
- 2x 10 uF tantalum capacitors (SMD 0603)
- 2x 330 Ohm resistor (SMD 0603)
- 2x 10 kOhm resistor (SMD 0603)
- 1x LED 3-5 mm (through-hole)
- 2x micro push button (through-hole)
- 1x 2x3 socket (through-hole)
- 1x SD socket or Arduino SD shield

The FinalGROM 99 board was designed with [KiCad][7], a cross platform and
open source electronics design automation suite.  The design files are
stored in `pcb/` in the [repository][2].

The size of the 330 Ohm resistor depends on the color and brightness of your
LED.  For bright, red LEDs, 330 Ohm is a suitable value.

Both LED and push buttons are through-hole components, so that they can be
mounted on a cartridge shell and connected by wires to the board.

![Parts Placement](doc/placement.jpg)

For the SD socket, you need a socket that has two little nubs at the bottom
of the socket.  Your best source for these is eBay, but check the pictures
before you buy.  Alternatively, you may use an Arduino SD shield that is
readily available online.  Make sure to pick a module that matches the
pin-out of the reference module shown below.  Note that the 5V input is not
used, so potential substitutes must be powered by 3.3V.

![SD socket](doc/sdsocket.jpg)

With some work, the FinalGROM 99 can be mounted in a standard TI 99
cartridge shell.

![Mounted in Cartridge Shell](doc/cartshell.jpg)

Please note that I don't offer carts, boards, or parts myself.


### Software

The firmware for the ATmega 328P consists of two components.  The `avr/`
folder contains the microcontroller logic for accessing the SD card,
generating the image browser, loading the selected image, and controlling
the CPLD.  The included bootloader enables firmware updates from an SD card.

The `ti/` folder contains TI 99 assembly sources for the image browser,
GPL wrapper and prepared-cartridge startup helpers.  The menu machine code
is loaded into RAM during operation; cartridge startup helpers are included
in the prepared cartridge's companion files.  The [TI source index](ti/index.md)
identifies original, modified and added programs and their build roles.

Build the 2.0 application from the repository root:

    python tools/build-firmware.py --verify-release --xdt99 C:/tools/xdt99

Replace the example path with the [xdt Suite][4] directory containing
`xas99.py`.  The [build guide](docs/development/building.md) lists compiler requirements,
output files and verification steps.  The current AVR Makefile delegates
to this builder.  The original `ti/make.py` and application/bootloader
packaging sequence is not the 2.0 release build procedure.

The application builder does not install a bootloader or program a new
board.  Initial board programming requires a suitable bootloader and the
ATmega 328P's 8 MHz clock configuration; retain the original hardware setup
instructions on the [project homepage][1].  This beta uses the existing
board and clock frequency.

For an existing programmed board, the AVR bootloader accepts `UPDATE.AVR`
from SD without replacing itself.  Use the matched AVR, CPLD and
`FGMENU.BIN` identified by the
[release manifest](release/2.0-beta1-125ms/provenance.json).

The 2.0 CPLD sources and constraints are in `vhdl/gigacart/`; the original
basic CPLD sources remain available for reference.  Generating a new CPLD
fit requires Xilinx [ISE 14.7][9].  Follow the [CPLD build notes](docs/development/building.md#cpld-build-and-verification)
for the correct device and source set.  The AVR's XSVF player programs the
CPLD from an `UPDATE.PLD` file on SD without a separate JTAG cable.


About the Project
-----------------

The TI 99/4A FinalGROM Cartridge is Open Source Hardware released under the
[CERN OHL license][5], in the hope that TI 99 enthusiasts may find it
useful.  Software components are released under the [GNU GPL license][6].

The microcontroller code uses a modified version of the [Petit FatFs][10]
library and an adapted version of the Xilinx [Application Note 58][11] code.

Heartfelt thanks go to Erik Spiehl (speccery) for helping me analyze an
electrical problem on the development boards, and to Jim Brain (Brain) for
showing me how to replace the Arduino shields by plain SD sockets.

Contributions to both hardware and software are very welcome!  Please email
feedback, support questions, inquiries for parts, and bug reports to the
developer at <r@0x01.de>.


[1]: https://endlos99.github.io/finalgrom99
[2]: https://github.com/endlos99/finalgrom99
[3]: https://endlos99.github.io/flashrom99
[4]: https://endlos99.github.io/xdt99
[5]: http://www.ohwr.org/projects/cernohl/wiki
[6]: http://www.gnu.org/licenses/gpl.html
[7]: http://kicad.org
[8]: http://www.nongnu.org/avrdude/
[9]: https://www.xilinx.com/products/design-tools/ise-design-suite/ise-webpack.html
[10]: http://elm-chan.org/fsw/ff/00index_p.html
[11]: http://www.xilinx.com/support/documentation/application_notes/xapp058.pdf
[12]: https://www.youtube.com/watch?v=6PB0u8irn-4
