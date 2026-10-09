# Player quickstart (Windows 11)

## What you need
1. A build of `wolf3d.exe` (and `SDL2.dll` beside it).
2. **Your own** Wolfenstein 3D data folder (WL1 or WL6).  
   Example local drop used in development: `D:\wolf3d-master\Wolf3dgame`

This project **never** ships game data.

## Run (easiest: double-click)

Layout that works when you double-click `build\wolf3d.exe`:

```
wolf3d-master\
  Wolf3dgame\          ← your VSWAP.WL6, etc. (gitignored)
  build\
    wolf3d.exe
    SDL2.dll
```

The game looks for:

1. `--data` / `WOLF3D_DATA`
2. `last_data` in `%APPDATA%\Wolf3DWin64\config.ini`
3. `Wolf3dgame` or `data` in the **current folder**
4. Same names **next to the .exe** and **one folder up** (`..\Wolf3dgame`)

If data is still missing, a **message box** explains how to fix it (window no longer just flashes closed).

### Explicit path (always works)
```bat
cd /d D:\wolf3d-master
set WOLF3D_DATA=D:\wolf3d-master\Wolf3dgame
build\wolf3d.exe --data "%WOLF3D_DATA%"
```

### Play a level
1. Double-click `build\wolf3d.exe` (with pack as above), or use `--data`.
2. **Enter** on the title → main menu.
3. **NEW GAME** → Enter → map 0.

Smoke harness (no full play loop):
```bat
scripts\smoke_wl6.bat
```
or:
```bat
build\wolf3d.exe --data "%WOLF3D_DATA%" --smoke-only
```

## Faithful engine preview (`--engine=orig`)

The faithful port of the original engine (fixed point, 70 Hz, original AI and
renderer; see ADR-002) can already be played. Menus, the intermission
screens and the title/demo loop arrive in P5, so for now it starts straight
into a level and **Esc quits**.

```bat
build\wolf3d.exe --data "%WOLF3D_DATA%" --engine=orig
build\wolf3d.exe --data "%WOLF3D_DATA%" --engine=orig --map 10 --skill 3
build\wolf3d.exe --data "%WOLF3D_DATA%" --engine=orig --demo 0
```

| Option | Meaning |
|--------|---------|
| `--map N` | 0-59: episode × 10 + level (`--map 10` is episode 2, floor 1). WL1 has maps 0-9 |
| `--skill 0-3` | Can I play, Daddy? … I am Death incarnate! (default 2) |
| `--viewsize 4-20` | View size as in the original (default 15; 20 fills the play area) |
| `--demo 0-3` | Watch one of the pack's demos, then quit |
| `--screenshot N file.bmp` | Save frame N as an 8-bit BMP (handy for bug reports) |
| `--frames N` | Stop after N frames; with `--fast` (no clock) used by the smoke tests |

Controls are the original defaults:

| Key | Action |
|-----|--------|
| Arrows | Move / turn |
| Ctrl | Shoot |
| Alt + Left/Right | Strafe |
| Right Shift | Run |
| Space | Open door / elevator |
| 1-4 | Knife, pistol, machine gun, chain gun |
| Pause | Pause |
| F11 | Fullscreen toggle |
| Esc | Quit (until the menus land in P5) |

The mouse turns and moves when `mouse_look` is on in `config.ini`. `scale`,
`fullscreen`, `music_on` and `sfx_on` apply as for the default engine. When
you die, BJ turns towards the killer and the level restarts; out of lives
ends the run.

## Where settings live
```
%APPDATA%\Wolf3DWin64\
  config.ini        scale, fullscreen, mouse, audio toggles, last_data
  highscores.dat
  save0.w64 …       modern save slots (F5/F9 in-game)
```

The game does **not** need write access next to Program Files.

## Desktop controls
| Key | Action |
|-----|--------|
| Enter | Title → menu / menu select |
| WASD / arrows | Move / turn |
| Ctrl or LMB | Shoot |
| E / Space | Open door / elevator |
| F5 / F9 | Save / load slot 0 |
| F11 | Fullscreen toggle |
| + / - | Integer scale |
| Esc | Back / quit |

## Build (developers)
See root `README.md`. Typical MinGW path:
```bat
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
build\wolf3d.exe --data Wolf3dgame --smoke-only
```

## Support notes
- Missing data → non-zero exit, optional popup; `MISSING:` list on console when pack incomplete.
- No audio device → silent fallback (game still runs).
- Aspect is fixed **4:3** logical 320×200 with nearest-neighbor integer scale.
- Interactive launch does **not** run the full smoke suite (that is `--smoke-only` / `scripts/smoke_wl6.sh`).
