# ADR-003: Own OPL2 emulator for AdLib music and effects

## Status

Accepted (2026-10-09). Fills the "OPL" slot ADR-001 left open in `third_party/`.

## Context

Playtesting the faithful engine (P4/S5b) showed two audio gaps, both from having no AdLib emulation:

- Music was a square-wave stand-in that read the AdLib note frequencies and ignored the instruments: shrill, no envelopes, no FM timbre.
- Sound effects without a digitized sample (treasure, ammo, health, keys, extra life, menus, the player's death) are AdLib-only in the original and were silent.

The original sound manager (`ID_SD.C`, `ID_SD_A.ASM`) drives a Yamaha YM3812 (OPL2): music from the 700 Hz timer, effects on channel 0 at 140 Hz. Mature emulators exist (ymfm BSD-3 in C++, Nuked OPL3 LGPL-2.1, DOSBox and MAME cores under GPL), but `reference/original/` is under id's 1995 Limited Use license and [`EXCLUSIONS.md`](../product/EXCLUSIONS.md) rules out third-party GPL code without a documented license decision.

## Decision

1. **Write our own OPL2 emulator** (`src/id/opl2.c`), in C, from the chip's documented behaviour: log-sine and exponent tables generated from their formulas, envelope rates, key scaling, KSL, tremolo/vibrato LFOs, FM/additive channels with feedback, the four OPL2 waveforms. No code from other emulators.
2. **Close, not bit-exact.** Pitch, levels and envelope times are checked against the YM3812 data sheet in `tests/test_opl2.c`. Rhythm mode is left out: Wolfenstein 3-D never switches it on.
3. **Port the AdLib half of `ID_SD.C` as is** (`src/id/sd_adlib.c`, no SDL): `SDL_ALPlaySound`, the 140 Hz effect service, the 700 Hz sequencer, `SD_MusicOff`, the register reset of `SDL_DetectAdLib`. `SD_PlaySound` keeps the original's separate digitized and AdLib channels and priorities, `SD_SoundPlaying` reports the AdLib effect only, and digitized sounds keep the SB Pro stereo placement (`SetSoundLoc`).

## Consequences

- Music and AdLib effects sound like an AdLib/Sound Blaster, for both engines.
- `SD_WaitSoundDone` waits for real again, so the death sequence lasts as long as the original's.
- Sound timing is now part of the faithful engine: `UpdateFace` skips its `US_RndT` draw while the gatling pickup sound plays, so demo playback with sound on matches the original rather than a silent run.
- If accuracy turns out not to be good enough, a later ADR can revisit a third-party core; the interface (`OPL2_Write`, `OPL2_Sample`) is small enough to swap.
