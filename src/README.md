# `src/` — modern port sources

| Path | Role |
|------|------|
| `main.c` | Process entry |
| `platform/win_sdl/` | SDL2 window backend |
| `id/` | ID engine managers (E2 stubs → real impl over epics) |
| `game/` | Ported `WL_*` game logic (E4+) |

E2 adds sized types + stub `VL/IN/SD/MM/PM/CA/US`. See [ID_MODULE_MAP](../docs/architecture/ID_MODULE_MAP.md).
