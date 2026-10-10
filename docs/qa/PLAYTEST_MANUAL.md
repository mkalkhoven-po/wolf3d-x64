# Manual playtest checklists

Use with **your** WL6/WL1 folder. File defects as **type:bug** with severity labels. Append findings to epic playtest notes or GO_NO_GO residual risk.

**Controls cheat sheet:** [Player quickstart](../player/QUICKSTART.md)

## Q2 — Present & data (#49)

| # | Step | Pass? | Bug # |
|---|------|-------|-------|
| 1 | `wolf3d.exe --data <good>` starts, title visible | | |
| 2 | `wolf3d.exe --data <bad>` exits non-zero, clear error | | |
| 3 | Enter → menu; Up/Down; Esc back/title | | |
| 4 | Quit path clean (no zombie process) | | |
| 5 | F11 fullscreen toggle + restore | | |
| 6 | +/- integer scale, aspect not stretched wrong | | |
| 7 | Alt-tab out/in, no crash | | |

## Q3 — World & combat (#53)

| # | Step | Pass? | Bug # |
|---|------|-------|-------|
| 1 | New Game → map walk, walls collide | | |
| 2 | Open unlocked door with E | | |
| 3 | Locked door blocked without key; works with key | | |
| 4 | Pickup ammo/health/treasure | | |
| 5 | Kill at least one guard / dog | | |
| 6 | CTRL shoot / weapon keys | | |
| 7 | Die → R respawn with lives | | |
| 8 | Elevator → level complete → next map | | |

**Thin any% path (no mop-up):** doors → elevator exit is enough for “map completable”; SCORE may stay 0.  
Step-by-step + fidelity snapshot: [E1M1_ANYPERCENT_REFERENCE.md](E1M1_ANYPERCENT_REFERENCE.md).

## Q4 — Audio & AppData (#57)

| # | Step | Pass? | Bug # |
|---|------|-------|-------|
| 1 | Hear pistol / door digi (device on) | | |
| 2 | Menu or in-game music audible when enabled | | |
| 3 | sfx/music off or silent still runs | | |
| 4 | Save Game (F2) with a typed name creates `%APPDATA%\Wolf3DWin64\savegam<slot>.WL6` (or `.WL1`); F8 then saves there at once | | |
| 5 | Load Game (F3) / F9 restores the level exactly (actors, doors, pickups, position) | | |
| 6 | config.ini updated (scale/fullscreen/last_data) | | |
| 7 | No need to write next to Program Files | | |

## After each session

1. Note OS build, commit SHA, data extension (WL6/WL1).  
2. File bugs immediately — do not rest in chat only.  
3. Update [FR_TEST_MATRIX.md](FR_TEST_MATRIX.md) status when evidence is solid.
