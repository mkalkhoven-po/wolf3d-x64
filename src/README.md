# `src/` — modern port sources

| Path | Role |
|------|------|
| `main.c` | Process entry |
| `platform/win_sdl/` | SDL2 window backend (still used by legacy shell path) |
| `id/` | ID engine managers (E2 stubs → real loaders E3/E4) |
| `game/` | Ported / new game flow (`wl_menu_simple` intro menu in E4) |

E4 adds VGAGRAPH title + keyboard main menu on a 320×200 framebuffer.
