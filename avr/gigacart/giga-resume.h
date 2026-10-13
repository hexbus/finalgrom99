/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef FG_GIGA_RESUME_H
#define FG_GIGA_RESUME_H
#include "giga-cache.h"
/* A partial payload occupies a whole physical slot. Values 0..15 count
 * completed sectors. Publication and retirement are separately retryable. */
enum { GR_PUBLISH=16, GR_READY=17, GR_RETIRE=18, GR_EMPTY=255 };
typedef struct { WORD bank; BYTE progress,rank; } GigaResumeSlot;
/* Single active session: 0 = full 14-bit key, 1 = 8-bit noninverted aliases.
 * Initialized offline; a mapping step completes all aliases before reuse. */
extern BYTE giga_resume_aliases;
typedef struct {
    GigaResumeSlot *slots;
    BYTE first,count,pending;
} GigaResume;
/* Offline prerequisite: no mapping/latch references these physical slots.
 * Logical banks in the pool must have no other cached copy. No scheduler,
 * victim selection, pinning, or console wait support is provided here. */
BYTE giga_resume_init(GigaResume *,GigaResumeSlot *,BYTE first,BYTE count);
BYTE giga_resume_begin(GigaResume *,BYTE index,WORD bank);
BYTE giga_resume_select(GigaResume *,BYTE index);
/* Pause only between payload sectors, never during publication/retirement. */
BYTE giga_resume_pause(GigaResume *);
/* At most one sector transfer or one complete mapping publication/retirement.
 * Ordinary 8-bit mode writes 64 aliases. Failed transfers do not
 * advance progress; retry overwrites the same unpublished sector. A failed
 * mapping operation must be retried, never discarded. */
BYTE giga_resume_step(GigaResume *,BYTE *buffer);
/* Optional external sector provider, e.g. checked on-load patch recipes.
 * It must return a complete final 512-byte sector or fail before publication.
 * No controller layout change; ordinary callers retain the original reader. */
typedef BYTE (*GigaSectorRead)(WORD bank,BYTE sector,BYTE *buffer);
BYTE giga_resume_step_read(GigaResume *,BYTE *buffer,GigaSectorRead);
/* Caller must prove the bank cannot be read throughout retirement/reuse:
 * either its physical slot is no longer latched and cannot be selected, or
 * an explicit cooperative lease release parks the CPU until a later safe
 * selection. A sampled bank number is not either of these proofs. */
BYTE giga_resume_retire(GigaResume *,BYTE index);
BYTE giga_resume_discard(GigaResume *,BYTE index);
#endif
