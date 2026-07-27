# Wolf3D Win64 Port

Native **Windows 11 x64** port project for the 1992 id Software *Wolfenstein 3D* engine sources.

> **Status:** Planning & governance. No playable port binary yet.  
> **Repo visibility:** Private (you must be signed into GitHub as an account with access — guests see “Page not found”)  
> **MVP target:** Shareware episode (`WL1`) via SDL2 software renderer

## Live GitHub links (named)

| Name | URL |
|------|-----|
| **Repository** | [mkalkhoven-po/wolf3d-win64](https://github.com/mkalkhoven-po/wolf3d-win64) |
| **Project board — Wolf3D Win64 Port** | [users/mkalkhoven-po/projects/1](https://github.com/users/mkalkhoven-po/projects/1) |
| **All issues** | [Issues](https://github.com/mkalkhoven-po/wolf3d-win64/issues) |
| **Milestones M0–M8** | [Milestones](https://github.com/mkalkhoven-po/wolf3d-win64/milestones) |
| **Epic E1 — Runnable shell** | [#4 Epic E1: Runnable shell](https://github.com/mkalkhoven-po/wolf3d-win64/issues/4) |
| **Story — CMake x64 build** | [#5 Story: CMake x64 build produces wolf3d.exe](https://github.com/mkalkhoven-po/wolf3d-win64/issues/5) |
| **Story — SDL window** | [#6 Story: SDL window opens and quits cleanly](https://github.com/mkalkhoven-po/wolf3d-win64/issues/6) |
| **Closed — Epic E0** | [#1 Epic E0: Repo & governance](https://github.com/mkalkhoven-po/wolf3d-win64/issues/1) |

If a link shows **Page not found**, open GitHub in the browser, confirm you are logged in as **`mkalkhoven-po`** (or an account invited to this private repo), then retry.

## What this is

| Path | Purpose |
|------|---------|
| [`reference/original/`](reference/original/) | Unmodified original source drop (Borland C++ / DOS) for study |
| [`docs/product/`](docs/product/) | Vision, exclusions, functional requirements |
| [`docs/adr/`](docs/adr/) | Architecture Decision Records |
| [`docs/epics/`](docs/epics/) | Epic briefs (goal, stories, FRs, exclusions, acceptance) |
| [`src/`](src/) | *(future)* modern port code — not started |
| [`.github/`](.github/) | Issue templates |

## Docs (in-repo)

- [Vision](docs/product/VISION.md)
- [Exclusions](docs/product/EXCLUSIONS.md)
- [Functional requirements (MVP)](docs/product/FUNCTIONAL_REQUIREMENTS.md)
- [ADR-001 Platform backend](docs/adr/001-platform-backend.md)
- [Port analysis plan](docs/plans/wolf3d-win64-port-analysis.md)
- [Epic index](docs/epics/README.md)

## Legal / data

- Original game **data** (VSWAP, VGAGRAPH, maps, audio) is **not** in this repo.
- You must supply legally owned shareware or commercial data at runtime.
- Original source is provided by id under their Limited Use terms (see `reference/original/README/`). Educational porting only unless you obtain appropriate rights.
- Do **not** open a PR that adds `.WL6` / `.WL1` binaries.

## Building (E1 shell)

Prerequisites (Windows 11 x64):

- [CMake](https://cmake.org/) 3.20+
- A C17 toolchain: **MinGW-w64** (WinLibs) or MSVC 2022
- Ninja (recommended) or a multi-config VS generator
- Network on first configure (SDL2 is fetched via CMake `FetchContent` unless you turn that off)

```bat
:: Example: MinGW + Ninja (adjust COMPILER paths to your install)
set PATH=C:\path\to\mingw64\bin;%PATH%
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
build\wolf3d.exe
```

```powershell
# Visual Studio 2022 generator (x64)
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config RelWithDebInfo
.\build\RelWithDebInfo\wolf3d.exe
```

What you should see: a dark-blue window titled **Wolf3D Win64 — E1 shell**. Close the window or press **Esc** for a clean exit (`SDL2.dll` is copied next to the exe when using the shared SDL build).

Optional CMake flag: `-DWOLF_FETCH_SDL2=OFF` if you provide SDL2 yourself (`find_package(SDL2)`).

See [ADR-001](docs/adr/001-platform-backend.md).

## Roadmap pointer

Milestones **M0–M8** track epics **E0–E8**.  
**M0 is done.** Next: **[M1 Runnable shell](https://github.com/mkalkhoven-po/wolf3d-win64/milestone/2)** — epic [#4](https://github.com/mkalkhoven-po/wolf3d-win64/issues/4), stories [#5](https://github.com/mkalkhoven-po/wolf3d-win64/issues/5) and [#6](https://github.com/mkalkhoven-po/wolf3d-win64/issues/6).

## Credits

- Original game and engine: id Software (John Carmack, John Romero, Jason Blochowiak, et al.)
- This repository: port planning and modern implementation work
