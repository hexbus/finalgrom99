/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef FG_GIGA_DEMAND_H
#define FG_GIGA_DEMAND_H
#include "giga-resume.h"
#include "giga-fixed.h"
/* Prototype controller: one execution lease. Background loading must honor
 * this lease (see giga-prefetch.h); service is serialized on the AVR.
 * COMMIT means the CPU is wholly in safe RAM and releases its old lease.
 * This permits recycling even a still-latched physical slot: the CPU must
 * make no payload reads until ACK followed by explicit SELECT.
 * Permanent code/return banks must be outside this managed pool.
 * Fixed requests use an immutable, offline-validated external catalog.
 * This entry point only services demand; it does not start predictions.
 */
typedef struct { WORD bank; BYTE epoch,active,lease,error; } GigaDemand;
enum { GD_IDLE,GD_PROGRESS,GD_ACK,GD_IO,GD_PROTOCOL };
enum { GD_FIXED_LEASE=254, GD_NO_LEASE=255 };
void giga_demand_init(GigaDemand *);
BYTE giga_demand_service(GigaDemand *,GigaResume *,BYTE *buffer);
/* Catalog and its mappings/payloads must be validated and installed before
 * RUN. No title-specific constants; NULL retains dynamic-only operation. */
BYTE giga_demand_service_fixed(GigaDemand *,GigaResume *,BYTE *buffer,const GigaFixed *);
BYTE giga_demand_service_source(GigaDemand *,GigaResume *,BYTE *buffer,const GigaFixed *,GigaSectorRead);
#endif
