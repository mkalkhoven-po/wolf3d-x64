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

### Play
1. Double-click `build\wolf3d.exe` (with the pack as above), or use `--data`.
   In PowerShell start it as `.\build\wolf3d.exe`.
2. **Enter** on the title → main menu.
3. **NEW GAME** → episode 1, floor 1 on "Bring 'em on", or **LOAD GAME** to
   continue from your quick save.

The game is a faithful port of the original engine (fixed point, 70 Hz, the
original AI and renderer; see ADR-002). The original menus (episode and skill
choice, sound, controls, save slots), the intermission screens and the
title/demo loop arrive in P5; until then the menu above is a stand-in, and
**Esc** in the game returns to it.

Smoke harness (no full play loop):
```bat
scripts\smoke_wl6.bat
```
or:
```bat
build\wolf3d.exe --data "%WOLF3D_DATA%" --smoke-only
```

### Starting the game directly

Any of these options skips the menu:

```bat
build\wolf3d.exe --data "%WOLF3D_DATA%" --map 10 --skill 3
build\wolf3d.exe --data "%WOLF3D_DATA%" --demo 0
build\wolf3d.exe --data "%WOLF3D_DATA%" --load 0
```

| Option | Meaning |
|--------|---------|
| `--map N` | 0-59: episode × 10 + level (`--map 10` is episode 2, floor 1). WL1 has maps 0-9 |
| `--skill 0-3` | Can I play, Daddy? … I am Death incarnate! (default 2) |
| `--viewsize 4-20` | View size as in the original (default 15; 20 fills the play area) |
| `--demo 0-3` | Watch one of the pack's demos, then quit |
| `--load 0-5` | Continue from a save slot (F8 saves to slot 0) |
| `--screenshot N file.bmp` | Save frame N as an 8-bit BMP (handy for bug reports) |
| `--frames N` | Stop after N frames; with `--fast` (no clock) used by the smoke tests |

`--engine=orig` still works and does nothing; the legacy engine and
`--engine=legacy` were removed in P4/S6.

## Controls

The original defaults:

| Key | Action |
|-----|--------|
| Arrows | Move / turn |
| Ctrl | Shoot |
| Alt + Left/Right | Strafe |
| Right Shift | Run |
| Space | Open door / elevator |
| 1-4 | Knife, pistol, machine gun, chain gun |
| F8 | Quick save (slot 0) |
| F9 | Quick load (slot 0) |
| Pause | Pause |
| F11 | Fullscreen toggle |
| Esc | Back to the menu (the original control panel is P5) |

In the menu: arrows and **Enter**, **+ / -** change the window scale, **Esc**
goes back to the title and from there quits.

With `mouse_look` on in `config.ini` the mouse turns and moves, and its
buttons shoot, strafe and open doors. When you die, BJ turns towards the
killer and the level restarts; out of lives ends the run.

### Game pad

An Xbox (XInput) pad works out of the box; PlayStation and other pads that
SDL knows work too. Plug it in before or during play. The layout is
twin-stick, analog on both sticks:

| Pad | Action |
|-----|--------|
| Left stick | Walk and strafe |
| Right stick | Turn |
| Left stick click (hold) | Run |
| RT or A | Shoot |
| X | Open door / elevator |
| LB / RB | Previous / next weapon |
| D-pad | As the arrow keys |
| Start | Back to the menu (as Esc) |
| Back | Pause |
| In the menu | D-pad or left stick, **A** chooses, **B** goes back |

Walking and strafing at the same time as turning is not something the
original controls could do; it is added for pads only, and demos (which
cannot store it) never use it.

## Saving

**F8** saves the game in slot 0 and **F9** loads it, from anywhere in a
level; the menu's **LOAD GAME** and `--load 0` start from it too. The save
holds the whole level as the original's did (every actor, door, pushwall
and pickup), so a loaded game carries on exactly where it was. A save only
loads with the data it was made with (WL1 or WL6). Saves from before P4/S6
(the old engine) cannot be loaded.

## Sound

Music and the AdLib-only effects (treasure, ammo, health, keys, extra life)
play through an emulated AdLib (OPL2) chip, and the digitized effects (guns,
doors, voices) like a Sound Blaster Pro, placed left or right by where they
happen. In `config.ini`, `sfx_on=0` turns the effects off and `music_on=0`
the music; each leaves the other alone.

## Where settings live
```
%APPDATA%\Wolf3DWin64\
  config.ini        scale, fullscreen, mouse, audio toggles, last_data
  highscores.dat
  save0.w64 …       save slots (F8 / F9 in the game)
```

The game does **not** need write access next to Program Files.

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
