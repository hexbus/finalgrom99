/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "giga-predict.h"
#include <string.h>
#include "crc32.h"
static WORD u16(const BYTE *p) {return p[0]|(WORD)p[1]<<8;}
static DWORD u32(const BYTE *p) {return u16(p)|(DWORD)u16(p+2)<<16;}
static BYTE fail(GigaPredict *p,BYTE error) {p->phase=GP_OFF;return error;}
BYTE giga_predict_open(GigaPredict *p,const GigaAux *f,const BYTE sha[32],BYTE *buf) {
    if(!p || !f || !sha || !buf)return GP_FORMAT;
    memset(p,0,sizeof(*p));p->file=f;
    if(giga_aux_read(f,0,buf))return GP_IO;
    if(memcmp(buf,"FGPR0001",8) || memcmp(buf+16,sha,32))return GP_FORMAT;
    for(BYTE i=48;i<64;++i)if(buf[i])return GP_FORMAT;
    p->records=u16(buf+8);p->words=u16(buf+10);
    DWORD bytes=1088UL+(DWORD)p->records*8+(DWORD)p->words*2;
    if(!p->records || f->pages!=(bytes+511)/512)return GP_FORMAT;
    DWORD expected=u32(buf+12),sum=0xffffffffUL;
    for(DWORD pos=64;pos<bytes;) {
        WORD start=pos&511,n=512-start;
        if(n>bytes-pos)n=bytes-pos;
        if(pos>=512 && giga_aux_read(f,pos/512,buf))return GP_IO;
        sum=crc(sum,buf+start,n);pos+=n;
    }
    if((sum^0xffffffffUL)!=expected)return GP_FORMAT;
    /* All pad bytes are outside the logical CRC, but must be canonical. */
    for(WORD n=bytes&511;n && n<512;++n)if(buf[n])return GP_FORMAT;
    return GP_OK;
}
void giga_predict_begin(GigaPredict *p,WORD bank) {
    p->bank=bank;p->branches=p->truncated=0;
    p->window_first=bank&0xffc0;p->window_last=p->window_first+63;
    p->sequential_last=0;p->phase=bank<16384?GP_DIRECTORY:GP_OFF;
}
BYTE giga_predict_step(GigaPredict *p,BYTE *buf) {
    if(!p || !buf || !p->file)return GP_FORMAT;
    if(p->phase==GP_READY)return GP_OK;
    DWORD offset;
    if(p->phase==GP_DIRECTORY) {
        offset=64UL+(p->bank/64)*4;
        if(giga_aux_read(p->file,offset/512,buf))return fail(p,GP_IO);
        p->index=u16(buf+(offset&511));p->end=u16(buf+(offset&511)+2);
        if(p->index>p->end || p->end>p->records)return fail(p,GP_FORMAT);
        p->phase=p->index==p->end?GP_READY:GP_RECORD;
    } else if(p->phase==GP_RECORD) {
        offset=1088UL+(DWORD)p->index*8;
        if(giga_aux_read(p->file,offset/512,buf))return fail(p,GP_IO);
        WORD at=offset&511;
        do {
            WORD first=u16(buf+at),last=u16(buf+at+2),pool=u16(buf+at+4);
            BYTE count=buf[at+6];
            if(first>last || last>=16384 || pool>p->words || count>p->words-pool || buf[at+7])return fail(p,GP_FORMAT);
            ++p->index;
            if(p->bank<first) {
                if(first-1<p->window_last)p->window_last=first-1;
            } else if(p->bank>last) {
                if(last+1>p->window_first)p->window_first=last+1;
            } else {
                if(first>p->window_first)p->window_first=first;
                if(last<p->window_last)p->window_last=last;
                if(last>p->sequential_last)p->sequential_last=last;
                if(count) {p->pool=pool;p->next_pool=pool+count;p->phase=GP_POOL;break;}
            }
            at+=8;
        } while(at<512 && p->index<p->end);
        if(p->phase==GP_RECORD && p->index==p->end)p->phase=GP_READY;
    } else if(p->phase==GP_POOL) {
        offset=1088UL+(DWORD)p->records*8+(DWORD)p->pool*2;
        if(giga_aux_read(p->file,offset/512,buf))return fail(p,GP_IO);
        WORD at=offset&511;
        do {
            WORD bank=u16(buf+at);if(bank>=16384)return fail(p,GP_FORMAT);
            BYTE found=0;
            for(BYTE i=0;i<p->branches;++i)if(p->targets[GP_LEAD+i]==bank)found=1;
            if(!found) {
                if(p->branches<GP_BRANCHES)p->targets[GP_LEAD+p->branches++]=bank;
                else p->truncated=1;
            }
            ++p->pool;at+=2;
        } while(at<512 && p->pool<p->next_pool);
        if(p->pool==p->next_pool)p->phase=p->index==p->end?GP_READY:GP_RECORD;
    } else return GP_FORMAT;
    return p->phase==GP_READY?GP_OK:GP_MORE;
}
BYTE giga_predict_targets(GigaPredict *p,WORD bank) {
    if(!p || p->phase!=GP_READY || bank<p->window_first || bank>p->window_last)return 0;
    for(BYTE i=0;i<GP_LEAD;++i) {
        WORD next=bank+i+1;
        p->targets[i]=next<=p->sequential_last?next:bank;
    }
    return GP_LEAD+p->branches;
}
#ifdef __AVR__
typedef char predictor_fits_workspace[(sizeof(GigaPredict)<=64)?1:-1];
#endif
