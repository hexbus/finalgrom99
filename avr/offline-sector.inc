/* SPDX-License-Identifier: GPL-3.0-or-later
 * Hardware has a bank selector and a wrapping 13-bit streaming pointer.
 * A nonzero sector MUST continue the preceding write in the same slot. */
static uint8_t compilation_write(uint8_t slot,uint8_t sector,const uint8_t *p) {
    if(!sector) writeSize(slot);
    for(uint16_t i=0;i<512;++i) writeByte(p[i]);
    return 0; /* Offline wire has no acknowledgement/readback. */
}
