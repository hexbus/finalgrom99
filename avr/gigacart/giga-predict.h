/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef FG_GIGA_PREDICT_H
#define FG_GIGA_PREDICT_H
#include "giga-aux.h"
enum { GP_OFF,GP_DIRECTORY,GP_RECORD,GP_POOL,GP_READY };
enum { GP_OK,GP_MORE,GP_IO,GP_FORMAT };
enum { GP_LEAD=8,GP_BRANCHES=8,GP_TARGETS=16 };
typedef struct {
    const GigaAux *file;
    WORD records,words,bank,index,end,pool,next_pool;
    WORD window_first,window_last,sequential_last;
    WORD targets[GP_TARGETS];
    BYTE phase,branches,truncated;
} GigaPredict;
/* Offline only: validate FGPR0001 identity, body CRC and sector-aligned file.
 * Existing unaligned development models must be zero padded to 512 bytes.
 * Lookup uses the caller's shared scratch, not a private sector cache.
 * There are no game-specific constants or bank paths here.
 */
BYTE giga_predict_open(GigaPredict *,const GigaAux *,const BYTE image_sha[32],BYTE *scratch);
void giga_predict_begin(GigaPredict *,WORD bank);
/* At most one SD metadata sector per call; preserves progress between calls.
 * May be interleaved with demand service that destroys scratch contents.
 * On failure disable prediction; demand remains the correctness fallback.
 */
BYTE giga_predict_step(GigaPredict *,BYTE *scratch);
/* Same successor set is valid throughout this interval. Reuse avoids a new
 * metadata scan for each sequential bank. Returns zero outside the interval.
 * Leading duplicate current-bank entries are harmless already-resident hits.
 * Branch overflow drops low-priority predictions, never expands RAM use.
 */
BYTE giga_predict_targets(GigaPredict *,WORD bank);
#endif
