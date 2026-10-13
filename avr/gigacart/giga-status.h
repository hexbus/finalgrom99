/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef FG_GIGA_STATUS_H
#define FG_GIGA_STATUS_H
#include <stdint.h>
#ifdef __AVR__
#include <avr/pgmspace.h>
#define FG_STATUS_READ(p) pgm_read_byte(p)
#else
#define FG_STATUS_READ(p) (*(const uint8_t *)(p))
#endif
/* 31-byte menu field. Prefix is in program memory on AVR, includes any
 * separator, and is limited to 27 characters to reserve three digits + NUL. */
static void giga_format_error(char *dest,const char *prefix,uint8_t code) {
    uint8_t n=0,c;
    while(n<27 && (c=FG_STATUS_READ(prefix+n))) {dest[n]=(char)c;++n;}
    if(code>=100) {dest[n++]='0'+code/100;code%=100;dest[n++]='0'+code/10;}
    else if(code>=10)dest[n++]='0'+code/10;
    dest[n++]='0'+code%10;dest[n]=0;
}
#undef FG_STATUS_READ
#endif
