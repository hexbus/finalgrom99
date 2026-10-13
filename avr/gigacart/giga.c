/* SPDX-License-Identifier: GPL-3.0-or-later
 * Persistent extent index for the preparation prototype. The complete image
 * allocation is compared on every reuse; filename/size alone are not trusted.
 * Only FAT16/FAT32 with 512-byte sectors and mirrored FATs are writable here.
 */
#include <string.h>
#include "giga.h"
#include "lib/diskio.h"

GigaReport giga_report;
static struct {
    FATFS *fs;
    BYTE *fat, *page;
    DWORD fat_page, fat_size, boot, info, backup_info;
    DWORD image_first, image_size, image_clusters;
    DWORD index_first, index_sector, index_bytes, index_clusters;
    DWORD dir_sector, runs, cached_runs, stream_page;
    WORD dir_offset;
    BYTE fats, width, type, valid, compare, writing, readonly, created, map_ram;
    BYTE name[11];
} g;

static WORD u16(const BYTE *p) { return (WORD)p[0] | (WORD)p[1]<<8; }
static DWORD u32(const BYTE *p) { return (DWORD)u16(p) | (DWORD)u16(p+2)<<16; }
static void w16(BYTE *p, WORD v) { p[0]=v; p[1]=v>>8; }
static void w32(BYTE *p, DWORD v) { w16(p,v); w16(p+2,v>>16); }
static BYTE read_sector(DWORD s, BYTE *p) { return disk_readp(p,s,0,512) ? GI_IO : GI_OK; }
static BYTE write_sector(DWORD s, const BYTE *p) {
    if (disk_writep(0,s) || disk_writep(p,512) || disk_writep(0,0)) return GI_IO;
    return GI_OK;
}
static DWORD cluster_sector(DWORD c) { return g.fs->database+(c-2)*g.fs->csize; }
static BYTE cluster_ok(DWORD c) { return c>=2 && c<g.fs->n_fatent; }
static BYTE eoc(DWORD c) { return c >= (g.width==4 ? 0x0ffffff8UL : 0xfff8UL); }
static BYTE fat_get(DWORD c, DWORD *next) {
    if (!cluster_ok(c)) return GI_CHAIN;
    DWORD p=g.fs->fatbase+(c*g.width)/512;
    if (p!=g.fat_page) {
        if (read_sector(p,g.fat)) return GI_IO;
        g.fat_page=p; ++giga_report.fat_reads;
    }
    WORD off=(WORD)(c*g.width)%512;
    *next=g.width==4 ? u32(g.fat+off)&0x0fffffffUL : u16(g.fat+off);
    return GI_OK;
}
static DWORD header_crc(const BYTE *p) {
    DWORD crc=0xffffffffUL;
    for (BYTE i=0;i<60;++i) {
        crc^=p[i];
        for (BYTE b=0;b<8;++b) crc=(crc>>1)^((crc&1)?0xedb88320UL:0);
    }
    return ~crc;
}
static void header(BYTE complete) {
    memset(g.page,0,512);
    memcpy(g.page,"FGGIDX01",8);
    g.page[8]=complete; g.page[9]=g.type; g.page[10]=g.fs->csize; g.page[11]=g.width;
    w32(g.page+12,g.fs->fatbase); w32(g.page+16,g.fs->database);
    w32(g.page+20,g.fs->n_fatent); w32(g.page+24,g.image_first);
    w32(g.page+28,g.image_size); w32(g.page+32,g.runs);
    w32(g.page+36,g.index_bytes); w32(g.page+40,g.index_first);
    w32(g.page+60,header_crc(g.page));
}
static BYTE geometry(void) {
    if (g.fs->fs_type!=2 && g.fs->fs_type!=3) return GI_FORMAT;
    g.width=g.fs->fs_type==3 ? 4 : 2;
    if (read_sector(0,g.fat)) return GI_IO;
    g.boot=0;
    if (memcmp(g.fat+54,"FA",2) && memcmp(g.fat+82,"FA",2)) {
        g.boot=u32(g.fat+454);
        if (!g.boot || read_sector(g.boot,g.fat)) return GI_FORMAT;
    }
    WORD reserved=u16(g.fat+14);
    g.fats=g.fat[16];
    g.fat_size=g.width==4 ? u32(g.fat+36) : u16(g.fat+22);
    if (u16(g.fat+510)!=0xaa55 || u16(g.fat+11)!=512 ||
        !g.fs->csize || (g.fs->csize & (g.fs->csize-1)) ||
        g.fat[13]!=g.fs->csize || g.fats<1 || g.fats>2 || !g.fat_size ||
        g.boot+reserved!=g.fs->fatbase ||
        (g.width==4 && (u16(g.fat+40)&0x80))) return GI_FORMAT;
    DWORD data=g.fs->fatbase+g.fats*g.fat_size+(g.width==2 ? (g.fs->n_rootdir*32UL+511)/512 : 0);
    if (data!=g.fs->database || g.fs->n_fatent>g.fat_size*(512/g.width)) return GI_FORMAT;
    g.info=g.backup_info=0;
    if (g.width==4) {
        WORD info=u16(g.fat+48), backup=u16(g.fat+50);
        if (info && info<reserved) g.info=g.boot+info;
        if (g.info && backup && backup!=0xffff && (DWORD)backup+info<reserved)
            g.backup_info=g.boot+backup+info;
    }
    g.fat_page=0xffffffffUL;
    return GI_OK;
}
static BYTE invalidate_info(DWORD sector) {
    if (!sector) return GI_OK;
    if (read_sector(sector,g.page)) return GI_IO;
    if (u32(g.page)!=0x41615252UL || u32(g.page+484)!=0x61417272UL ||
        u32(g.page+508)!=0xaa550000UL) return GI_OK; /* no valid hints */
    w32(g.page+488,0xffffffffUL); w32(g.page+492,0xffffffffUL);
    return write_sector(sector,g.page);
}
/* An index is deliberately allocated as one contiguous file. Its capacity
 * covers the maximum 128 MiB image at this card's cluster size, even if every
 * image cluster is a separate extent. This avoids resizing published files. */
