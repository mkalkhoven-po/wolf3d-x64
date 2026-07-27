# E3 — Data path

## Goal
**Plan step: user-supplied assets.** Locate the player’s legally owned WL1/WL6 data, open core files, decode maps + VSWAP directory; fail clearly if not.

## User stories
- As a player, I can pass `--data` or set `WOLF3D_DATA` to my game folder.
- As a player, if files are missing I see which names are missing.
- As a developer, I can smoke-load map planes + VSWAP headers for verification.

## Functional requirements
- FR-DATA-001, FR-DATA-002, FR-DATA-003, FR-DATA-005, FR-DATA-006, FR-DATA-007

## Acceptance
- [x] `docs/product/GAME_DATA.md` lists required files and discovery rules
- [x] Valid WL6 folder: load smoke exits 0 with `--smoke-only`
- [x] Invalid folder: non-zero + missing file list
- [x] Carmackized + RLEW map decompress + VSWAP directory parse
- [x] No path in docs/README tells users to commit assets to the repo
