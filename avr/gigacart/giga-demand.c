/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "giga-demand.h"
#include "giga-wire.h"

void giga_demand_init(GigaDemand *d) {
    d->bank=0;d->epoch=0;d->active=0;d->lease=255;d->error=0;
}
BYTE giga_demand_service(GigaDemand *d,GigaResume *c,BYTE *buffer) {
    return giga_demand_service_fixed(d,c,buffer,0);
}
BYTE giga_demand_service_fixed(GigaDemand *d,GigaResume *c,BYTE *buffer,const GigaFixed *fixed) {
    return giga_demand_service_source(d,c,buffer,fixed,giga_read_sector);
}
BYTE giga_demand_service_source(GigaDemand *d,GigaResume *c,BYTE *buffer,const GigaFixed *fixed,GigaSectorRead read) {
    uint16_t bank;BYTE epoch;
    if(!d || !c || !buffer || !read)return GD_PROTOCOL;
    if(d->error)return d->error; /* An I/O failure never acknowledges. */
    BYTE q=giga_mailbox_query(&bank,&epoch);
    if(q==255)return d->error=GD_PROTOCOL;
    if(!q)return GD_IDLE;
    if(!d->active) {
        /* Explicit new COMMIT releases the preceding execution lease. */
        d->bank=bank;d->epoch=epoch;d->active=1;d->lease=255;
    } else if(d->bank!=bank || d->epoch!=epoch)return d->error=GD_PROTOCOL;

    /* A prior publication/retirement cannot be abandoned half complete. */
    if(c->pending!=255) {
        GigaResumeSlot *s=c->slots+c->pending;
        if(s->progress>=GR_PUBLISH || s->bank==bank) {
            BYTE r=giga_resume_step_read(c,buffer,read);
            if(r!=GC_MORE && r!=GC_DONE)return d->error=GD_IO;
            return GD_PROGRESS;
        }
        if(giga_resume_pause(c)!=GC_OK)return d->error=GD_PROTOCOL;
    }
    BYTE index=255;
    for(BYTE i=0;i<c->count;++i) {
        if(c->slots[i].progress!=GR_EMPTY && c->slots[i].bank==bank) {
            index=i;break;
        }
    }
    if(giga_fixed_slot(fixed,bank)!=255) {
        /* A fixed mapping cannot also exist in the reusable pool. Permanent
         * ownership was established offline; no SD read or eviction needed. */
        if(index!=255)return d->error=GD_PROTOCOL;
        d->lease=GD_FIXED_LEASE;
        if(!giga_mailbox_ack(bank,epoch))return d->error=GD_PROTOCOL;
        d->active=0;return GD_ACK;
    }
    if(index!=255) {
        if(c->slots[index].progress==GR_READY) {
            /* Establish the lease BEFORE ACK. Only a subsequent COMMIT
             * releases it; ordinary idle service never changes this slot. */
            d->lease=index;
            if(!giga_mailbox_ack(bank,epoch))return d->error=GD_PROTOCOL;
            d->active=0;return GD_ACK;
        }
        if(giga_resume_select(c,index)!=GC_OK)return d->error=GD_PROTOCOL;
        return GD_PROGRESS;
    }
    /* No execution lease remains while the requesting CPU is parked.
     * Prefer empty/partial space, then a complete victim. Pinned banks are
     * NOT in this pool. Plan rank is advisory in this demand-only prototype. */
    for(BYTE i=0;i<c->count;++i) {
        if(c->slots[i].progress==GR_EMPTY) {index=i;break;}
        if(index==255 || c->slots[i].progress<GR_PUBLISH)index=i;
    }
    if(index==255)return d->error=GD_PROTOCOL;
    if(c->slots[index].progress==GR_READY) {
        if(giga_resume_retire(c,index)!=GC_MORE)return d->error=GD_PROTOCOL;
        return GD_PROGRESS;
    }
    if(c->slots[index].progress!=GR_EMPTY && giga_resume_discard(c,index)!=GC_OK)
        return d->error=GD_PROTOCOL;
    if(giga_resume_begin(c,index,bank)!=GC_OK)return d->error=GD_PROTOCOL;
    return GD_PROGRESS;
}
