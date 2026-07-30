# F2 — First-person weapon view

## Source of truth (player screenshots)
Reference frames show the **held weapon in world view**: hands + pistol/machinegun model centered lower-middle above the status bar (not only a status-bar silhouette).

## Goal
Draw the active weapon as a **first-person overlay** from VSWAP weapon shapes (and attack frames when firing), matching snout / bob of the original campaign look as far as the port’s frame set allows.

## User stories
- Pistol default shows correct first-person art at rest.
- Shooting advances attack frames and returns to ready.
- Switching 1–4 (knife/pistol/MG/chain) swaps the view weapon.
- Overlay respects view height (above status bar) and does not break raycast HUD z-order.

## FRs
- **FR-FID-010** First-person weapon gear drawn in the 3D view region.
- **FR-FID-011** Weapon switches with inventory / number keys.
- **FR-FID-012** Attack animation plays on fire (at least one cycle per weapon class).
- Ties **FR-GAME-007** (weapons) and **FR-IN-001**.

## Acceptance
- [x] Rest pose matches silhouettes on reference screenshots for pistol / auto
- [x] Fire visibly animates
- [x] No permanent covering of status bar digits
- [x] Smoke / present still OK

## Notes
- Status bar weapon **icon** is F1; this epic is the **in-view hands/gun**.
- Simplified frames acceptable if full pipe set is huge — document Partial.
- In-motion rest snout on E1M1 any%: [`docs/qa/E1M1_ANYPERCENT_REFERENCE.md`](../qa/E1M1_ANYPERCENT_REFERENCE.md).
- **Unlocks F4 dual pistols** — single pistol rest/fire must land before akimbo mirror ([F4-dual-pistols.md](F4-dual-pistols.md)).

## GitHub

- Epic: #77
- Stories: #82, #83, #84

## Status
**Done** — pack VSWAP FP weapons via SimpleScaleShape (`wl_weapon.c`); smoke `Weapon smoke:`.
