# `src/` — modern port sources

| Path | Role |
|------|------|
| `main.c` | Process entry |
| `platform/win_sdl/` | SDL2 window/input/audio backend (grows over epics) |
| `id/` | Rewritten ID engine managers (E2+) |
| `game/` | Ported `WL_*` game logic (E4+) |

E1 only builds `main.c` + the SDL shell.
