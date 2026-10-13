/* Experimental stock-pin transport; not a complete cartridge runtime.
 * SPDX-License-Identifier: GPL-3.0-or-later */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "giga-wire.h"

#if F_CPU != 8000000UL
#error The SRAM ownership sequence requires an 8 MHz AVR build.
#endif

#define CLK _BV(PC2)
#define ACK _BV(PC3)
#define CMD1 _BV(PC0)
#define CMD0 _BV(PC4)
#define COMMAND_MASK (_BV(PC1)|CMD1|CMD0)

static void command(uint8_t bits) {
    PORTC=(PORTC & ~(COMMAND_MASK|ACK)) | CLK | bits;
    __asm__ __volatile__("nop\n\tnop" ::: "memory");
}
static void input(uint8_t bits) {
    /* Stop driving CDX before selecting a CPLD output. */
    DDRD=0;PORTD=0;DDRC &= ~ACK;
    command(bits);
}
static void output(uint8_t bits) {
    /* Select a high-impedance CPLD command before enabling AVR outputs. */
    DDRD=0;PORTD=0;DDRC &= ~ACK;
    command(bits);DDRD=0xff;
}
uint8_t giga_wire_identify(void) {
    input(0);PORTC &= ~CLK;
    __asm__ __volatile__("nop\n\tnop" ::: "memory");
    uint8_t id=PIND;PORTC |= CLK;return id==0xa7;
}
void giga_wire_address(uint32_t address) {
    output(CMD1);
    for (int8_t shift=16;shift>=0;shift-=8) {
        PORTD=(uint8_t)(address>>shift);
        PORTC &= ~CLK;PORTC |= CLK;
    }
    command(CMD1|CMD0);
}
uint8_t giga_wire_byte(uint8_t value) {
    PORTD=value;
    const uint8_t high=PORTC|CLK, low=high & ~CLK;
    for (uint16_t tries=0;tries<256;++tries) {
        uint8_t saved=SREG;cli();
        /* Exactly six OUT instructions, one 125 ns cycle each at 8 MHz.
         * The first falling edge samples idle; the first rising edge grants.
         * The second pulse writes; registered phase 100 then releases DATA.
         * The CPLD releases ownership after the driver control is low, without
         * waiting for the third falling edge. The release chain starts at
         * nominal 375 ns; fitted propagation is additional. The final rising
         * edge advances only an accepted byte. See the timing guide.
         * There must be no interrupt, call, branch or loop within this block. */
        __asm__ __volatile__(
            "out %[port],%[low]\n\t"
            "out %[port],%[high]\n\t"
            "out %[port],%[low]\n\t"
            "out %[port],%[high]\n\t"
            "out %[port],%[low]\n\t"
            "out %[port],%[high]\n\t"
            : : [port] "I" (_SFR_IO_ADDR(PORTC)), [low] "r" (low), [high] "r" (high)
            : "memory");
        uint8_t ok=PINC & ACK;SREG=saved;
        if(ok) return 1;
        /* Prevent a failed request from becoming an unlimited MCU stall.
         * A subsequent caller must treat timeout as an uncompleted bank. */
        _delay_us(1);
    }
    return 0;
}
uint16_t giga_wire_block(const uint8_t *data, uint16_t length) {
    const uint8_t high=PORTC|CLK, low=high & ~CLK;
    uint16_t done=0;
    while(done<length) {
        PORTD=data[done];
        uint8_t tries=0;
        for(;;) {
            uint8_t saved=SREG;cli();
            __asm__ __volatile__(
                "out %[port],%[low]\n\t"
                "out %[port],%[high]\n\t"
                "out %[port],%[low]\n\t"
                "out %[port],%[high]\n\t"
                "out %[port],%[low]\n\t"
                "out %[port],%[high]\n\t"
                : : [port] "I" (_SFR_IO_ADDR(PORTC)), [low] "r" (low), [high] "r" (high)
                : "memory");
            uint8_t ok=PINC & ACK;SREG=saved;
            if(ok) break;
            _delay_us(1);
            if(++tries==0) return done;
        }
        ++done;
    }
    return done;
}
void giga_wire_status(uint8_t *status, uint16_t *bank) {
    /* Advisory snapshot, not an event queue: rapid intervening bank changes
     * can be missed. A scheduler cannot infer a complete trace from this. */
    uint8_t hi1,hi2,lo;
    input(CMD0);hi1=PIND & 63;
    command(0);lo=PIND;
    command(CMD0);hi2=PIND & 63;
    if(hi1!=hi2) { command(0);lo=PIND; }
    *bank=((uint16_t)hi2<<8)|lo;
    /* The retry above leaves command 000 selected. With CCLK low that
     * command returns ID A7, whose bit 7 would look like a sticky fault.
     * Always restore command 001 before sampling the actual status. */
    command(CMD0);
    PORTC &= ~CLK;
    __asm__ __volatile__("nop\n\tnop" ::: "memory");
    *status=PIND;PORTC |= CLK;
}

uint8_t giga_mailbox_query(uint16_t *bank, uint8_t *epoch) {
    input(0);PORTC &= ~CLK;
    __asm__ __volatile__("nop\n\tnop" ::: "memory");
    uint8_t state=PIND;PORTC |= CLK;
    if((state & 0xfc)!=0xa8)return 255;
    if((state & 1)==((state>>1)&1))return 0;
    __asm__ __volatile__("nop\n\tnop" ::: "memory");
    uint8_t lo=PIND;
    command(CMD0);uint8_t hi=PIND & 63;
    command(0);PORTC &= ~CLK;
    __asm__ __volatile__("nop\n\tnop" ::: "memory");
    uint8_t check=PIND;PORTC |= CLK;
    if(check!=state)return 0; /* Reset or a changed epoch: resample later. */
    *bank=((uint16_t)hi<<8)|lo;*epoch=state & 1;return 1;
}

uint8_t giga_mailbox_ack(uint16_t bank, uint8_t epoch) {
    uint16_t observed;uint8_t sequence;
    if(giga_mailbox_query(&observed,&sequence)!=1 || observed!=bank || sequence!=epoch)return 0;
    command(CMD0);PORTC &= ~CLK;
    __asm__ __volatile__("nop\n\tnop" ::: "memory");
    PORTC |= CLK;
    /* Avoid acknowledging another request when merely reading status. */
    command(0);return 1;
}
