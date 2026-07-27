# Wolf3D Win64 Port

Native **Windows 11 x64** port project for the 1992 id Software *Wolfenstein 3D* engine sources.

> **Status:** Planning & governance. No playable port binary yet.  
> **Repo visibility:** Private  
> **MVP target:** Shareware episode (`WL1`) via SDL2 software renderer

## What this is

| Path | Purpose |
|------|---------|
| `reference/original/` | Unmodified original source drop (Borland C++ / DOS) for study |
| `docs/product/` | Vision, exclusions, functional requirements |
| `docs/adr/` | Architecture Decision Records |
| `docs/epics/` | Epic briefs (goal, stories, FRs, exclusions, acceptance) |
| `src/` | *(future)* modern port code — not started |
| `.github/` | Issue templates, label notes |

## Quick links

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

## Building (future)

Not yet. Planned stack: CMake + MSVC/Clang x64 + SDL2. See ADR-001.

## Project board

- **Repo:** https://github.com/mkalkhoven-po/wolf3d-win64 (private)
- **Board:** https://github.com/users/mkalkhoven-po/projects/1
- **Issues:** https://github.com/mkalkhoven-po/wolf3d-win64/issues

Milestones **M0–M8** track epics E0–E8. **M0 is done** — next is **M1 Runnable shell** (#4–#6).

## Credits

- Original game and engine: id Software (John Carmack, John Romero, Jason Blochowiak, et al.)
- This repository: port planning and modern implementation work
