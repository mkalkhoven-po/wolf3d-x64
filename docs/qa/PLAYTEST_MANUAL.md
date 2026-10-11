# Manual playtest checklists

Use with **your** WL6/WL1 folder. File defects as **type:bug** with severity labels. Append findings to epic playtest notes or GO_NO_GO residual risk.

**Controls cheat sheet:** [Player quickstart](../player/QUICKSTART.md)

The **Evidence** column says how an item is covered: an automated test (CTest name), a maintainer playtest (date), or open.

## Q2 — Present & data (#49)

| # | Step | Pass? | Evidence |
|---|------|-------|----------|
| 1 | `wolf3d.exe --data <good>` starts: rating screen, title, credits, high scores, demo | Pass | `test_orig_menu` (attract loop); maintainer 2026-10-11 |
| 2 | `wolf3d.exe --data <bad>` exits non-zero with a clear error | Pass | `bad_data` |
| 3 | Any key → Options; arrows / mouse / wheel / pad; Esc back | Pass | `test_orig_menu`; maintainer 2026-10-11 |
| 4 | Quit (Y) or closing the window ends cleanly | Pass | `test_orig_menu` (Quit); maintainer playtests |
| 5 | F11 / Video → Fullscreen toggles and restores | Pass | maintainer 2026-10-11 |
| 6 | Video → Fill the screen: 4:3, not stretched wrong | Pass | maintainer 2026-10-11 |
| 7 | Alt-tab out and in, no crash | | open |

## Q3 — World & combat (#53)

| # | Step | Pass? | Evidence |
|---|------|-------|----------|
| 1 | New Game → episode and difficulty → the floor; walls collide | Pass | `test_orig_menu` (New Game); `orig_player` |
| 2 | Open a door with Space | Pass | `orig_player` (Cmd_Use); maintainer playtests |
| 3 | Locked door: closed without the key, opens with it | Pass | `orig_player` (Cmd_Use: locks) |
| 4 | Pick up ammo, health, treasure (with their AdLib sounds) | Pass | `orig_player`; maintainer ear check (P4 audio) |
| 5 | Kill a guard / dog | Pass | `orig_ai`; demos in sync (`orig_fidelity`) |
| 6 | Ctrl shoots, 1–4 change weapons | Pass | `orig_player`, `orig_loop` |
| 7 | Die: the floor restarts with a life less; out of lives → high scores | Pass | `orig_loop` (GameLoop); maintainer 2026-10-11 |
| 8 | Elevator → intermission → next floor; secret exit → secret floor | Pass | `orig_loop`, `test_orig_menu`; maintainer 2026-10-11 |

**Thin any% path (no mop-up):** doors → elevator exit is enough for “map completable”.
Step-by-step + fidelity snapshot: [E1M1_ANYPERCENT_REFERENCE.md](E1M1_ANYPERCENT_REFERENCE.md); the `orig_fidelity` CTest walks it.

## Q4 — Audio & AppData (#57)

| # | Step | Pass? | Evidence |
|---|------|-------|----------|
| 1 | Hear the pistol and doors (digitized) and the pickups (AdLib) | Pass | `smoke_only` (digitized map per pack, AdLib effect); maintainer ear check |
| 2 | Music in the menus and on every floor | Pass | `smoke_only` (music keys notes); level music table identical to the original; maintainer ear check |
| 3 | Sound → Sound devices: effects, digitized and music off still runs, silent | | `smoke_only` (each switch); open: hear it once |
| 4 | Sound → Music / Effects volume: each on its own, 0 is silent | Pass | `smoke_only` (the mixer at 0 and at full); maintainer 2026-10-10 |
| 5 | Save Game (F2) with a typed name creates `%APPDATA%\Wolf3DWin64\savegam<slot>.WL6` (or `.WL1`); F8 then saves there at once | Pass | `test_orig_menu` (save, F8) |
| 6 | Load Game (F3) / F9 restores the level exactly (actors, doors, pickups, position) | Pass | `orig_loop_maps` (a save halfway through a demo ends identically) |
| 7 | config.ini keeps the settings (scale, fullscreen, menus, last_data) | Pass | `smoke_only` (config roundtrip); `cold_start` |
| 8 | No need to write next to Program Files; no admin rights | Pass | `cold_start`; all runs non-elevated (2026-10-11) |

## After each session

1. Note OS build, commit SHA, data extension (WL6/WL1).
2. File bugs immediately — do not rest in chat only.
3. Update [FR_TEST_MATRIX.md](FR_TEST_MATRIX.md) status when evidence is solid.
