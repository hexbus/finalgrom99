/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "giga-overlay.h"
#include "crc32.h"
#include <string.h>
static WORD u16(const BYTE *p) {return p[0]|(WORD)p[1]<<8;}
static DWORD u32(const BYTE *p) {return u16(p)|(DWORD)u16(p+2)<<16;}
static DWORD checksum(const BYTE *p,WORD n) {return crc(0xffffffffUL,p,n)^0xffffffffUL;}
static BYTE checked(const GigaAux *f,DWORD page,BYTE *p) {
    if(giga_aux_read(f,page,p))return GO_IO;
    return checksum(p,508)==u32(p+508)?GO_OK:GO_CRC;
}
BYTE giga_overlay_open(GigaOverlay *o,const GigaAux *f,DWORD size,BYTE *p) {
    if(!o)return GO_FORMAT;
    o->file=0;o->bank=65535;o->base=0;o->mask=0;o->banks=0;
    if(!f || !p || !size || size>134217728UL || (size&8191))return GO_FORMAT;
    BYTE r=checked(f,0,p);if(r)return r;
    if(memcmp(p,"FGOV0001",8) || u32(p+8)!=size || u32(p+12)!=f->pages ||
       u16(p+16)!=size/8192 || f->pages<1+(size/8192+30)/31)return GO_FORMAT;
    o->file=f;o->banks=size/8192;return GO_OK;
}
BYTE giga_overlay_read(GigaOverlay *o,WORD bank,BYTE sector,BYTE *p) {
    if(!o || !o->file || !p || bank>=o->banks || sector>=16)return GO_FORMAT;
    BYTE r;
    if(o->bank!=bank) {
        DWORD page=1+bank/31;
        if((r=checked(o->file,page,p)))return r;
        if(u32(p)!=page)return GO_FORMAT;
        BYTE *entry=p+4+(bank%31)*16;
        WORD mask=u16(entry+2);DWORD base=u32(entry+4);
        BYTE count=0;for(WORD bits=mask;bits;bits>>=1)count+=bits&1;
        if(u16(entry)!=bank || u32(entry+8) || u32(entry+12) ||
           (!mask && base) || (mask && (base<1UL+(o->banks+30)/31 ||
           base>=o->file->pages || (DWORD)count*2>o->file->pages-base)))return GO_FORMAT;
        o->bank=bank;o->mask=mask;o->base=base;
    }
    if(!(o->mask & ((WORD)1<<sector)))return giga_read_sector(bank,sector,p)?GO_IO:GO_OK;
    BYTE ordinal=0;WORD prior=o->mask & (((WORD)1<<sector)-1);
    while(prior) {ordinal+=prior&1;prior>>=1;}
    DWORD page=o->base+(WORD)ordinal*2;
    if((r=checked(o->file,page,p)))return r;
    if(memcmp(p,"FGOP0001",8) || u16(p+8)!=bank || p[10]!=sector || p[11])return GO_FORMAT;
    DWORD original=u32(p+12),replacement=u32(p+16);
    if(giga_read_sector(bank,sector,p))return GO_IO;
    if(checksum(p,512)!=original)return GO_IMAGE;
    if(giga_aux_read(o->file,page+1,p))return GO_IO;
    if(checksum(p,512)!=replacement)return GO_CRC;
    return GO_OK;
}
