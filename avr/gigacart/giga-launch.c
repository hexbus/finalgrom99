/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "giga-launch.h"
#include <string.h>
static WORD u16(const BYTE *p) {return p[0]|(WORD)p[1]<<8;}
static DWORD u32(const BYTE *p) {return u16(p)|(DWORD)u16(p+2)<<16;}
#include "crc32.h"
BYTE giga_launch_load(const GigaAux *file,DWORD image_bytes,GigaLaunch *out,
                      BYTE *buffer,GigaLaunchWrite write) {
    if(!file || !out || !buffer || !write)return GL_FORMAT;
    out->bridge_bytes=0;
    if(giga_aux_read(file,0,buffer))return GL_IO;
    if(memcmp(buffer,"FGLA0001",8) || u32(buffer+8)!=image_bytes ||
       !image_bytes || image_bytes>134217728UL || (image_bytes&8191) ||
       u32(buffer+12)!=file->pages)return GL_FORMAT;
    if((crc(0xffffffffUL,buffer,508)^0xffffffffUL)!=u32(buffer+508))return GL_CRC;
    WORD jobs=u16(buffer+16),bridge=u16(buffer+22);
    DWORD data_first=1+(jobs+31)/32;
    if(!jobs || jobs>512 || data_first>=file->pages || !bridge || bridge>512 ||
       !buffer[19] || buffer[19]>=125 || !buffer[20] || buffer[20]>125-buffer[19] ||
       buffer[21]>15)return GL_FORMAT;
    out->policy=buffer[18];out->first=buffer[19];out->count=buffer[20];out->grom=buffer[21];
    out->runtime=u32(buffer+64);out->bridge_page=0;
    DWORD directory_crc=u32(buffer+24),checked=0xffffffffUL;
    for(DWORD page=1;page<data_first;++page) {
        if(giga_aux_read(file,page,buffer))return GL_IO;
        checked=crc(checked,buffer,512);
    }
    if((checked^0xffffffffUL)!=directory_crc)return GL_CRC;
    BYTE previous_slot=255,next_sector=0;
    for(WORD job=0;job<jobs;++job) {
        if(giga_aux_read(file,1+job/32,buffer))return GL_IO;
        BYTE record[16];memcpy(record,buffer+(job%32)*16,16);
        if((crc(0xffffffffUL,record,12)^0xffffffffUL)!=u32(record+12))return GL_CRC;
        BYTE op=record[0],slot=record[1],sector=record[2],count=record[3];
        DWORD source=u32(record+4),expected=u32(record+8),c=0xffffffffUL;
        BYTE last=job+1==jobs;
        if(op>3 || slot>=128 || sector>=16 || !count || count>16-sector ||
           (op!=3 && !slot && !last) ||
           (last && (op!=1 || slot || sector || count!=1)))return GL_FORMAT;
        if(op!=3 && !last) {
            if(sector && (slot!=previous_slot || sector!=next_sector))return GL_FORMAT;
            previous_slot=slot;next_sector=sector+count;
        }
        if(op==0 || op==3) {
            if(source>=image_bytes/512 || count>image_bytes/512-source)return GL_FORMAT;
        } else if(op==1) {
            if(source<data_first || source>=file->pages || count>file->pages-source)return GL_FORMAT;
        } else if(source>255)return GL_FORMAT;
        for(BYTE i=0;i<count;++i) {
            if(op==0 || op==3) {
                DWORD page=source+i;
                if(giga_read_sector(page/16,page%16,buffer))return GL_IO;
            } else if(op==1) {
                if(giga_aux_read(file,source+i,buffer))return GL_IO;
            } else memset(buffer,(BYTE)source,512);
            c=crc(c,buffer,512);
            if(op!=3 && !last && write(slot,sector+i,buffer))return GL_WRITE;
        }
        if((c^0xffffffffUL)!=expected)return GL_CRC;
        if(last)out->bridge_page=source;
    }
    out->bridge_bytes=bridge;return GL_OK;
}
