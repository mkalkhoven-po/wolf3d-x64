# Level layout & world geometry authenticity — gap index

> **Superseded as the live epic brief by [L1-level-geometry-secrets.md](L1-level-geometry-secrets.md).**  
> This file remains as the short inventory that seeded L1.

## Already covered (partially)

| Topic | Where | State |
|-------|--------|--------|
| MAPHEAD/GAMEMAPS load + Carmack/RLEW | `src/id/id_ca.c`, E3 | Met |
| Wall textures from VSWAP `(tile-1)*2+side` | `wl_draw.c` | Met (basic) |
| Doors + lock types + elevator | #72/#75 | Met |
| Door slide open + gap shows walls | #105–#107 | Improving |
| Wall height mul ~1.45 + FOV | #67 | Partial vs original `SetupScaling` |
| Floor/ceiling **solid colors** | `wl_draw.c` sky/floor fills `0x19`/`0x18` | MVP only |
| Pushwalls / secrets | FR-GAME-004 Partial → **L1.1** | **Not done** |
| Sprite world projection | F3 + **#108** authentic projection | Met (path) |

## Live epic

See **L1** brief for stories L1.1–L1.6, FR-FID-040..042, FR-GAME-011, priority vs F4/Q4/Q5, and acceptance.
