/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef FG_GIGA_OVERLAY_H
#define FG_GIGA_OVERLAY_H
#include "giga-aux.h"
/* FGOV0001 external sector replacements; original image stays read-only.
 * One sector buffer, one last-bank directory cache. No cartridge constants.
 * CRCs detect mismatches/corruption, not malicious profile substitution.
 * Installer establishes whole-image SHA and adapter coverage separately. */
typedef struct { const GigaAux *file; DWORD base; WORD bank,mask,banks; } GigaOverlay;
enum { GO_OK,GO_IO,GO_FORMAT,GO_CRC,GO_IMAGE };
BYTE giga_overlay_open(GigaOverlay *,const GigaAux *,DWORD image_bytes,BYTE *buffer);
BYTE giga_overlay_read(GigaOverlay *,WORD bank,BYTE sector,BYTE *buffer);
#endif
