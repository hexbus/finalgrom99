/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef FG_GIGA_WIRE_H
#define FG_GIGA_WIRE_H
#include <stdint.h>
/* Development interface for finalgrom_giga.vhdl. Caller has already loaded
 * bank zero, shim and page table offline, set config bit 2 and entered RUN.
 * This transport is NOT enabled in the released preparation/player firmware. */
uint8_t giga_wire_identify(void);
void giga_wire_address(uint32_t address);
/* Sends one byte with a bounded retry count; failed attempts do not advance
 * SRAM address. Returns 1 on success, 0 when console activity prevents a grant.
 * ADDRESS must be issued before entering a new stream. Never alter CCMD or
 * CDX, or suspend CCLK, inside the six-instruction pulse train. */
uint8_t giga_wire_byte(uint8_t value);
/* Transfer a bounded block; returns the acknowledged prefix length. The
 * caller must not publish a bank after a short transfer. Same pulse train,
 * with port constants reused and interrupts restored between attempts. */
uint16_t giga_wire_block(const uint8_t *data, uint16_t length);
void giga_wire_status(uint8_t *status, uint16_t *bank);
/* Cooperative prototype only (CPLD ENABLE_MAILBOX build, giga mode).
 * query: 0 idle, 1 pending, 255 wrong interface. A pending result is stable
 * until ACK. ACK must follow complete publication AND a held reservation.
 * Never call legacy giga_wire_status while cooperative mode is enabled. */
uint8_t giga_mailbox_query(uint16_t *bank, uint8_t *epoch);
uint8_t giga_mailbox_ack(uint16_t bank, uint8_t epoch);
#endif
