# E7 — Audio

## Goal
SDL audio callback with digitized sounds; AdLib/IMF music via lightweight OPL-style emulator.

## User stories
- As a player, shooting and enemy alerts make sounds.
- As a player, menu/game music plays when music is enabled.
- As a player, missing audio device still allows silent play.

## Functional requirements
- FR-AUD-001–003

## Exclusions
- Hardware SB/AdLib detection UX beyond On/Off
- Surround/HRTF
- PC speaker authenticity if digi+OPL cover UX
- Full nuked-OPL fidelity (simple square-wave IMF voice)

## Acceptance
- [x] Silent mode stable (`SD_SetSilent` / missing device)
- [x] Digi recognisable for weapons/doors (VSWAP digi list, 46 samples)
- [x] Music path loads AUDIOT IMF and starts/stops (menu intro + in-game track)
- [x] Gameplay events call `SD_PlaySound` (shoot, door, pickup, death)

## Implementation notes
- `src/id/id_sd.c` — SDL float mixer, digi voices, IMF→OPL-ish synth
- `src/id/audio_wl6.h` — sound/music name indices
- Digi table from last VSWAP page (original `SDL_SetupDigi` algorithm)
- DigiMap from classic wolf digimap (weapon/door/actor cues)
