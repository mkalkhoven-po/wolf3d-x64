# Local smoke (QA)

## Goals

- Non-interactive gate for loader + core systems already wired into `--smoke-only`.
- Stable **exit codes** and **log markers**.
- **Never** requires packing commercial data into git.

## Prerequisites

- Built `build/wolf3d.exe` (+ `SDL2.dll` beside it).
- Own data folder (e.g. `Wolf3dgame` or `WOLF3D_DATA`).

```bat
cmake --build build -j
set WOLF3D_DATA=D:\path\to\Wolf3dgame
build\wolf3d.exe --data "%WOLF3D_DATA%" --smoke-only
```

Bad path (must fail):

```bat
build\wolf3d.exe --data C:\no-such-wolf-pack --smoke-only
```

Expected: **exit 2** (or other non-zero) and no hang.

## Exit codes (current main)

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

Exact codes can drift; treat non-zero as fail and capture logs.

## Recommended markers (good run)

Grep smoke log for (subset is OK; expand under Q1 story #47):

- `smoke OK` or `wolf3d-win64 smoke OK`
- `TITLEPIC loaded`
- `Audio digi:` / audio smoke
- `UserData smoke:`
- `Present smoke:` / `logical=320x200`

## One-button script (Q1 #46)

When landed, prefer:

```bat
scripts\smoke_wl6.bat
```

or Git Bash:

```bash
scripts/smoke_wl6.sh
```

Until then, the bare `--smoke-only` commands above are the harness.

## CI note

GitHub Actions **must not** download commercial assets. Options:

1. Compile-only CI (no play smoke).
2. Self-hosted runner with private local pack (optional, later).

## Related FRs

FR-PLAT-001, FR-DATA-001–005, FR-AUD-001, FR-DATA-004, FR-SAVE smoke path.
