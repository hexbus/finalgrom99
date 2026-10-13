/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef FG_GIGA_FIXED_H
#define FG_GIGA_FIXED_H
#include "giga-resume.h"
/* External startup manifest: contiguous logical banks in contiguous physical
 * slots. A validated catalog may be used ONLY after complete payloads/patches
 * and their mappings were successfully installed offline. The catalog is an
 * ownership claim, not a hardware probe. Keep it immutable until reset. */
typedef struct { WORD bank,count; BYTE first; } GigaFixedRange;
typedef struct { const GigaFixedRange *ranges; BYTE count; } GigaFixed;
/* Sorted, non-overlapping logical ranges; disjoint physical ranges outside
 * the dynamic pool and reserved slots 0/125/126/127. */
BYTE giga_fixed_check(const GigaFixed *,const GigaResume *);
/* Validated catalog only. Returns physical slot or 255 when not fixed. */
BYTE giga_fixed_slot(const GigaFixed *,WORD bank);
#endif
