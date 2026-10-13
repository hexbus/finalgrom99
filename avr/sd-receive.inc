/* SPDX-License-Identifier: GPL-3.0-or-later
 * Buffered SPI payload receive. Commands, token waits, CRC consumption and
 * the legacy direct-to-cartridge stream remain in diskio.c. Poll completion
 * for every byte; this works at both initialization and running SPI clocks.
 */
/* Keep this leaf out of disk_readp's register-heavy command/forward path. */
static __attribute__((noinline,noclone)) void sd_receive(BYTE *dest, UINT count) {
    do {
        SPDR=0xff;
        while (!(SPSR & _BV(SPIF))) { }
        *dest++=SPDR;
    } while (--count);
}
