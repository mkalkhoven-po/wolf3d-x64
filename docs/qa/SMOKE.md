# Local smoke (QA) — Q1 harness

## Goals

- One-button **good path** + **bad path** gate for systems behind `--smoke-only`.
- Assert **stable log markers** so silent regressions fail loudly.
- **Never** pack commercial / shareware game data into git (`FR-DATA-005`).

## One-button (preferred)

From repo root, with Git Bash or `scripts\smoke_wl6.bat`:

```bash
# Git Bash / MSYS
export WOLF3D_DATA="/d/path/to/Wolf3dgame"   # optional if ./Wolf3dgame exists
scripts/smoke_wl6.sh
```

```bat
REM cmd.exe — wraps bash when Git for Windows is installed
set WOLF3D_DATA=D:\path\to\Wolf3dgame
scripts\smoke_wl6.bat
```

Options (`smoke_wl6.sh`):

| Flag | Meaning |
|------|---------|
| `--data PATH` | Data directory (overrides `WOLF3D_DATA`) |
| `--exe PATH` | Path to `wolf3d.exe` (default `build/wolf3d.exe`) |
| `--no-build` | Skip `cmake --build` |
| `--bad-path P` | Override negative-test directory |

Default data search order: `--data` → `WOLF3D_DATA` → `./Wolf3dgame` → `./data` (if it contains `VSWAP.*`).

## Manual equivalent

```bat
cmake --build build -j
set WOLF3D_DATA=D:\path\to\Wolf3dgame
build\wolf3d.exe --data "%WOLF3D_DATA%" --smoke-only
build\wolf3d.exe --data C:\no-such-wolf-pack --smoke-only
```

Bad path expected: **exit 2**, no `wolf3d-win64 smoke OK`.

## Harness exit codes (`smoke_wl6.sh`)

| Code | Meaning |
|------|---------|
| 0 | All PASS |
| 1 | Setup/build/harness failure |
| 2 | Good-path process nonzero |
| 3 | Good-path missing required markers |
| 4 | Bad-path did not exit 2 / leaked smoke OK |

## Game exit codes (`wolf3d.exe --smoke-only`)

| Code | Meaning (approx.) |
|------|-------------------|
| 0 | Smoke OK |
| 2 | Data resolve failed |
| 3 | Required files check failed |
| 4–5 | Map / VSWAP open failed |
| 8 | Graphics open failed |
| 9–14 | Title / level / combat / episode smoke failed |
| 15–16 | Audio setup / audio smoke failed |
| 17–19 | UserData / present smoke failed |

## Required markers (good run)

Asserted by `scripts/smoke_wl6.sh` (must all appear in combined stdout/stderr):

| Marker substring |
|------------------|
| `wolf3d-win64 smoke OK` |
| `TITLEPIC loaded` |
| `VSWAP chunks=` |
| `VGAGRAPH open` |
| `Audio digi:` |
| `Audio smoke:` |
| `silent fallback play: OK` |
| `UserData smoke:` |
| `config roundtrip OK` |
| `save/load OK` |
| `Present smoke:` |
| `logical=320x200` |

When changing engine print strings, update **both** the game and this list / script in the same PR.

## CI note

GitHub-hosted CI **must not** download commercial assets. Prefer:

1. Compile-only workflow (optional, later).
2. Self-hosted runner with private local pack (optional).

Local maintainer smoke is the source of truth for Q1.

## Related FRs

FR-PLAT-001 · FR-DATA-001–005 · FR-AUD-001 · FR-DATA-004 · FR-SAVE smoke path · FR-QA-001

## Related issues

Epic #45 · Stories #46 #47 #48
