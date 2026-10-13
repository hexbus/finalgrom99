/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "giga-aux.h"
#include "lib/diskio.h"

BYTE giga_aux_open(FATFS *fs,const char *path,GigaAux *out,BYTE *scratch) {
    if(!out)return GI_FORMAT;
    out->count=0;out->pages=0;
    if(!fs || !path || !scratch || (fs->fs_type!=2 && fs->fs_type!=3) ||
       !fs->csize || fs->csize>128 || (fs->csize&(fs->csize-1)) ||
       fs->n_fatent<3)return GI_FORMAT;
    BYTE width=fs->fs_type==3?4:2;
    DWORD limit=width==4?0x0ffffff0UL:0xfff0UL;
    if(fs->n_fatent>limit ||
       fs->n_fatent-2>(0xffffffffUL-fs->database)/fs->csize ||
       fs->fatbase>0xffffffffUL-(fs->n_fatent-1)/(512/width))return GI_FORMAT;
    if(pf_open(path)!=FR_OK)return GI_IO;
    if(!fs->fsize || (fs->fsize&511))return GI_FORMAT;
    DWORD pages=fs->fsize/512,remaining=pages,cluster=fs->org_clust;
    BYTE runs=0;
    DWORD cached=0xffffffffUL;
    while(remaining) {
        if(cluster<2 || cluster>=fs->n_fatent)return GI_CHAIN;
        DWORD first=fs->database+(cluster-2)*fs->csize;
        DWORD take=remaining<fs->csize?remaining:fs->csize;
        if(runs && first==out->extent[runs-1].first+out->extent[runs-1].count)
            out->extent[runs-1].count+=take;
        else {
            if(runs==GA_EXTENTS)return GI_SPACE;
            out->extent[runs].first=first;out->extent[runs++].count=take;
        }
        /* Reuse the caller's sector buffer during preparation. Adjacent
         * clusters share one FAT sector; do not reread it per entry. */
        DWORD fat_sector=fs->fatbase+cluster/(512/width);
        if(fat_sector!=cached) {
            if(disk_readp(scratch,fat_sector,0,512))return GI_IO;
            cached=fat_sector;
        }
        WORD offset=(cluster%(512/width))*width;
        DWORD next=scratch[offset]|((DWORD)scratch[offset+1]<<8);
        if(width==4)next|=((DWORD)scratch[offset+2]<<16)|((DWORD)(scratch[offset+3]&15)<<24);
        remaining-=take;
        if(!remaining) {
            if(next<limit+8)return GI_CHAIN;
        } else if(next<2 || next>=fs->n_fatent)return GI_CHAIN;
        cluster=next;
    }
    out->pages=pages;out->count=runs;
    return GI_OK;
}
BYTE giga_aux_read(const GigaAux *map,DWORD page,BYTE *buffer) {
    if(!map || !buffer || !map->count || map->count>GA_EXTENTS || page>=map->pages)
        return GI_FORMAT;
    for(BYTE i=0;i<map->count;++i) {
        if(page<map->extent[i].count)
            return disk_readp(buffer,map->extent[i].first+page,0,512)?GI_IO:GI_OK;
        page-=map->extent[i].count;
    }
    return GI_CHAIN;
}
#ifdef __AVR__
typedef char aux_size_check[sizeof(GigaAux)==29?1:-1];
#endif
