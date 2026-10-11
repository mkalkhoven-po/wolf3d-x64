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
2. Any key on the title page → the original control panel (**Options**).
3. **New Game** → pick an episode and how tough you are, or **Load Game**.

The game is a faithful port of the original engine (fixed point, 70 Hz, the
original AI and renderer, the original control panel; see ADR-002). **Esc**
in the game opens the control panel; **Back to Game** returns. Still to come
in P5: the credits / high scores / demos
between title pages (P5d).

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
| `--load 0-9` | Continue from a save slot (the Save Game menu's slots, top one 0) |
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
| Esc | The control panel |
| F1 | Help (Read This!; the shareware only, as in the original) |
| F2 / F3 | Save Game / Load Game |
| F4 | Sound (devices and volume) |
| F5 | Change View (view size) |
| F6 | Control |
| F7 | End the game |
| F8 / F9 | Quick save / quick load: the slot last used in the Save or Load menu |
| F10 | Quit |
| Pause | Pause |
| F11 | Fullscreen toggle |

In the control panel: arrows, the mouse wheel (or moving the mouse, with
Mouse Enabled on) move the gun, **Enter** / **Space** / **Ctrl** / left click
choose, **Esc** / **Alt** / right click go back, and the first letter of an
item jumps to it, as in the original. Questions take **Y** or **N**. The window scale is `scale` in
`config.ini`.

The settings, kept in `config.ini`. Where the menus differ from the original:

| Menu | |
|------|-|
| **Sound** | **Sound devices** is the original screen (AdLib effects, digitized sound, music on or off); **Music** and **Effects** set each volume on its own (Enter, then left / right or the wheel; Enter keeps it, Esc puts it back) |
| **Video** (the original's Change View) | **View size** is the original Change View screen; **Fill the screen** off: whole multiples of 320×240 with black borders, on: as large as fits at 4:3 with sharp pixel edges; **Fullscreen** as **F11** |
| **Control** | the original's Mouse Enabled, Mouse Sensitivity and **Customize controls** (pick a row, **Enter**, then press the new key or button; **Esc** cancels), plus **Controller look speed**: how fast the pad's right stick turns |

With **Mouse Enabled** on (Control menu) the mouse turns and moves, and its
buttons shoot, strafe and open doors (Customize controls changes which). When you die, BJ turns towards the
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
| In the menus | D-pad or left stick, **A** chooses, **B** goes back; **A** / **B** answer yes / no, and accept / cancel a save's name |

Walking and strafing at the same time as turning is not something the
original controls could do; it is added for pads only, and demos (which
cannot store it) never use it.

## Saving

**Save Game** (or **F2**) has the original's ten slots: pick one, type a
name, **Enter**. **Load Game** (**F3**) continues from one. After that,
**F8** saves to the same slot at once and **F9** asks, then loads it. A save
holds the whole level as the original's did (every actor, door, pushwall and
pickup), so a loaded game carries on exactly where it was. WL1 and WL6 have
their own slots. Quick saves from before P5b (`save0.w64`) are no longer
listed.

## Sound

Music and the AdLib-only effects (treasure, ammo, health, keys, extra life)
play through an emulated AdLib (OPL2) chip, and the digitized effects (guns,
doors, voices) like a Sound Blaster Pro, placed left or right by where they
happen. The control panel's **Sound** menu switches the AdLib effects, the
digitized sound and the music on and off, each on its own (the PC speaker and
the Disney Sound Source are greyed out, as on a machine without them); the
choice is kept in `config.ini` (`sfx_on`, `digi_on`, `music_on`).

## Where settings live
```
%APPDATA%\Wolf3DWin64\
  config.ini        scale, fullscreen, the Sound, Video and Control menus, last_data
  scores.WL6 / .WL1 the high score table of each pack
  savegam0.WL6 …    the ten save slots of each pack
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
