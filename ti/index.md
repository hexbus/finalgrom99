# TI assembly source index

This directory contains **five original assembly sources and two later additions**, plus a build helper and two generated binaries. They provide the TI-side menu and current cartridge startup helpers.

Start with `menu.a99` for the menu, or `player-boot-mailbox.a99` and `dragon-choice.a99` for the supplied cartridge adapters.

## How origin is classified

The reference is upstream FinalGROM commit `26c223f6b9321d97356ffcd9719d2665f1070237`. **Original, unchanged** matches its text, ignoring line endings. **Original, modified** has later changes. **Added** is absent from that commit; it does not imply sole authorship.

Original headers credit Ralph Benzinger. The menu also credits Jon Guidry with Lee Stewart for /4 changes and Jochen Buur for QI wait changes. See file headers for credits and licenses.

## Original assembly sources

| File | Origin | Purpose and 2.0 status |
| --- | --- | --- |
| [menu.a99](menu.a99) | Original, modified | Menu and launch code. Changed artwork/credits, wait handling and xdt99 include filenames. |
| [browser.a99](browser.a99) | Original, modified | Included by `menu.a99`. Changed status/detail display, page display and /4 key scanning. |
| [sort.a99](sort.a99) | Original, unchanged | Image-list sorting, included by `browser.a99`. |
| [grom.gpl](grom.gpl) | Original, unchanged | Small GPL wrapper transferring control to the menu's ROM entry. |
| [help.a99](help.a99) | Original, modified | Help viewer; only xdt99 include suffixes changed (`.a99` to `.asm`). Per-cartridge help is omitted from 2.0. |

`.a99` files use xdt99's TMS9900 assembler, `xas99.py`; `.gpl` uses its GPL assembler, `xga99.py`. Shared VDP routines come from the xdt99 library.

## Build helper and generated files

| File | Origin | Role |
| --- | --- | --- |
| [make.py](make.py) | Original, unchanged | Historical menu/GPL/help builder with author-specific paths. Use the maintained builders below. |
| [menu.bin](menu.bin) | Added generated file | 2,310-byte menu binary matching the frozen `FGMENU.BIN`. |
| [grom.gbc](grom.gbc) | Added generated file | 35-byte assembled output of the original `grom.gpl`. The wrapper source itself is original. |

The [release source map](../release/2.0-beta1-125ms/source-map.json) records baseline inputs. Inclusion in it does not mean a file is active or reassembled on every build.

## Added cartridge startup helpers

| File | Purpose | Current use |
| --- | --- | --- |
| [player-boot-mailbox.a99](player-boot-mailbox.a99) | Restores scratchpad state and waits in RAM for the starting bank; optional music/speech chooser. | BABUFFER, CINFRAME, GAMES1M, GAMES2M, SPACE and STLSRV2 bridges. |
| [dragon-choice.a99](dragon-choice.a99) | Dragon's Lair chooser and startup bridge. | DRAGON uses `MAILBOX=1`. The default non-mailbox variant is an earlier path. |

Current bridges become `bridge.bin`, embedded in the cartridge's GLP companion. Players do not copy them separately. The [bridge guide](../docs/beta-cartridge-recipes.md#what-is-bridgebin-and-where-did-it-come-from) explains their operation; [bridge-builds.json](../templates/gigacart/beta1/bridge-builds.json) defines sources and parameters.

## Build and verification entry points

- **AVR and menu:** follow the [build guide](../docs/building.md#verify-the-baseline). Include `--xdt99` to reassemble and verify the menu.
- **Seven bridges:** the [bridge rebuild command](../docs/beta-cartridge-recipes.md#what-is-bridgebin-and-where-did-it-come-from) checks outputs against supplied binaries and recipe hashes.

Update this index when adding or changing a TI program; keep commands and parameters in the linked guides.
