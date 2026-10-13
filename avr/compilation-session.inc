/* SPDX-License-Identifier: GPL-3.0-or-later
 * Original compilation menus cooperate through an external RAM helper.
 * CIX still supplies each selected entry's resident banks and checksums. */

#include "offline-sector.inc"
static void compilation_root(uint8_t kind) {
    const char *name=kind==2?"GAMES1M":"GAMES2M";
    for(uint8_t i=0;i<8;++i)writeByte(name[i]);
    writeByte(0xff);writeByte(SEL_COMPILATION);
    writeByte(FG99_RESTART>>8);writeByte(FG99_RESTART&255);
    for(uint8_t i=0;i<ENTRY_NAME_LEN;++i)writeByte(i<7?name[i]:' ');
    has_giga=1;
}
void prepareCompilation(void) {
    led(1);writeBegin();buildPath("BIN");
    uint8_t error=profile_session();
    giga_format_error(giga_status,PSTR("GAMES STARTUP ERROR "),error);
}
