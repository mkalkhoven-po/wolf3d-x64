# E3 — Data path

## Goal
**Plan step: user-supplied assets.** Locate the player’s legally owned WL1 data, open core files, decode enough to prove maps/chunks load; fail clearly if not.

This epic is the roadmap gate before menus/3D: without completing the [GAME_DATA](../product/GAME_DATA.md) checklist, E4+ work is blocked.

## User stories
- As a player preparing to play (later), I have a clear checklist of which files to gather and where to put them (docs + story “Asset setup gate”).
- As a player, I can pass `--data` or set `WOLF3D_DATA` to my shareware folder.
- As a player, if files are missing I see which names are missing.
- As a developer, I can dump map plane 0 ASCII or chunk counts for verification.

## Functional requirements
- FR-DATA-001, FR-DATA-002, FR-DATA-003, FR-DATA-005, FR-DATA-006, FR-DATA-007
- Prerequisite for FR-GAME-* 

## Exclusions
- Rendering maps in 3D
- Writing saves
- WL6/SOD multi-SKU matrix (WL1 only)
- Committing game binaries to git

## Acceptance
- [ ] `docs/product/GAME_DATA.md` lists required files and discovery rules
- [ ] Valid WL1 folder: exits 0 after load smoke test (or proceeds to next stub state)
- [ ] Invalid folder: non-zero + missing file list
- [ ] Carmackized/RLEW map path verified against known wall layout sample
- [ ] No path in docs/README tells users to commit assets to the repo
