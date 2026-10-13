/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "giga-fixed.h"
BYTE giga_fixed_check(const GigaFixed *f,const GigaResume *c) {
    if(!f || !c || !c->first || c->first>=125 || !c->count ||
       c->count>125-c->first || (f->count && !f->ranges))return GC_STATE;
    WORD previous=0;
    for(BYTE i=0;i<f->count;++i) {
        const GigaFixedRange *r=f->ranges+i;
        if(!r->count || r->bank>=16384 || r->count>16384-r->bank ||
           (i && r->bank<previous) || !r->first || r->first>=125 ||
           r->count>(WORD)(125-r->first))return GC_STATE;
        WORD end=r->first+r->count;
        if(r->first<c->first+c->count && end>c->first)return GC_STATE;
        for(BYTE j=0;j<i;++j) {
            const GigaFixedRange *q=f->ranges+j;
            if(r->first<q->first+q->count && end>q->first)return GC_STATE;
        }
        previous=r->bank+r->count;
    }
    return GC_OK;
}
BYTE giga_fixed_slot(const GigaFixed *f,WORD bank) {
    if(f)for(BYTE i=0;i<f->count;++i) {
        const GigaFixedRange *r=f->ranges+i;
        if(bank<r->bank)break;
        if(bank-r->bank<r->count)return r->first+(bank-r->bank);
    }
    return 255;
}
