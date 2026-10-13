/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "giga-prefetch.h"

static BYTE rank(WORD bank,const WORD *targets,BYTE count) {
    for(BYTE i=0;i<count;++i)if(targets[i]==bank)return i;
    return 255;
}
static BYTE step(GigaDemand *d,GigaResume *c,BYTE *buffer,GigaSectorRead read) {
    BYTE r=giga_resume_step_read(c,buffer,read);
    return (r==GC_MORE || r==GC_DONE)?GD_PROGRESS:(d->error=GD_IO);
}
BYTE giga_prefetch_service(GigaDemand *d,GigaResume *c,BYTE *buffer,
    const GigaFixed *fixed,GigaSectorRead read,const WORD *targets,BYTE count) {
    BYTE r=giga_demand_service_source(d,c,buffer,fixed,read);
    if(r!=GD_IDLE)return r;
    if(count==255 || (count && !targets))return d->error=GD_PROTOCOL;
    for(BYTE i=0;i<count;++i)
        if(targets[i]>=(giga_resume_aliases?256:16384))return d->error=GD_PROTOCOL;
    if(d->lease<c->count && c->slots[d->lease].progress!=GR_READY)
        return d->error=GD_PROTOCOL;
    /* Complete a started mapping transaction even if its priority changed.
     * No new plan or demand can expose a partially published bank. */
    if(c->pending!=255) {
        if(c->pending>=c->count || c->pending==d->lease)return d->error=GD_PROTOCOL;
        if(c->slots[c->pending].progress>=GR_PUBLISH)return step(d,c,buffer,read);
    }
    BYTE target=255,index=255;
    for(BYTE t=0;t<count;++t) {
        /* Resident-heavy callers can switch hundreds of times per second.
         * Do not make a new COMMIT wait through the whole advisory scan. */
        r=giga_demand_service_source(d,c,buffer,fixed,read);
        if(r!=GD_IDLE)return r;
        if(t && targets[t]==targets[t-1])continue;
        if(giga_fixed_slot(fixed,targets[t])!=255)continue;
        BYTE found=255;
        for(BYTE i=0;i<c->count;++i)
            if(c->slots[i].progress!=GR_EMPTY && c->slots[i].bank==targets[t]) {found=i;break;}
        if(found!=255 && c->slots[found].progress==GR_READY)continue;
        target=t;index=found;break;
    }
    if(c->pending!=255 && c->pending!=index)
        if(giga_resume_pause(c)!=GC_OK)return d->error=GD_PROTOCOL;
    if(target==255)return GD_IDLE;
    if(index!=255) {
        if(index==d->lease)return d->error=GD_PROTOCOL;
        if(c->pending==index)return step(d,c,buffer,read);
        if(giga_resume_select(c,index)!=GC_OK)return d->error=GD_PROTOCOL;
        return GD_PROGRESS;
    }
    /* Reuse only unleased space with lower priority than the wanted bank.
     * An empty slot wins; otherwise choose the least useful resident. */
    /* Ranks only participate in victim selection. Recomputing every rank
     * when all targets are already resident delays hits without doing I/O. */
    for(BYTE i=0;i<c->count;++i) {
        if(!(i&7)) {
            r=giga_demand_service_source(d,c,buffer,fixed,read);
            if(r!=GD_IDLE)return r;
        }
        c->slots[i].rank=rank(c->slots[i].bank,targets,count);
    }
    for(BYTE i=0;i<c->count;++i) {
        if(i==d->lease)continue;
        if(c->slots[i].progress==GR_EMPTY) {index=i;break;}
        if(c->slots[i].rank>target && (index==255 || c->slots[i].rank>c->slots[index].rank))index=i;
    }
    if(index==255)return GD_IDLE;
    if(c->slots[index].progress==GR_READY) {
        if(giga_resume_retire(c,index)!=GC_MORE)return d->error=GD_PROTOCOL;
        return GD_PROGRESS;
    }
    if(c->slots[index].progress!=GR_EMPTY && giga_resume_discard(c,index)!=GC_OK)
        return d->error=GD_PROTOCOL;
    if(giga_resume_begin(c,index,targets[target])!=GC_OK)return d->error=GD_PROTOCOL;
    c->slots[index].rank=target;
    return GD_PROGRESS;
}