static BYTE allocate_index(void) {
    DWORD start=0, run=0, value;
    BYTE r;
    for (DWORD c=2;c<g.fs->n_fatent;++c) {
        if ((r=fat_get(c,&value))) return r;
        if (!value) { if (!run) start=c; if (++run==g.index_clusters) break; }
        else run=0;
    }
    if (run!=g.index_clusters) return GI_SPACE;
    g.index_first=start; g.index_sector=cluster_sector(start);
    DWORD first=start*g.width/512, last=(start+run-1)*g.width/512;
    /* Check all affected mirror sectors before making any FAT changes. */
    for (DWORD s=first;s<=last;++s) {
        if (read_sector(g.fs->fatbase+s,g.fat)) return GI_IO;
        for (BYTE f=1;f<g.fats;++f) {
            if (read_sector(g.fs->fatbase+s+f*g.fat_size,g.page)) return GI_IO;
            if (memcmp(g.fat,g.page,512)) return GI_FORMAT;
        }
    }
    if ((r=invalidate_info(g.info)) || (r=invalidate_info(g.backup_info))) return r;
    for (DWORD s=first;s<=last;++s) {
        if (read_sector(g.fs->fatbase+s,g.fat)) return GI_IO;
        DWORD low=s*(512/g.width), high=low+512/g.width;
        if (low<start) low=start;
        if (high>start+run) high=start+run;
        for (DWORD c=low;c<high;++c) {
            WORD off=(WORD)(c*g.width)%512;
            DWORD next=c==start+run-1 ? 0x0fffffffUL : c+1;
            if (g.width==4) w32(g.fat+off,(u32(g.fat+off)&0xf0000000UL)|next);
            else w16(g.fat+off,(WORD)next);
        }
        for (BYTE f=0;f<g.fats;++f)
            if (write_sector(g.fs->fatbase+s+f*g.fat_size,g.fat)) return GI_IO;
    }
    g.fat_page=0xffffffffUL;
    /* Do not expose old contents of newly allocated free sectors through the
     * preallocated companion file. Clear its entire logical capacity first. */
    memset(g.page,0,512);
    for (DWORD i=0;i<g.index_bytes/512;++i)
        if (write_sector(g.index_sector+i,g.page)) return GI_IO;
    /* Publish an identifiable incomplete header before publishing the entry.
     * Failure before the directory write can leak an allocation, never reuse
     * another file's clusters. A subsequent preparation can rebuild state 0. */
    header(0);
    if (write_sector(g.index_sector,g.page) || read_sector(g.dir_sector,g.page)) return GI_IO;
    BYTE *entry=g.page+g.dir_offset;
    if (entry[0]!=0 && entry[0]!=0xe5) return GI_DIRECTORY;
    BYTE end=!entry[0];
    memset(entry,0,32); memcpy(entry,g.name,11); entry[11]=0x20;
    w16(entry+16,0x21); w16(entry+24,0x21); /* 1980-01-01, no RTC */
    w16(entry+20,g.width==4 ? start>>16 : 0); w16(entry+26,start);
    w32(entry+28,g.index_bytes);
    if (end) entry[32]=0;
    return write_sector(g.dir_sector,g.page);
}
static BYTE emit_run(DWORD first, DWORD count) {
    DWORD page=g.runs/64;
    WORD offset=(WORD)(g.runs%64)*8;
    if (g.writing) {
        if (!offset) memset(g.page,0,512);
        w32(g.page+offset,first); w32(g.page+offset+4,count);
        if (offset==504 && write_sector(g.index_sector+1+page,g.page)) return GI_IO;
    } else if (g.compare) {
        if (g.runs>=g.cached_runs) g.compare=0;
        else {
            if (page!=g.stream_page) {
                if (read_sector(g.index_sector+1+page,g.page)) return GI_IO;
                g.stream_page=page;
            }
            if (u32(g.page+offset)!=first || u32(g.page+offset+4)!=count) g.compare=0;
        }
    }
    ++g.runs;
    return GI_OK;
}
static BYTE scan_image(void) {
    DWORD c=g.image_first, first=c, length=0, next;
    BYTE r;
    g.runs=0; g.stream_page=0xffffffffUL; g.fat_page=0xffffffffUL;
    for (DWORD i=0;i<g.image_clusters;++i) {
        if (g.index_first && c>=g.index_first && c-g.index_first<g.index_clusters)
            return GI_CHAIN; /* refuse an image cross-linked with its index */
        if ((r=fat_get(c,&next))) return r;
        ++length;
        if (i+1==g.image_clusters) {
            if (!eoc(next)) return GI_CHAIN; /* catches loops/overlong chains */
            if ((r=emit_run(first,length))) return r;
        } else {
            if (!cluster_ok(next)) return GI_CHAIN;
            if (next!=c+1) {
                if ((r=emit_run(first,length))) return r;
                first=next; length=0;
            }
            c=next;
        }
    }
    if (g.writing && g.runs%64 && write_sector(g.index_sector+1+g.runs/64,g.page)) return GI_IO;
    if (g.runs!=g.cached_runs) g.compare=0;
    giga_report.extents=g.runs;
    return GI_OK;
}
static BYTE prepare(char *path) {
    BYTE r=geometry();
    if (r) return r;
    if (pf_open(path)!=FR_OK) return GI_IMAGE;
    g.image_first=g.fs->org_clust; g.image_size=g.fs->fsize;
    if (!g.image_size || g.image_size>134217728UL || g.image_size%8192) return GI_IMAGE;
    DWORD cluster_bytes=(DWORD)g.fs->csize*512;
    g.image_clusters=(g.image_size+cluster_bytes-1)/cluster_bytes;
    g.index_bytes=512+((134217728UL/cluster_bytes*8+511)/512)*512;
    g.index_clusters=(g.index_bytes+cluster_bytes-1)/cluster_bytes;
    /* GCR->GIR, GCS->GIS, GCG->GIG, BIN->BIX. A BIN must never
     * accidentally use itself as its writable allocation sidecar. */
    char *ext=path+strlen(path)-3;
    char *changed=ext+(g.type=='B'?2:1);
    char original=*changed;
    *changed=g.type=='B'?'X':'I';
    FRESULT found=pf_giga_slot(path,&g.dir_sector,&g.dir_offset,g.name,g.page);
    if (found!=FR_OK && found!=FR_NO_FILE) { *changed=original; return GI_DIRECTORY; }
    if (found==FR_OK) {
        g.readonly=g.page[11]&1;
        if (g.page[11]&0x18 || u32(g.page+28)!=g.index_bytes) { *changed=original; return GI_COLLISION; }
        g.index_first=(DWORD)u16(g.page+26)|(g.width==4 ? (DWORD)u16(g.page+20)<<16 : 0);
        g.index_sector=cluster_sector(g.index_first);
        DWORD next;
        /* Validate the companion's own allocation before any direct writes. */
        for (DWORD i=0;i<g.index_clusters;++i) {
            if ((r=fat_get(g.index_first+i,&next))) { *changed=original; return r; }
            if (i+1<g.index_clusters ? next!=g.index_first+i+1 : !eoc(next)) {
                *changed=original; return GI_COLLISION;
            }
        }
        if (read_sector(g.index_sector,g.page)) { *changed=original; return GI_IO; }
        if (memcmp(g.page,"FGGIDX01",8)) { *changed=original; return GI_COLLISION; }
        g.cached_runs=u32(g.page+32);
        g.compare=g.page[8]==1 && g.page[9]==g.type && g.page[10]==g.fs->csize &&
            g.page[11]==g.width && u32(g.page+12)==g.fs->fatbase &&
            u32(g.page+16)==g.fs->database && u32(g.page+20)==g.fs->n_fatent &&
            u32(g.page+24)==g.image_first && u32(g.page+28)==g.image_size &&
            u32(g.page+36)==g.index_bytes && u32(g.page+40)==g.index_first &&
            u32(g.page+60)==header_crc(g.page) && g.cached_runs<=g.image_clusters;
    }
    *changed=original;
    /* Always compare the live allocation; no probabilistic stale-map test. */
    if ((r=scan_image())) return r;
    if (g.compare) { giga_report.reused=1; g.valid=1; return GI_OK; }
    if (g.readonly) return GI_READONLY;
    if (found==FR_NO_FILE) {
        if ((r=allocate_index())) return r;
        g.created=1;
    }
    header(0); /* invalidate before changing records */
    if (write_sector(g.index_sector,g.page)) return GI_IO;
    g.writing=1;
    if ((r=scan_image())) return r;
    g.writing=0;
    header(1); /* commit only after complete records */
    if (write_sector(g.index_sector,g.page)) return GI_IO;
    /* Read back and compare the full saved map before reporting success. */
    if (read_sector(g.index_sector,g.page) || u32(g.page+60)!=header_crc(g.page) || g.page[8]!=1)
        return GI_IO;
    g.cached_runs=g.runs; g.compare=1;
    if ((r=scan_image())) return r;
    if (!g.compare) return GI_IO;
    g.valid=1;
    return GI_OK;
}
BYTE giga_prepare(FATFS *fs, char *path, BYTE *scratch) {
    memset(&g,0,sizeof(g)); memset(&giga_report,0,sizeof(giga_report));
    g.fs=fs; g.fat=scratch; g.page=scratch+512;
    WORD n=strlen(path);
    if (n<5 || path[n-4]!='.') return GI_IMAGE;
    if (path[n-3]=='B' && path[n-2]=='I' && path[n-1]=='N') g.type='B';
    else if (path[n-3]=='G' && path[n-2]=='C' &&
        (path[n-1]=='R' || path[n-1]=='S' || path[n-1]=='G')) g.type=path[n-1];
    else return GI_IMAGE;
    BYTE result=prepare(path);
    if (!result && g.runs<=GIGA_RAM_EXTENTS) {
        /* FAT scanning has finished. Reuse its buffer for the small map.
         * Only its first GIGA_MAP_BYTES remain reserved during bank reads. */
        if (read_sector(g.index_sector+1,g.fat)) { g.valid=0; return GI_IO; }
        g.map_ram=1;
    }
    return result;
}
BYTE giga_map_in_ram(void) { return g.valid && g.map_ram; }
BYTE giga_read_sector(WORD bank, BYTE sector, BYTE *dest) {
    DWORD offset=(DWORD)bank*8192+(DWORD)sector*512;
    if (!g.valid || !dest || sector>=16 || offset>=g.image_size ||
        g.image_size-offset<512) return GI_IMAGE;
    DWORD skip=offset/512;
    for (DWORD i=0;i<g.runs;++i) {
        DWORD first,count;
        BYTE r=giga_get_extent(i,&first,&count);
        if (r) return r;
        DWORD sectors=count*g.fs->csize;
        if (skip>=sectors) { skip-=sectors;continue; }
        /* Capture the record before reading payload into the shared page.
         * The non-RAM path deliberately rereads records on the next call. */
        return disk_readp(dest,cluster_sector(first)+skip,0,512) ? GI_IO : GI_OK;
    }
    return GI_CHAIN;
}
BYTE giga_get_extent(DWORD index, DWORD *first, DWORD *clusters) {
    if (!g.valid || index>=g.runs) return GI_IMAGE;
    BYTE *p;
    if (g.map_ram) p=g.fat+(WORD)index*8;
    else {
        if (read_sector(g.index_sector+1+index/64,g.page)) return GI_IO;
        p=g.page+(WORD)(index%64)*8;
    }
    *first=u32(p); *clusters=u32(p+4);
    return GI_OK;
}
BYTE giga_read_bank(WORD bank) {
    DWORD offset=(DWORD)bank*8192;
    if (!g.valid || offset>=g.image_size) return GI_IMAGE;
    DWORD skip=offset/512, remaining=16, page=0xffffffffUL;
    for (DWORD i=0;i<g.runs && remaining;++i) {
        if (!g.map_ram && i/64!=page) {
            page=i/64;
            if (read_sector(g.index_sector+1+page,g.page)) return GI_IO;
        }
        BYTE *p=g.map_ram ? g.fat+(WORD)i*8 : g.page+(WORD)(i%64)*8;
        DWORD first=u32(p), count=u32(p+4), sectors=count*g.fs->csize;
        if (skip>=sectors) { skip-=sectors; continue; }
        DWORD take=sectors-skip;
        if (take>remaining) take=remaining;
        DWORD s=cluster_sector(first)+skip;
        while (take--) {
            if (disk_readp(0,s++,0,512)) return GI_IO;
            --remaining;
        }
        skip=0;
    }
    return remaining ? GI_CHAIN : GI_OK;
}
