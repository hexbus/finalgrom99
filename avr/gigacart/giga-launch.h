/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef FG_GIGA_LAUNCH_H
#define FG_GIGA_LAUNCH_H
#include "giga-aux.h"
enum { GL_OK,GL_IO,GL_FORMAT,GL_CRC,GL_WRITE };
typedef struct { BYTE policy,first,count,grom;WORD bridge_bytes;DWORD runtime,bridge_page; } GigaLaunch;
typedef BYTE (*GigaLaunchWrite)(BYTE slot,BYTE sector,const BYTE *data);
/* Offline only: slot 0 must already be cleared, and the console must not
 * execute cartridge code. Uses an existing prepared ROM plus profile map.
 * Profile CRCs validate all copied ROM ranges; full image identity remains
 * the installer/profile compiler's responsibility. No title-specific logic.
 * On success, the final bridge sector remains in buffer, NOT written to SRAM.
 * Caller validates the policy, initializes runtime state, sets GROM mask,
 * then publishes exactly bridge_bytes to slot 0. Never publish on failure.
 * No allocations; no filesystem writes. Profile and ROM maps must be live. */
BYTE giga_launch_load(const GigaAux *,DWORD image_bytes,GigaLaunch *,BYTE *buffer,GigaLaunchWrite);
#endif
