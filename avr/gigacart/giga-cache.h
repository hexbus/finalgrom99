/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef FG_GIGA_CACHE_H
#define FG_GIGA_CACHE_H
#include "giga.h"
enum { GC_EMPTY, GC_FILLING, GC_READY, GC_PINNED, GC_BROKEN };
enum { GC_OK, GC_MORE, GC_DONE, GC_STATE, GC_DISK, GC_WIRE };
/* Caller-owned metadata, separate from scratch[0..127] and scratch[512..1023]. */
typedef struct { WORD bank; BYTE state; } GigaSlot;
typedef struct {
    GigaSlot *slots;
    BYTE first, count, pending, sector;
} GigaCache;
/* Offline prerequisite: these slots have no valid mappings and cannot be
 * selected by the TI. Excludes shim/table slots 125..127. */
BYTE giga_cache_init(GigaCache *, GigaSlot *, BYTE first, BYTE count);
/* Logical bank must have no cached copy outside this pool. A separate,
 * immutable diagnostic miss sentinel is allowed. Objects/buffers must
 * remain alive; no concurrent cache/index calls. */
BYTE giga_cache_begin(GigaCache *, BYTE index, WORD bank);
/* One indexed SD sector, then acknowledged online SRAM fill. The caller must
 * guarantee the destination physical slot remains unlatched and has no
 * valid mapping throughout the payload fill, until final publication.
 * Last sector publishes the complete bank; TI must reselect to latch it. */
BYTE giga_cache_step(GigaCache *, BYTE *sector_buffer);
BYTE giga_cache_pin(GigaCache *, BYTE index);
/* CONTRACT: caller proves the bank is no longer latched and cannot be
 * requested until reloaded. A sampled bank number is NOT that proof.
 * No automatic LRU/eviction or game policy is implemented here. */
BYTE giga_cache_retire(GigaCache *, BYTE index);
/* Abandon a failed/unpublished fill. The mapping was never published. */
BYTE giga_cache_discard(GigaCache *, BYTE index);
#endif
