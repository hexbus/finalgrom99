# Cooperative loader source index

These **25 files were added after the upstream reference** in the [TI index](../../ti/index.md#how-origin-is-classified). For runtime and retained-input status, read the [firmware guide](../../docs/development/architecture.md). For staging and compilation, read the [build guide](../../docs/development/building.md). Other sources are in the [AVR index](../index.md).

## Session entry and control

| File | Responsibility |
| --- | --- |
| [loader-giga.inc](loader-giga.inc) | Recognize large-image selections and dispatch the selected loading path. |
| [video-session.inc](video-session.inc) | Set up and service the cooperative session for prepared video and game cartridges. |
| [cooperative-fault.inc](cooperative-fault.inc) | Save cooperative-session fault reports to a preallocated file. |

## Storage, loading and mapping

Headers define interfaces and caller preconditions.

| Implementation / header | Responsibility |
| --- | --- |
| [giga.c](giga.c) / [giga.h](giga.h) | Build/reuse an SD extent index and read image sectors. |
| [giga-aux.c](giga-aux.c) / [giga-aux.h](giga-aux.h) | Open and read companion files. |
| [giga-launch.c](giga-launch.c) / [giga-launch.h](giga-launch.h) | Validate and install the startup bank description. |
| [giga-overlay.c](giga-overlay.c) / [giga-overlay.h](giga-overlay.h) | Apply prepared patches to sectors as they load. |
| [giga-resume.c](giga-resume.c) / [giga-resume.h](giga-resume.h) | Track partially loaded banks and publish/retire cache mappings. |
| [giga-fixed.c](giga-fixed.c) / [giga-fixed.h](giga-fixed.h) | Check fixed-bank ranges and resolve their slots. |
| [giga-demand.c](giga-demand.c) / [giga-demand.h](giga-demand.h) | Service explicit requests and manage the current reservation. |
| [giga-wire.c](giga-wire.c) / [giga-wire.h](giga-wire.h) | Transfer bytes and exchange mailbox messages with the CPLD. |
| [giga-predict.c](giga-predict.c) / [giga-predict.h](giga-predict.h) | Read and validate optional prediction data. |
| [giga-prefetch.c](giga-prefetch.c) / [giga-prefetch.h](giga-prefetch.h) | Schedule speculative loading without disturbing the active bank. |

## Shared definitions

| File | Responsibility |
| --- | --- |
| [giga-cache.h](giga-cache.h) | Shared cache status definitions used by the resumable cache. |
| [giga-status.h](giga-status.h) | Format error messages for the menu status field. |
