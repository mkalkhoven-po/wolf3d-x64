# E8 — Win11 productization

## Goal
Make the build comfortable as a daily desktop app.

## User stories
- As a player, my settings and saves land in AppData.
- As a player, I can toggle fullscreen and integer scales.
- As a player, DPI/scaling does not destroy aspect incorrectly.

## Functional requirements
- FR-DATA-004
- FR-SAVE-001, FR-SAVE-002
- FR-VID-003–005
- FR-PLAT-002, FR-PLAT-004

## Exclusions
- Installer/MSI (zip + readme OK)
- Code signing (optional later)
- Store submission

## Acceptance
- [x] AppData config/saves verified (`%APPDATA%\Wolf3DWin64\`)
- [x] Fullscreen toggle + windowed restore (F11)
- [x] Integer scale windowing (`RenderSetIntegerScale`, +/- keys)
- [x] DPI awareness enabled before window create
- [x] README player quickstart complete
- [x] No primary writes required next to the exe (uses AppData)

## Controls (desktop)
| Input | Action |
|-------|--------|
| F11 | Toggle desktop fullscreen |
| +/- | Integer window scale 1..8 |
| F5 / F9 | Save / load slot 0 |
| Esc | Back / quit |

## Files
- `src/platform/win_sdl/wolf_userdata.c` — AppData config, high scores, saves
- `src/platform/win_sdl/wolf_present.c` — DPI + integer-scale present helper
- `docs/player/QUICKSTART.md` — player-facing run notes
