# Player quickstart (Windows 11)

## What you need
1. A build of `wolf3d.exe` (and `SDL2.dll` beside it).
2. **Your own** Wolfenstein 3D data folder (WL1 or WL6).  
   Example local drop used in development: `D:\wolf3d-master\Wolf3dgame`

This project **never** ships game data.

## Run
```bat
cd /d D:\path\to\build-or-release
set WOLF3D_DATA=D:\path\to\your\Wolf3dgame
wolf3d.exe --data "%WOLF3D_DATA%"
```

Smoke (no interactive window loop required beyond brief present probe):
```bat
wolf3d.exe --data "%WOLF3D_DATA%" --smoke-only
```

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
- Missing data → non-zero exit and `MISSING:` list.
- No audio device → silent fallback (game still runs).
- Aspect is fixed **4:3** logical 320×200 with nearest-neighbor integer scale.
