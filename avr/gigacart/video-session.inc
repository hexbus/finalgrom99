/* SPDX-License-Identifier: GPL-3.0-or-later
 * Cooperative session. External policy 4 startup, GOV overlay, optional GPR.
 * Requires the experimental mailbox CPLD; no legacy observer calls in RUN.
 */
#include "giga-launch.h"
#include "giga-demand.h"
#include "giga-overlay.h"
#include "giga-wire.h"
#include "giga-prefetch.h"
#include "giga-predict.h"

#include "crc32.h"
static uint8_t compilation_write(uint8_t,uint8_t,const uint8_t *);
typedef struct {
    GigaAux launch_file,overlay_file;
    GigaOverlay overlay;
    GigaFixedRange ranges[8];
    GigaFixed fixed;
    GigaDemand demand;
    GigaResume cache;
    WORD prediction_bank;
    BYTE prediction_enabled;
    BYTE report_enabled;
} CooperativeSession;
typedef struct {
    GigaAux launch_file,overlay_file,prediction_file;
    GigaLaunch launch;
    BYTE image_sha[32];
} CooperativeStartup;
#ifdef __AVR__
typedef char cooperative_state_fits_path[(sizeof(CooperativeSession)<=sizeof(path))?1:-1];
typedef char cooperative_slots_fit[(80*sizeof(GigaResumeSlot)<=384)?1:-1];
typedef char cooperative_prediction_fits[(sizeof(GigaPredict)<=64)?1:-1];
typedef char cooperative_startup_fits[(sizeof(CooperativeStartup)<=320)?1:-1];
#endif
static uint8_t cooperative_read(WORD bank,BYTE sector,BYTE *data) {
    CooperativeSession *s=(CooperativeSession *)path;
    return giga_overlay_read(&s->overlay,bank,sector,data);
}
static WORD cooperative_u16(const BYTE *p) {return p[0]|(WORD)p[1]<<8;}
static DWORD cooperative_u32(const BYTE *p) {return cooperative_u16(p)|(DWORD)cooperative_u16(p+2)<<16;}
#include "cooperative-fault.inc"
static uint8_t profile_session(void) {
    BYTE *scratch=buffer+512;
    writeSize(0);for(WORD i=0;i<8192;++i)writeByte(0);
    BYTE error=giga_prepare(&sd_fs,(char *)path,buffer);
    if(error)return 60+error;
    if(!giga_map_in_ram())return 5;
    DWORD image_bytes=sd_fs.fsize;
    /* Reuse the not-yet-active slot workspace for startup temporaries. Their
     * lifetime ends before giga_resume_init writes the 80 slot records. */
    CooperativeStartup *t=(CooperativeStartup *)(buffer+128);
    BYTE n=strlen((char *)path);char saved[3];
    if(n<4)return 110;
    memcpy(saved,path+n-3,3);memcpy(path+n-3,"GLP",3);
    error=giga_aux_open(&sd_fs,(char *)path,&t->launch_file,scratch);
    memcpy(path+n-3,saved,3);
    if(error)return 80+error;
    if(giga_aux_read(&t->launch_file,0,scratch) || scratch[18]!=4)return 110;
    memcpy(t->image_sha,scratch+32,32);
    memcpy(path+n-3,"GOV",3);
    error=giga_aux_open(&sd_fs,(char *)path,&t->overlay_file,scratch);
    memcpy(path+n-3,saved,3);
    if(error)return 90+error;
    /* Prediction is optional; a missing or invalid model falls back to the
     * same explicit request/wait path. Never infer safety from this model. */
    GigaPredict *prediction=(GigaPredict *)(buffer+448);
    memcpy(path+n-3,"GPR",3);
    BYTE prediction_enabled=!giga_aux_open(&sd_fs,(char *)path,&t->prediction_file,scratch);
    memcpy(path+n-3,saved,3);
    if(prediction_enabled)
        prediction_enabled=giga_predict_open(prediction,&t->prediction_file,t->image_sha,scratch)==GP_OK;
    BYTE report_enabled=cooperative_report_start(scratch);
    setConfig(4);receiveBegin();
    uint16_t bank;BYTE epoch;
    BYTE identified=giga_mailbox_query(&bank,&epoch);
    writeBegin();
    if(identified!=0) {error=2;goto fail;}
    error=giga_launch_load(&t->launch_file,image_bytes,&t->launch,scratch,compilation_write);
    if(error) {error+=100;goto fail;}
    if(t->launch.policy!=4 || t->launch.count>80 || t->launch.grom || !t->launch.runtime) {error=110;goto fail;}
    /* The path is no longer needed. Retain both raw-sector maps without
     * sharing their state with Petit FatFs's current open file. */
    CooperativeSession *s=(CooperativeSession *)path;
    memset(s,0,sizeof(*s));s->launch_file=t->launch_file;s->overlay_file=t->overlay_file;
    if(giga_aux_read(&s->launch_file,t->launch.runtime,scratch) ||
       memcmp(scratch,"FGDM0001",8) || !scratch[8] || scratch[8]>8 || scratch[9] ||
       cooperative_u16(scratch+10)!=image_bytes/8192 ||
       (crc(0xffffffffUL,scratch,508)^0xffffffffUL)!=cooperative_u32(scratch+508)) {error=111;goto fail;}
    s->fixed.ranges=s->ranges;s->fixed.count=scratch[8];
    for(BYTE i=0;i<s->fixed.count;++i) {
        BYTE *p=scratch+16+i*5;
        s->ranges[i].bank=cooperative_u16(p);s->ranges[i].count=cooperative_u16(p+2);s->ranges[i].first=p[4];
        if(s->ranges[i].bank>=image_bytes/8192 || s->ranges[i].count>image_bytes/8192-s->ranges[i].bank) {error=111;goto fail;}
    }
    giga_demand_init(&s->demand);
    s->prediction_bank=0xffff;s->prediction_enabled=prediction_enabled;
    s->report_enabled=report_enabled;
    if(giga_overlay_open(&s->overlay,&s->overlay_file,image_bytes,scratch)) {error=113;goto fail;}
    if(giga_aux_read(&s->launch_file,t->launch.bridge_page,scratch)) {error=114;goto fail;}
    WORD bridge_bytes=t->launch.bridge_bytes;
    BYTE first=t->launch.first,count=t->launch.count;
    /* Startup map is no longer used; reuse its storage for the predictor
     * before replacing all startup temporaries with cache slot records. */
    if(prediction_enabled) {s->launch_file=t->prediction_file;prediction->file=&s->launch_file;}
    if(giga_resume_init(&s->cache,(GigaResumeSlot *)(buffer+128),first,count)!=GC_OK ||
       giga_fixed_check(&s->fixed,&s->cache)!=GC_OK) {error=112;goto fail;}
    /* Compiler establishes correspondence of fixed catalog, patched startup
     * jobs and mappings. No console code is published until all checks pass. */
    writeSize(0x80);writeSize(0);
    for(WORD i=0;i<bridge_bytes;++i)writeByte(scratch[i]);
    led(0);receiveBegin();
    for(;;) {
        error=giga_demand_service_source(&s->demand,&s->cache,scratch,&s->fixed,cooperative_read);
        if(error==GD_ACK && s->demand.lease!=GD_FIXED_LEASE)
            s->prediction_bank=s->demand.bank;
        if(error==GD_IDLE) {
            BYTE count=0;
            if(s->prediction_enabled && s->prediction_bank!=0xffff) {
                count=giga_predict_targets(prediction,s->prediction_bank);
                if(!count) {
                    if(prediction->phase==GP_OFF || prediction->phase==GP_READY)
                        giga_predict_begin(prediction,s->prediction_bank);
                    BYTE result=giga_predict_step(prediction,scratch);
                    if(result==GP_OK || result==GP_MORE)continue;
                    s->prediction_enabled=0;
                }
            }
            error=giga_prefetch_service(&s->demand,&s->cache,scratch,&s->fixed,cooperative_read,
                count?prediction->targets:0,count);
            if(error==GD_ACK && s->demand.lease!=GD_FIXED_LEASE)
                s->prediction_bank=s->demand.bank;
        }
        if(error<GD_IO)continue;
        /* A speculative failure can happen while the CPU is still running.
         * Preserve its old lease until its next explicit RAM-side COMMIT.
         * Only then take the cartridge offline and write a diagnostic. A
         * broken transport identity is already a protocol failure. */
        if(s->report_enabled) {
            uint16_t failed_bank=0xffff;BYTE failed_epoch=0,state;
            do {state=giga_mailbox_query(&failed_bank,&failed_epoch);} while(state==0);
            writeBegin();cooperative_report_save(error,state,failed_bank,failed_epoch);
        }
        for(;;) {
            for(BYTE i=0;i<error+10;++i) {led(1);_delay_ms(250);led(0);_delay_ms(250);}
            _delay_ms(1500);
        }
    }
fail:
    setConfig(0);path[0]=0;return error;
}
