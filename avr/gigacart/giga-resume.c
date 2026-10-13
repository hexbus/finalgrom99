/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "giga-resume.h"
#include "giga-wire.h"
BYTE giga_resume_aliases;
BYTE giga_resume_init(GigaResume *c,GigaResumeSlot *slots,BYTE first,BYTE count) {
    if(!c || !slots || !first || first>=125 || !count || count>125-first) return GC_STATE;
    c->slots=slots;c->first=first;c->count=count;c->pending=255;
    giga_resume_aliases=0;
    for(BYTE i=0;i<count;++i) {
        slots[i].bank=0;slots[i].progress=GR_EMPTY;slots[i].rank=255;
    }
    return GC_OK;
}
BYTE giga_resume_begin(GigaResume *c,BYTE index,WORD bank) {
    if(!c || index>=c->count || bank>=(giga_resume_aliases?256:16384) || c->pending!=255 ||
       c->slots[index].progress!=GR_EMPTY) return GC_STATE;
    for(BYTE i=0;i<c->count;++i)
        if(c->slots[i].progress!=GR_EMPTY && c->slots[i].bank==bank) return GC_STATE;
    c->slots[index].bank=bank;c->slots[index].progress=0;c->pending=index;
    return GC_OK;
}
BYTE giga_resume_select(GigaResume *c,BYTE index) {
    if(!c || index>=c->count || c->pending!=255 || c->slots[index].progress>=16)
        return GC_STATE;
    c->pending=index;return GC_OK;
}
BYTE giga_resume_pause(GigaResume *c) {
    if(!c || c->pending==255 || c->slots[c->pending].progress>=16) return GC_STATE;
    c->pending=255;return GC_OK;
}
BYTE giga_resume_step(GigaResume *c,BYTE *buffer) {
    return giga_resume_step_read(c,buffer,giga_read_sector);
}
BYTE giga_resume_step_read(GigaResume *c,BYTE *buffer,GigaSectorRead read) {
    if(!c || c->pending==255) return GC_STATE;
    GigaResumeSlot *s=c->slots+c->pending;
    if(s->progress<16) {
        if(!buffer || !read) return GC_STATE;
        if(read(s->bank,s->progress,buffer)) return GC_DISK;
        giga_wire_address((DWORD)(c->first+c->pending)*8192+(WORD)s->progress*512);
        if(giga_wire_block(buffer,512)!=512) return GC_WIRE;
        ++s->progress;return GC_MORE;
    }
    if(s->progress!=GR_PUBLISH && s->progress!=GR_RETIRE) return GC_STATE;
    BYTE publish=s->progress==GR_PUBLISH;
    WORD key=s->bank;
    do {
        giga_wire_address(0xfc000UL+key);
        if(!giga_wire_byte(publish?(0x80|(c->first+c->pending)):0)) return GC_WIRE;
        key+=256;
    } while(giga_resume_aliases && key<16384);
    s->progress=publish?GR_READY:GR_EMPTY;c->pending=255;
    return GC_DONE;
}
BYTE giga_resume_retire(GigaResume *c,BYTE index) {
    if(!c || index>=c->count || c->pending!=255 || c->slots[index].progress!=GR_READY)
        return GC_STATE;
    c->slots[index].progress=GR_RETIRE;c->pending=index;return GC_MORE;
}
BYTE giga_resume_discard(GigaResume *c,BYTE index) {
    if(!c || index>=c->count || c->slots[index].progress>=16) return GC_STATE;
    if(c->pending==index)c->pending=255;
    c->slots[index].progress=GR_EMPTY;return GC_OK;
}
#ifdef __AVR__
typedef char resume_workspace_fits[(sizeof(GigaResume)+81*sizeof(GigaResumeSlot)<=384)?1:-1];
#endif
