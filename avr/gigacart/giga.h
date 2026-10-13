/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef FG_GIGA_H
#define FG_GIGA_H
#include "lib/pff.h"
enum { GI_OK=0, GI_IO=1, GI_FORMAT=2, GI_IMAGE=3, GI_CHAIN=4,
       GI_SPACE=5, GI_DIRECTORY=6, GI_COLLISION=7, GI_READONLY=8 };
typedef struct { DWORD extents, fat_reads; BYTE reused; } GigaReport;
#define GIGA_RAM_EXTENTS 16
#define GIGA_MAP_BYTES (GIGA_RAM_EXTENTS * 8)
extern GigaReport giga_report;
/* scratch is 1024 bytes; path is writable and its extension is restored.
 * Supports uppercase GCR/GCS/GCG and BIN. BIN allocation sidecars use BIX;
 * that is storage indexing only, not recognition of a supported mapper/game. */
BYTE giga_prepare(FATFS *fs, char *path, BYTE *scratch);
/* Available only after successful preparation, before another disk mutation.
 * Forwards 8192 bytes through disk_readp(NULL,...), like the existing loader. */
BYTE giga_read_bank(WORD bank);
/* Buffered sector read for online transfers. dest is a full 512-byte buffer;
 * use scratch+512, never the reserved map at scratch[0..127]. No SRAM writes.
 * No concurrent preparation, filesystem mutation or other index user. */
BYTE giga_read_sector(WORD bank, BYTE sector, BYTE *dest);
BYTE giga_map_in_ram(void);
BYTE giga_get_extent(DWORD index, DWORD *first, DWORD *clusters);
/* Read-only helper appended to the pinned Petit FatFs during the build. */
FRESULT pf_giga_slot(const char *, DWORD *, WORD *, BYTE *, BYTE *);
#endif
