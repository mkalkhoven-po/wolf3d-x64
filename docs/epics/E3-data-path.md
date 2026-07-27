# E3 — Data path

## Goal
Locate user WL1 data, open core files, decode enough to prove maps/chunks load; fail clearly if not.

## User stories
- As a player, I can pass `--data` or set `WOLF3D_DATA` to my shareware folder.
- As a player, if files are missing I see which names are missing.
- As a developer, I can dump map plane 0 ASCII or chunk counts for verification.

## Functional requirements
- FR-DATA-001, FR-DATA-002, FR-DATA-003, FR-DATA-005
- Prerequisite for FR-GAME-* 

## Exclusions
- Rendering maps in 3D
- Writing saves
- WL6/SOD multi-SKU matrix (WL1 only)

## Acceptance
- [ ] Valid WL1 folder: exits 0 after load smoke test (or proceeds to next stub state)
- [ ] Invalid folder: non-zero + missing file list
- [ ] Carmackized/RLEW map path verified against known wall layout sample
