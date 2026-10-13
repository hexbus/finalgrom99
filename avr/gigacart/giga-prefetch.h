/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef FG_GIGA_PREFETCH_H
#define FG_GIGA_PREFETCH_H
#include "giga-demand.h"
/* One cooperative service quantum. Always checks demand first. Targets are
 * advisory logical banks in decreasing priority, supplied by an external
 * model. No selected/leased payload can be retired or overwritten by this
 * function. Count must be <=254; fixed banks may appear in the list.
 *
 * One explicit execution lease suffices only for adapted callers whose next
 * COMMIT is wholly in RAM. Code/return banks used without another COMMIT must
 * be permanently fixed. This does not make unadapted ROM accesses safe.
 *
 * At most one payload-sector or complete mapping operation per call. A new
 * COMMIT arriving during that operation is serviced on the next call. SD or
 * transfer failure fails closed, including when encountered speculatively.
 */
BYTE giga_prefetch_service(GigaDemand *,GigaResume *,BYTE *buffer,
    const GigaFixed *,GigaSectorRead,const WORD *targets,BYTE count);
#endif
