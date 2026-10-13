/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef FG_GIGA_AUX_H
#define FG_GIGA_AUX_H
#include "giga.h"
#define GA_EXTENTS 3
typedef struct { DWORD first, count; } GigaAuxExtent;
typedef struct {
    GigaAuxExtent extent[GA_EXTENTS];
    DWORD pages;
    BYTE count;
} GigaAux;
/* Open an existing sector-aligned profile on the mounted FAT16/FAT32 volume.
 * Fully validate its live chain; keep at most three physical-sector extents.
 * Changes the single Petit FatFs open-file state, not the prepared ROM map.
 * Uses a caller-owned 512-byte FAT sector cache during preparation.
 * Any failure leaves out->count zero. Scratch contents are destroyed.
 * No disk writes. Preparation is not safe concurrently with another reader.
 * Map lifetime ends on filesystem mutation, remount or card replacement. */
BYTE giga_aux_open(FATFS *,const char *path,GigaAux *out,BYTE *scratch);
/* Helper appended to the pinned Petit FatFs in the development build. */
DWORD pf_giga_fat(FATFS *,DWORD cluster);
/* One direct 512-byte read; no FAT access or dependency on pf_open state.
 * Only use a successfully prepared, unmodified map. Output is undefined on
 * I/O failure. Return GI_*; no cartridge identity or payload CRC check here. */
BYTE giga_aux_read(const GigaAux *,DWORD page,BYTE *sector);
#endif
