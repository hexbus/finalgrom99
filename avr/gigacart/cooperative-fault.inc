/* SPDX-License-Identifier: GPL-3.0-or-later
 * Preallocated 1024-byte report. No FAT allocation or online SD writes.
 * Sector zero snapshots the map/slot workspace; sector one commits last.
 */
static BYTE cooperative_report_write(const BYTE *data) {
    UINT written;
    return pf_write(data,512,&written) || written!=512 || pf_write(0,0,&written);
}
static BYTE cooperative_report_open(void) {
    return pf_open("MAILBOX.BIN") || sd_fs.fsize!=1024;
}
static BYTE cooperative_report_start(BYTE *scratch) {
    if(cooperative_report_open())return 0;
    memset(scratch,0,512);memcpy(scratch,"FGCOPEN1",8);
    return !cooperative_report_write(scratch);
}
static BYTE cooperative_report_save(BYTE error,BYTE state,WORD bank,BYTE epoch) {
    CooperativeSession *s=(CooperativeSession *)path;
    BYTE *header=buffer+512;
    memset(header,0,512);memcpy(header,"FGCFAULT",8);
    header[8]=error;header[9]=state;memcpy(header+10,&bank,2);header[12]=epoch;
    header[13]=s->prediction_enabled;
    memcpy(header+16,&s->demand,sizeof(GigaDemand));
    header[22]=s->cache.first;header[23]=s->cache.count;header[24]=s->cache.pending;
    memcpy(header+26,&s->prediction_bank,2);
    DWORD check=crc(0xffffffffUL,buffer,512)^0xffffffffUL;
    memcpy(header+28,&check,4);
    /* Raw context is supplementary evidence; explicit fields above have a
     * stable layout. Keep snapshots within the hardware buffer allocations. */
    memcpy(header+32,path,128);memcpy(header+160,buffer+448,64);
    check=crc(0xffffffffUL,header,508)^0xffffffffUL;memcpy(header+508,&check,4);
    if(cooperative_report_open() || cooperative_report_write(buffer))return 1;
    return cooperative_report_write(header);
}
