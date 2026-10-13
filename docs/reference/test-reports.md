# Beta 1 test reports

[Start here](../README.md) · [Known playback issues](../troubleshooting.md#known-playback-issues) · [Report a problem](../troubleshooting.md#report-a-problem)

This is the detailed evidence behind the player-facing issue summary. Read it when comparing a particular report or investigating a cause. A reported success describes the stated setup and test scope; it is not a complete compatibility certification. Hypotheses below remain separate from observed results.

The [release notes](../release-notes.md) describe feature support. File rebuild results are described in [adapter verification](../development/adapters.md#verification-and-hardware-status).

## Lapo Serafini: PAL console report

Recorded on 2026-10-13 from the supplied tester report and follow-up reply. This is the recording date; the exact test date was not supplied. The photographs have 2026-10-12 filenames. The original forum post URL was not supplied.

**Reported setup:** PAL TI-99, 32K expansion, speech synthesizer, Fabrizio Corpetti SCART cable, and a 23.5-inch Samsung TV/monitor. Play used the keyboard. Tester described Beta 1 with the latest files; the exact AVR/CPLD/menu hashes, companion revisions, console board revision, SD card and previous firmware version were not supplied. The description alone does not identify the 125 ms build.

The follow-up reply identifies the main game as Stealth Runner and describes it as a 2 MiB game running through the preparation mechanism on the 1 MiB SRAM board. The tester excerpt itself does not name the main game or its exact image revision. Filename and hashes remain to be confirmed.

### Reported observations

| Area | Tester observation | Evidence status |
| --- | --- | --- |
| Main game session | About 90 minutes of play; keyboard controls consistently responsive. | User-reported hardware result; not a complete-game qualification. |
| Main game audio | First spoken sentence garbled and unintelligible; later speech fine. No other sound-effect problems noticed. | Opening speech remains an open defect, distinct from general audio quality. |
| Main character | Minor sprite glitches, with three linked photographs. | User-reported; no visual diagnosis established here. |
| Loading | One freeze across approximately 20-30 loads. | Open intermittent defect. Exact title, stage and recovery method unspecified; not necessarily a gameplay freeze. |
| Bad Apple | Speech slightly distorted. | Consistent with an existing reported symptom; exact variant not identified. |
| Spaceballs | Dialogue not perfectly clear. | Reported audio-quality issue; cause unresolved. |
| Compilations | About ten games tested, all working without freezes or other noticed problems. | Useful spot checks; exact titles and compilation revisions not listed. |
| Ghostbusters | Previously troublesome on this FinalGROM; now reportedly runs well after the update. | Before/after user report, not a controlled firmware comparison. |
| Don't Mess with Texas | Previously troublesome on this FinalGROM; now reportedly runs well after the update. | Before/after user report, not a controlled firmware comparison. |

The last two reports support improved compatibility on this tester's setup. Their ROM sizes, previous symptoms and responsible firmware changes are unknown. They do not establish a size-specific fix above 512 KiB or that the original cartridge hardware was faulty.

### Comparison reports and unresolved causes

A separate development report describes reproducing Bad Apple audio problems using a PPEB with different behavior under emulation. This supports investigating a problem beyond the FinalGROM streaming path. The suggestion of a physical-hardware timing issue is a hypothesis; it does not establish that every physical console is affected. PAL/NTSC timing, image revision, audio path and the exact comparison setup still need to be controlled.

The follow-up reply recalls similar Stealth Runner sprite glitches on some 2 MiB boards, with the cause unknown. A shared defect has not been established. Matching image revisions and the same scene would help distinguish an existing game issue from a preparation or loading regression.

The follow-up reply describes Spaceballs and Looney Tunes audio as 1-bit PCM and suggests increasing volume. This explanation has not been verified against the recording. Low-fidelity source audio may explain limited clarity; it does not rule out additional playback distortion or establish that volume changes cure it. Lapo's excerpt contains no Looney Tunes test result.

The garbled first Stealth Runner sentence should be investigated separately from later speech and from the video titles' sampled audio. Speech initialization or startup timing is a possible direction, not a demonstrated cause. Correct later speech does not establish that the opening defect is resolved.

### Evidence links and next comparisons

The linked audio and photographs have not been independently reviewed. The observations above are based on the written report.

- [Opening audio attachment](https://forums.atariage.com/applications/core/interface/file/attachment.php?id=1379520&key=d29b91a64fc0474a8428855fbeef5a78)
- [Sprite photograph 1](https://media.invisioncic.com/r322239/monthly_2026_10/20261012_090946.jpg.9af1570c434b0e247b54119eadfbf01c.jpg)
- [Sprite photograph 2](https://media.invisioncic.com/r322239/monthly_2026_10/20261012_091002.jpg.0e951aeee94557c39a67cf4b40f299ff.jpg)
- [Sprite photograph 3](https://media.invisioncic.com/r322239/monthly_2026_10/20261012_091013.jpg.d657b35e51466fdeef65f1cc3f412fea.jpg)

For a follow-up, collect the exact release package and cartridge set, SD make/model, names of the compilation games, and which title and screen froze. A photograph of any error indicator and whether reset recovered would make a recurrence more useful. Record cold power-on versus reset for the opening speech comparison.

Reproduce the same opening sentence and sprite scene on the same PAL console with the prepared set and, where available, the corresponding native cartridge. Compare Spaceballs with the same clip on a known reference playback path. Keep those observations separate from Bad Apple's speech result. No firmware change or root-cause conclusion is justified by this report alone.

## Dragon's Lair playback reports

Recorded on 2026-10-13; exact test dates were not supplied. Two reports describe different playback experiences:

| Report | Observation | Unresolved details |
| --- | --- | --- |
| Development testing | Dragon's Lair works well under Beta 1 with **very slight stutter**, not enough to interrupt gameplay. | No measured pause durations or complete-game qualification. |
| Separate beta tester | The game plays but is much more jittery than an actual cartridge. | Installed AVR/CPLD hashes, card make/model, image and companion hashes, and comparable video evidence remain unconfirmed. |

The supplied Beta 1 package was reported to be the same in both cases; the installed files have not been independently compared. The heavier jitter applies to the second report, not both setups.

Requested follow-up testing uses a freshly formatted FAT32 card, possibly a second card type, matching companions including GPR, and the supplied menu. Let FinalGROM rebuild GIR/GIS indexes for the destination card. Confirm both firmware versions and compare the same intro or gameplay sequence; Spaceballs and other video playback provide additional comparison points. Results remain pending.

The development report attributes continued correct gameplay to the overlay's waits. Cooperative software requests and waits are part of this preparation, but that observation does not establish physical TI READY wait-state control. The contribution of SD access, card layout, prediction or other causes to individual pauses remains unresolved. [Companion rebuild checks](../development/adapters.md#verification-and-hardware-status) establish file identity, not playback latency.
