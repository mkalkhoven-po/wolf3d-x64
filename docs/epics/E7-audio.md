# E7 — Audio

## Goal
SDL audio callback with digitized sounds; AdLib music via OPL emulator when scheduled.

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

## Acceptance
- [ ] Silent mode stable
- [ ] Digi recognisable for weapons
- [ ] Music path works or explicitly deferred with issue link if timebox hits
