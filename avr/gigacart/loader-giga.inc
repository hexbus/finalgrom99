/* SPDX-License-Identifier: GPL-3.0-or-later
 * Included in the generated loader.c, sharing its existing 1024-byte buffer.
 */
#include <avr/interrupt.h>
#include <stdio.h>
#include <string.h>
#include "giga.h"
#ifdef FGCINEMA
static char giga_status[31]=""; /* 1.4 Beta 1: normal menu; errors remain visible. */
#elif defined(FGREUSETEST)
static char giga_status[31]="V14 - SELECT REUSE.GCR";
#elif defined(FGTIMINGTEST)
#ifdef FGLIVETEST
static char giga_status[31]="V13 - FAST OR LIVE.GCR";
#else
#ifdef FGSTREAMTEST
static char giga_status[31]="V12 - TIMING OR STREAM.GCR";
#else
static char giga_status[31]="V12 - SELECT TIMING.GCR";
#endif
#endif
#else
static char giga_status[31]="SELECT GIGACART TO PREPARE";
#endif
static char giga_detail[31];
static uint8_t has_giga;
static volatile uint16_t giga_ticks;
ISR(TIMER1_OVF_vect) { ++giga_ticks; }
static uint32_t giga_time(void) {
    uint8_t saved=SREG; cli();
    uint16_t high=giga_ticks, low=TCNT1;
    if ((TIFR1 & _BV(TOV1)) && low<0x8000) ++high;
    SREG=saved;
    return ((uint32_t)high<<16)|low;
}
#ifdef FGBANKTEST
#include "bank-test.inc"
#endif
#ifdef FGPLAYERTEST
#include "player-test.inc"
#endif
#ifdef FGTIMINGTEST
#include "timing-test.inc"
#endif
#ifdef FGREUSETEST
#include "reuse-test.inc"
#endif
#ifdef FGCINEMA
#include "cinema-session.inc"
#endif
static uint8_t genGigaEntry(uint8_t *fn) {
    uint8_t n=strlen((char *)fn);
    if (n<5 || fn[n-4]!='.' || fn[n-3]!='G' || fn[n-2]!='C') return 0;
    uint8_t kind=fn[n-1]=='R' ? 0 : fn[n-1]=='S' ? 1 : fn[n-1]=='G' ? 2 : 255;
    if (kind==255 || !sd_fno.fsize || sd_fno.fsize>134217728UL || sd_fno.fsize%8192) return 0;
    for (uint8_t i=0;i<8;++i) writeByte(i<n-4 ? fn[i] : 0);
    writeByte(0xff); writeByte(SEL_GIGA|(kind<<4));
    writeByte(FG99_RESTART>>8); writeByte(FG99_RESTART&255);
    /* Preserve the real extension in the visible label. */
    for (uint8_t i=0;i<ENTRY_NAME_LEN;++i) writeByte(i<n ? fn[i] : ' ');
    has_giga=1;
    return 1;
}
__attribute__((noinline)) void prepareGigacart(void) {
    led(1); writeBegin(); setConfig(0);
    uint8_t kind=(selection[9]>>4)&3;
    buildPath(kind==0 ? "GCR" : kind==1 ? "GCS" : "GCG");
#ifdef FGCINEMA
    {
        uint8_t result=profile_session();
        giga_format_error(giga_status,PSTR("PROFILE STARTUP ERROR "),result);
    }
    /* Launch may reuse path as session state. Recover to root on every error,
     * including failures after the original directory has been overwritten. */
    path_len=0; path[0]=0;
    giga_detail[0]=0;
    return;
#else
#ifdef FGPLAYERTEST
    uint8_t playing=kind==0 && memcmp(selection,"DEMO\0",5)==0;
    if(playing && player_report_begin()) {
        strcpy_P(giga_status,PSTR("NEED ROOT PLAYER.CSV")); giga_detail[0]=0; return;
    }
#endif
#ifdef FGBANKTEST
    uint8_t testing=kind==0 && memcmp(selection,"TEST\0",5)==0;
    if (testing && bank_report_begin()) {
        strcpy_P(giga_status,PSTR("NEED WRITABLE ROOT INDEX.CSV"));
        giga_detail[0]=0;
        return;
    }
#endif
    cli(); TCCR1A=0; TCCR1B=0; TCNT1=0; giga_ticks=0;
    TIFR1=_BV(TOV1); TIMSK1=_BV(TOIE1); TCCR1B=_BV(CS11); sei();
#ifdef FGREUSETEST
    if(kind==0 && path_len==0 && memcmp(selection,"REUSE\0",6)==0) {
        uint8_t result=reuse_session();
        snprintf_P(giga_status,sizeof(giga_status),PSTR("REUSE SETUP ERROR %u"),result);
        strcpy_P(giga_detail,PSTR("SEE V14 TRIAL GUIDE"));
    } else {
        strcpy_P(giga_status,PSTR("SELECT ROOT REUSE.GCR"));giga_detail[0]=0;
    }
    cli();TIMSK1=0;TCCR1B=0;return;
#elif defined(FGTIMINGTEST)
    uint8_t streaming=0;
#ifdef FGSTREAMTEST
    streaming=kind==0 && memcmp(selection,"STREAM\0",7)==0;
#endif
#ifdef FGLIVETEST
    if(kind==0 && memcmp(selection,"FAST\0",5)==0) streaming=2;
    if(kind==0 && memcmp(selection,"LIVE\0",5)==0) streaming=3;
#endif
    if(streaming || (kind==0 && memcmp(selection,"TIMING\0",7)==0)) {
        uint8_t result=timing_session(streaming);
        cli();TIMSK1=0;TCCR1B=0;
        snprintf_P(giga_status,sizeof(giga_status),PSTR("%s SETUP ERROR %u"),streaming==3?"LIVE":streaming==2?"FAST":streaming?"STREAM":"TIMING",result);
        if(result==1) snprintf_P(giga_detail,sizeof(giga_detail),PSTR("NEED 4096-BYTE %s.CSV"),
                    streaming==3?"LIVE":streaming==2?"FAST":streaming?"STREAM":"TIMING");
        else strcpy_P(giga_detail,result==9?PSTR("NEED 1MIB ROOT STREAM.DAT"):result==10?PSTR("STREAM.DAT CONTENT ERROR"):PSTR("NEED MATCHING GIGA CPLD"));
        return;
    }
    cli();TIMSK1=0;TCCR1B=0;
    strcpy_P(giga_status,PSTR("SELECT TIMING.GCR TO TEST"));giga_detail[0]=0;
    return;
#else
    uint32_t start=giga_time();
    uint8_t result=giga_prepare(&sd_fs,(char *)path,buffer);
    uint32_t ms=(giga_time()-start)/1000;
#ifdef FGPLAYERTEST
    if(!result && playing) result=player_session(ms);
#endif
#ifdef FGBANKTEST
    uint8_t bank_result=0;
    if (!result && testing) bank_result=bank_test(ms);
#endif
    cli(); TIMSK1=0; TCCR1B=0;
    giga_detail[0]=0;
    if (result) snprintf_P(giga_status,sizeof(giga_status),PSTR("PREP ERROR %u - SEE GUIDE"),result);
    else {
        snprintf_P(giga_status,sizeof(giga_status),giga_report.reused ?
                   PSTR("INDEX REUSED %luMS") : PSTR("INDEX CREATED %luMS"),ms);
        snprintf_P(giga_detail,sizeof(giga_detail),PSTR("%lu EXTENTS"),giga_report.extents);
#ifdef FGBANKTEST
        if (testing) {
            if (bank_result) snprintf_P(giga_detail,sizeof(giga_detail),PSTR("BANK TEST ERROR %u - INDEX.CSV"),bank_result);
            else strcpy_P(giga_detail,PSTR("BANK TEST PASS - INDEX.CSV"));
        }
#endif
    }
    /* Main loop regenerates the patched menu and resumes its normal sender.
     * No Gigacart image is launched while CPLD paging remains unimplemented. */
#endif
#endif
}
