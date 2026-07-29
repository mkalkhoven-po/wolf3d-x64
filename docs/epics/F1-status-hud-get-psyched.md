# F1 — Status HUD & “Get Psyched!” interstitial

## Source of truth (player screenshots)
1. **Get Psyched!** loading panel centered on dark teal field, with full **STATUSBAR** visible under it (FLOOR / SCORE / LIVES / face / HEALTH% / AMMO / weapon glyph).
2. In-game frames share the same **original status bar layout** (not a temporary text strip).

## Goal
Implement original-style **interstitial load** + **VGA status bar** UI driven by pack art (`GETPSYCHEDPIC`, `STATUSBARPIC`, digit/face/weapon latch pics from VGAGRAPH).

## User stories
- Loading: after New Game / map change, show Get Psyched! (or pack equivalent) before possession of play view.
- Status bar: FLOOR, SCORE, LIVES, BJ face, HEALTH %, AMMO, current-weapon silhouette match reference layout.
- Values update live from `player_t` / mapnum; face reacts to damage/death when assets allow.

## FRs
- **FR-FID-001** Get Psyched! interstitial between menu/new level and play.
- **FR-FID-002** Original STATUSBARPIC (or equivalent composite) occupies bottom strip.
- **FR-FID-003** HUD numbers/face/weapon use pack digit/face/weapon chunks, not the ad-hoc bitmap font strip alone.
- Ties **FR-VID-001** (status region in 320×200 layout).

## Acceptance
- [ ] Screenshot parity for load screen vector against Get Psyched reference
- [ ] In-play HUD layout matches reference slots (floor/score/lives/face/health/ammo/weapon)
- [ ] Smoke still passes; no commercial data committed
- [ ] Temporary `VH_DrawString` bar retired or relegated to debug

## Notes / Partial
- Exact face animation set can land after static face + numbers.
- Digits may latch from `L_*` / number pics in `GFXV_WL6.H`.

## GitHub

- Epic: #76
- Stories: #79, #80, #81
