# E5 — Move & walls

## Goal
Software raycaster + player movement/collision; walk first map (episode map index 0).

## User stories
- As a player, I can walk through E1M1-style geometry and see textured walls.
- As a player, walls block movement; doors can open with the use key.

## Functional requirements
- FR-GAME-003, FR-GAME-004
- FR-IN-001, FR-IN-002
- FR-VID-001–003
- NFR-001, NFR-002

## Exclusions
- Enemies/combat (static props optional)
- Audio required
- Pushwall secrets can be minimal but doors required

## Acceptance
- [x] Start map from menu (New Game → game loop)
- [x] Move/turn with keyboard (WASD + arrows)
- [x] Raycast textured walls from VSWAP in pure C
- [x] Collision with walls; doors open with Space/E and become passable
- [x] `--smoke-only` verifies level setup + non-empty raycast framebuffer

## Implementation notes
- `src/game/wl_play.c` — tilemap/doors/player + collision
- `src/game/wl_draw.c` — DDA raycast + wall column texturing
- `src/game/wl_game.c` — SDL game loop
- `src/id/id_pm.c` — VSWAP page cache for wall textures
