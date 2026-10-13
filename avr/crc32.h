/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef FG_CRC32_H
#define FG_CRC32_H
#ifdef __AVR__
#include <avr/pgmspace.h>
#define CRC_READ(p) pgm_read_dword(p)
#else
#ifndef PROGMEM
#define PROGMEM
#endif
#define CRC_READ(p) (*(p))
#endif
#ifdef FG_SHARED_CRC
DWORD crc(DWORD,const BYTE *,WORD);
#else
static const DWORD crc_nibble[16] PROGMEM={
0x00000000UL,0x1db71064UL,0x3b6e20c8UL,0x26d930acUL,0x76dc4190UL,0x6b6b51f4UL,0x4db26158UL,0x5005713cUL,0xedb88320UL,0xf00f9344UL,0xd6d6a3e8UL,0xcb61b38cUL,0x9b64c2b0UL,0x86d3d2d4UL,0xa00ae278UL,0xbdbdf21cUL};
static DWORD crc(DWORD c,const BYTE *p,WORD n) {
    while(n--) {
        c^=*p++;
        for(BYTE half=0;half<2;++half)c=(c>>4)^CRC_READ(crc_nibble+(c&15));
    }
    return c;
}
#endif
#endif
