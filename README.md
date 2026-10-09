# Wolf3D Win64 Port

Native **Windows 11 x64** port of the 1992 id Software *Wolfenstein 3D* engine sources.

> **Status:** Playable desktop build through **E8** (AppData config/saves, integer scale, fullscreen).  
> **Repo visibility:** Private (sign in as an account with access — guests see “Page not found”)  
> **MVP:** Shareware **WL1** first (WL6 is supported for development and testing today; WL1 support lands in P3) via SDL2 software renderer + digitized audio. See [ADR-002](docs/adr/002-faithful-fixed-point-port.md).

## Player quickstart

See **[docs/player/QUICKSTART.md](docs/player/QUICKSTART.md)** for run instructions, AppData paths, and controls.

```bat
set WOLF3D_DATA=D:\path\to\your\wolfdata
build\wolf3d.exe --data "%WOLF3D_DATA%"
```

Faithful engine preview (P4, no menus yet; Esc quits):
```bat
build\wolf3d.exe --data "%WOLF3D_DATA%" --engine=orig
```

Smoke:
```bat
build\wolf3d.exe --data "%WOLF3D_DATA%" --smoke-only
```

One-button harness (Git Bash or `scripts\smoke_wl6.bat`):
```bat
set WOLF3D_DATA=D:\path\to\your\wolfdata
scripts\smoke_wl6.bat
```
See [docs/qa/SMOKE.md](docs/qa/SMOKE.md).

## Live GitHub

Private repo: https://github.com/mkalkhoven-po/wolf3d-win64  

| Board | URL |
|-------|-----|
| **Delivery (M0–M8)** | https://github.com/users/mkalkhoven-po/projects/1 |
| **QA & Stabilization (Q0–Q5)** | https://github.com/users/mkalkhoven-po/projects/2 |

If links show **Page not found** in chat previews, paste into a browser where you are signed in as **`mkalkhoven-po`**.

## QA

Phase 2 after E8: automated smoke, playtest checklists, FR matrix, bug burn-down.

- **[QA index](docs/qa/README.md)** · [FR matrix](docs/qa/FR_TEST_MATRIX.md) · [Smoke](docs/qa/SMOKE.md) · [Playtest](docs/qa/PLAYTEST_MANUAL.md)
- Milestones **Q0–Q5** on the repo; epics **#41–#60** on Project #2.

## What this is

| Path | Purpose |
|------|---------|
| [`reference/original/`](reference/original/) | Unmodified original DOS/Borland sources (study only) |
| [`docs/product/`](docs/product/) | Vision, exclusions, functional requirements |
| [`docs/player/`](docs/player/) | Player quickstart |
| [`docs/qa/`](docs/qa/) | Test matrix, smoke, playtest, go/no-go |
| [`docs/epics/`](docs/epics/) | Epic briefs E0–E8 and Q0–Q5 |
| [`src/`](src/) | Modern port (platform + ID managers + game) |

## Docs

- [Vision](docs/product/VISION.md)
- [Exclusions](docs/product/EXCLUSIONS.md)
- [Functional requirements](docs/product/FUNCTIONAL_REQUIREMENTS.md)
- [User-supplied game assets](docs/product/GAME_DATA.md)
- [Local assets layout (pack vs PNG lookbook)](docs/product/LOCAL_ASSETS.md)
- [**Player quickstart**](docs/player/QUICKSTART.md)
- [**QA index**](docs/qa/README.md)
- [ADR-001 Platform backend](docs/adr/001-platform-backend.md)
- [Epic index](docs/epics/README.md)
- [ID module map](docs/architecture/ID_MODULE_MAP.md)

## Legal / data

- Original game **data** is **not** in this repo.
- Point `--data` / `WOLF3D_DATA` at a legally owned WL1/WL6 folder (see [GAME_DATA.md](docs/product/GAME_DATA.md)).
- Config/saves go to `%APPDATA%\Wolf3DWin64\` — not next to the exe.
- Do **not** open a PR that adds `.WL6` / `.WL1` binaries.

## Building

Prerequisites: CMake 3.20+, C17 toolchain (MinGW-w64 or MSVC 2022), Ninja recommended. First configure fetches SDL2 unless `-DWOLF_FETCH_SDL2=OFF`.

```bat
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
build\wolf3d.exe --data Wolf3dgame --smoke-only
```

Tests and checks:
```bat
ctest --test-dir build --output-on-failure
cmake -S . -B build-asan -G Ninja -DCMAKE_BUILD_TYPE=Debug -DWOLF_SANITIZE=ON
```
Unit tests need no game data. Set `WOLF3D_DATA` before configuring to also register the `smoke_only` CTest case (and `WOLF3D_DATA_WL1` for a WL1 pack). Sanitizers need GCC/Clang with ASan support (not every MinGW build has it).

Troubleshooting: on Windows 11 with **Smart App Control** on, a freshly built unsigned `wolf3d.exe` or `SDL2.dll` can be blocked ("Bad Image", status `0xc0e90002`, or ctest "BAD_COMMAND"). The Code Integrity event log (event 3077) confirms it. It is an OS policy, not a build error; retrying later sometimes passes, otherwise turn Smart App Control off or build on another machine/CI.

Runtime dependency: **`SDL2.dll`** is copied next to `wolf3d.exe` when using the shared SDL build (`FR-PLAT-004`).

### In-game desktop controls

| Key | Action |
|-----|--------|
| F11 | Fullscreen toggle |
| +/- | Integer scale |
| F5 / F9 | Save / load slot 0 |
| Esc | Menu back / quit |

## Roadmap

Milestones **M0–M8** = epics **E0–E8** — the platform shell, data loading, raycast view, placeholder combat, audio and Win11 polish are on `main`. Gameplay is **not** yet faithful to the original (see *Known gaps* in [docs/epics/README.md](docs/epics/README.md)).

Current plan: a faithful fixed-point port driven by the original sources — P1 safety net → P2 docs → P3 WL1 data → P4 engine core → P5 game flow + QA → P6 WL6 → F4 (dual pistols, optional, last). **WL1 shareware first, then WL6.** Details in [docs/epics/README.md](docs/epics/README.md).

## Credits

- Original game and engine: id Software  
- This repository: Win64 port planning and implementation
