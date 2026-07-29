# F1 — Status HUD & “Get Psyched!” interstitial

## Source of truth (player screenshots)
1. **Get Psyched!** loading panel centered on dark teal field, with full **STATUSBAR** visible under it (FLOOR / SCORE / LIVES / face / HEALTH% / AMMO / weapon glyph).
2. Under the title inside the psyched panel: a thin **horizontal progress bar that fills left→right in red** (same warm red family as the “Get Psyched!” lettering — not cyan/green/white).
3. In-game frames share the same **original status bar layout** (not a temporary text strip).

## Goal
Implement original-style **interstitial load** + **VGA status bar** UI driven by pack art (`GETPSYCHEDPIC`, `STATUSBARPIC`, digit/face/weapon latch pics from VGAGRAPH).

## User stories
- Loading: after New Game / map change, show Get Psyched! (or pack equivalent) before possession of play view.
- Psyched progress: black track at panel bottom; fill uses original red palette indices (see implementation notes).
- Status bar: FLOOR, SCORE, LIVES, BJ face, HEALTH %, AMMO, current-weapon silhouette match reference layout.
- Values update live from `player_t` / mapnum; face reacts to damage/death when assets allow.

## FRs
- **FR-FID-001** Get Psyched! interstitial between menu/new level and play, including **red progress bar** under the banner.
- **FR-FID-002** Original STATUSBARPIC (or equivalent composite) occupies bottom strip.
- **FR-FID-003** HUD numbers/face/weapon use pack digit/face/weapon chunks, not the ad-hoc bitmap font strip alone.
- Ties **FR-VID-001** (status region in 320×200 layout).

## Acceptance
- [ ] Screenshot parity for load screen vector against Get Psyched reference
- [ ] Progress bar is **red** (letter-matched), fills with preload, not a random UI color
- [ ] In-play HUD layout matches reference slots (floor/score/lives/face/health/ammo/weapon)
- [ ] Smoke still passes; no commercial data committed
- [ ] Temporary `VH_DrawString` bar retired or relegated to debug

## Implementation notes (original `WL_INTER.C`)
```
PreloadGraphics:
  VWB_Bar(0,0,320,200-STATUSLINES, 127);   // teal backdrop
  LatchDrawPic(... GETPSYCHEDPIC);
  Window ≈ 28×6 char cells around the pic

PreloadUpdate(current, total):
  track: VWB_Bar(..., BLACK);
  fill:  VWB_Bar(..., 0x37);               // SECONDCOLOR — red
  high:  VWB_Bar(..., 0x32);               // lighter red 1px cap
```
Palette indices **0x37 / 0x32** are the authentic reds (same family as the gothic letter shading). Do not invent a blue/green “loading” look.

## Notes / Partial
- Exact face animation set can land after static face + numbers.
- Digits may latch from `L_*` / number pics in `GFXV_WL6.H`.

## GitHub

- Epic: #76
- Stories: #79, #80, #81
