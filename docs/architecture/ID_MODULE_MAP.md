# Original ID Engine → modern port mapping (E2)

| Original (reference/original) | Modern path | Status (E2) |
|-------------------------------|-------------|-------------|
| `ID_HEADS.H` / `ID_HEAD.H` | `src/id/id_heads.h` + `wl_types.h` | Stub umbrella + sized types |
| `ID_MM.C/.H` | `src/id/id_mm.c/.h` | malloc wrappers, no EMS/XMS |
| `ID_PM.C/.H` | `src/id/id_pm.c/.h` | Stub (real VSWAP in E3) |
| `ID_CA.C/.H` | `src/id/id_ca.c/.h` | Stub codecs (E3) |
| `ID_VL.C/.H` + ASM | `src/id/id_vl.c/.h` | Palette stubs; present still SDL shell |
| `ID_VH.C/.H` + ASM | `src/id/id_vh.c/.h` | Pic blits stub (E4) |
| `ID_IN.C/.H` | `src/id/id_in.c/.h` | Stub keys; SDL ticks |
| `ID_SD.C/.H` + ASM | `src/id/id_sd.c/.h` | Silent stub (E7) |
| `ID_US*.C/.H` / `Quit` | `src/id/id_us.c/.h` | Quit to stderr/exit |
| Startup DOS / `C0.ASM` | CRT + `src/main.c` | Deleted concept |
| `WL_DR_A.ASM` etc. | *(none yet)* | Not in build; C later |

**Rules for E2+**

- No `dos.h`, `far` pointers, or ASM objects in the CMake target.
- `far` / `near` / `_seg` are empty macros in `wl_types.h` for transitional includes only — prefer plain pointers in new code.
- Real window present remains `src/platform/win_sdl/` until VL/VH own a chunky buffer (E4).

## E3 additions
| Module | Path | Notes |
|--------|------|-------|
| Data discovery | `src/id/wl_data.c` | `--data`, `WOLF3D_DATA`, required files |
| Maps | `src/id/id_ca.c` | MAPHEAD/GAMEMAPS, Carmack+RLEW |
| VSWAP | `src/id/id_pm.c` | Directory smoke (full page cache later) |

## E4 additions
| Module | Path | Notes |
|--------|------|-------|
| VGA graph IDs | `src/id/gfxv_wl6.h` | TITLEPIC / menu chunk constants |
| Huffman + VGAGRAPH | `src/id/id_ca.c` | `CA_OpenGraphFiles`, planar→chunky pics |
| Framebuffer | `src/id/id_vl.c` | 320×200×8 + game palette |
| Palette | `src/id/gamepal.c` | 6-bit VGA GAMEPAL |
| 2D draw | `src/id/id_vh.c` | VWB_DrawPic + fallback font |
| Intro UI | `src/game/wl_menu_simple.c` | Title + keyboard main menu present |

## E5 additions
| Module | Path | Notes |
|--------|------|-------|
| Level / player | `src/game/wl_play.c` | tilemap, doors, move/collide, use |
| Raycast | `src/game/wl_draw.c` | DDA walls + VSWAP textures |
| Game loop | `src/game/wl_game.c` | WASD / doors / Esc |
| VSWAP pages | `src/id/id_pm.c` | Cached `PM_GetPage` |

## E6 additions
| Module | Path | Notes |
|--------|------|-------|
| Combat state | `src/game/wl_play.c` | enemies, pickups, keys, hitscan, lives |
| Billboards | `src/game/wl_draw.c` | enemy/pickup sprites + hurt flash |
| Episode loop | `src/game/wl_game.c` | shoot, respawn, elevator→next map |
