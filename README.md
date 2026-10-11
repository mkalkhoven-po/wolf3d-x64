# Wolf3D x64

A faithful **x64 port of the 1992 *Wolfenstein 3-D* engine**, rebuilt from id Software's original source code: the same fixed-point maths, 70 Hz game clock, AI, renderer and screens, so it plays like the DOS game. For Windows 11 and Linux.

You bring the game files: the free **shareware** version (Episode 1) or the **full version** (the registered game, all six episodes), both version 1.4. No game data is in this repository.

> **Status:** the faithful engine and all of the original's screens are done (P1–P5) and the go/no-go was signed off **Go** (2026-10-11). In progress: **R1**, a downloadable package for Windows and Linux. The repository is private until then.

## What you get

- **The original engine**, ported from `WL_*.C` in 16.16 fixed point: original AI and state tables, doors, pushwalls, the raycaster and sprite scaler, `US_RndT`. All eight original demos replay in sync ([ADR-002](docs/adr/002-faithful-fixed-point-port.md)).
- **The original screens**: rating screen, title, credits and high scores with the demos in between; the full control panel (new game, sound, control and Customize, load and save with ten named slots, view size, Read This! for the shareware); "Get Psyched!", the intermission tally, victory, high score entry, the death cam.
- **Sound as on a Sound Blaster Pro**: digitized effects placed left and right, and AdLib music and effects through an OPL2 emulator written for this port ([ADR-003](docs/adr/003-own-opl2-emulator.md)).
- **Modern additions, kept apart from the game logic**: XInput and other game pads (twin-stick), separate music and effects volume, controller look speed, fill-the-screen scaling with sharp pixels, fullscreen, mouse wheel in the menus, settings and saves in `%APPDATA%\Wolf3D-x64\`.

## Play

Full guide: **[docs/player/QUICKSTART.md](docs/player/QUICKSTART.md)** (finding the data, controls, menus, saves, settings).

```bat
build\wolf3d.exe --data D:\path\to\your\wolf3d
```

The game also looks for a `Wolf3dgame` or `data` folder next to the exe; when it finds no game files it asks for the folder, and remembers it. Any game option starts a game directly:

```bat
build\wolf3d.exe --data D:\path\to\your\wolf3d --map 0 --skill 3
build\wolf3d.exe --data D:\path\to\your\wolf3d --demo 0
```

## Build

Prerequisites: CMake 3.20+, a C17 compiler (MinGW-w64 or MSVC 2022), Ninja recommended. The first configure fetches SDL2 (`-DWOLF_FETCH_SDL2=OFF` to use your own).

```bat
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
```

`SDL2.dll` is copied next to `wolf3d.exe`.

### Tests

```bat
set WOLF3D_DATA=D:\path\to\full-version
set WOLF3D_DATA_WL1=D:\path\to\shareware
cmake -S . -B build
ctest --test-dir build --output-on-failure
```

Without game data the unit tests run (also in CI). With one or both packs set at configure time, the engine tests run on real data: demo sync per view size, golden frames, the E1M1 route, every menu and screen driven by scripted input with pinned screenshots, smoke, cold start, settings migration (49 tests with both packs). `-DWOLF_SANITIZE=ON` builds with sanitizers where the compiler has them.

Troubleshooting: with Windows 11 **Smart App Control** on, a freshly built unsigned `wolf3d.exe` or `SDL2.dll` can be blocked ("Bad Image", status `0xc0e90002`, or ctest "BAD_COMMAND"; Code Integrity event 3077). That is an OS policy, not a build error.

## Roadmap

| Phase | | Status |
|-------|-|--------|
| E0–E8, Q0–Q3, F1–F3, L1 | First playable shell, data loading, QA harness (a float engine, since replaced) | Done |
| P1–P3 | Safety net (warnings, unit tests, CI), honest docs, shareware data layer | Done |
| P4 | The faithful engine core: fixed point, 70 Hz, original AI, player, renderer; fidelity gate | Done |
| P5 | The original game flow: menus, intermission, attract loop; audio pass and go/no-go (Go) | Done |
| **R1** | **Release: GPL-2.0 license, icon, Windows zip and Linux build, first-run folder picker** | **In progress** |
| P6 | Live playtest after release, bugs as issues | Planned |
| F4 | Dual pistols (optional) | Deferred |

Details: [docs/epics/README.md](docs/epics/README.md). Boards: [Wolf3D x64 — Delivery](https://github.com/users/mkalkhoven-po/projects/1) and [Wolf3D x64 — QA & Stabilization](https://github.com/users/mkalkhoven-po/projects/2).

## Docs

- [Player quickstart](docs/player/QUICKSTART.md) · [User-supplied game data](docs/product/GAME_DATA.md)
- [ADR-001 Platform backend](docs/adr/001-platform-backend.md) · [ADR-002 Faithful fixed-point port](docs/adr/002-faithful-fixed-point-port.md) · [ADR-003 Own OPL2 emulator](docs/adr/003-own-opl2-emulator.md)
- [QA index](docs/qa/README.md): [FR matrix](docs/qa/FR_TEST_MATRIX.md), [playtest checklists](docs/qa/PLAYTEST_MANUAL.md), [go/no-go](docs/qa/GO_NO_GO.md), [smoke](docs/qa/SMOKE.md)
- [Epic index and roadmap](docs/epics/README.md) · [ID module map](docs/architecture/ID_MODULE_MAP.md)

| Path | |
|------|-|
| [`src/orig/`](src/orig/) | The faithful engine, ported from the original `WL_*.C` and `ID_*.C` |
| [`src/id/`](src/id/) | Data loading (maps, VSWAP, VGAGRAPH, audio), the OPL2 emulator, the sound mixer |
| [`src/platform/win_sdl/`](src/platform/win_sdl/) | SDL2 window, input, game pad, settings and saves, the engine's hooks |
| [`reference/original/`](reference/original/) | id's original DOS sources, unmodified (the reference) |
| [`tests/`](tests/) | Unit and engine tests (CTest) |

## License

Wolf3D x64 is free software under the **GNU General Public License, version 2** ([LICENSE](LICENSE)), as id Software's 2012 release of the *Wolfenstein 3-D* source it derives from. SDL2 is under the zlib license; see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

- No game data is in this repository and the license does not cover it: point the game at files you are entitled to use ([GAME_DATA.md](docs/product/GAME_DATA.md)). Pull requests adding game files (`.WL1` shareware, `.WL6` full version) are not accepted.
- *Wolfenstein* is a trademark of ZeniMax Media; this project is not affiliated with or endorsed by id Software or ZeniMax.

## Credits

- The original game and engine: id Software (1992)
- This port: mkalkhoven-po
