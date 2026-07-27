# E4 — 2D present & menus

## Goal
Chunky framebuffer presentation with palette; show title and navigate main menu.

## User stories
- As a player, I see the title screen with correct colours.
- As a player, I can move the menu cursor and choose New Game / Options / Quit.

## Functional requirements
- FR-VID-001–005
- FR-IN-003
- FR-GAME-001, FR-GAME-002 (start new game transition may still be stub level)

## Exclusions
- Full options persistence (minimal OK)
- 3D view
- Music (optional stub)

## Acceptance
- [x] 320×200×8 framebuffer + `gamepal` present path (SDL texture)
- [x] Title visible from VGAGRAPH `TITLEPIC` with palette
- [x] Menu keyboard navigate (↑/↓/Enter) + quit (Esc / Quit item)
- [x] Focus loss does not crash (SDL event loop)
- [x] `--smoke-only` loads title/menu chunks without window

## Implementation notes
- `src/id/id_vl.c` — RAM framebuffer + bar/plot/blit helpers
- `src/id/id_ca.c` — VGADICT Huffman + VGAHEAD/VGAGRAPH chunk load; planar→chunky unmunge
- `src/id/gamepal.c` — VGA 6-bit palette extracted from `GAMEPAL.OBJ`
- `src/game/wl_menu_simple.c` — title → main menu loop (New Game still stub until E5)
