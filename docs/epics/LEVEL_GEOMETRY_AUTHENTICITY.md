# Level layout & world geometry authenticity — gap index

> **Status:** **No dedicated epic/PR yet.** This is an inventory of what exists vs original, so we can open a plan epic when ready.  
> Related: sprite projection authenticity PR (world actors/props size); this note is **maps + walls + floor/ceiling + secrets**.

## Already covered (partially)

| Topic | Where | State |
|-------|--------|--------|
| MAPHEAD/GAMEMAPS load + Carmack/RLEW | `src/id/id_ca.c`, E3 | Met |
| Wall textures from VSWAP `(tile-1)*2+side` | `wl_draw.c` | Met (basic) |
| Doors + lock types + elevator | #72/#75 | Met |
| Door slide open + gap shows walls | #105–#107 | Improving |
| Wall height mul ~1.45 + FOV | #67 | Partial vs original `SetupScaling` |
| Floor/ceiling **solid colors** | `wl_draw.c` sky/floor fills `0x19`/`0x18` | MVP only |
| Pushwalls / secrets | FR-GAME-004 Partial; E5 brief | **Not done** |
| Sprite world projection | F3 + authentic projection plan | In flight |

## Not yet a dedicated analysis/optimize PR

There is **no** open PR/epic whose sole job is:

> Optimize and fully audit level plans: secret rooms, texture placement, wall height authenticity, floor/ceiling color or texture, pushwalls, etc.

Mentions only:

- `docs/plans/wolf3d-win64-port-analysis.md` — early checklist (pushwalls, floor/ceiling colors)
- `docs/epics/E5-move-walls.md` — doors required; pushwalls “minimal”
- `docs/qa/FR_TEST_MATRIX.md` — FR-GAME-004 pushwalls **Partial residual**
- F3 out-of-scope: pushwalls/secret art

## Recommended future epic (when you want it)

**Working title:** `F5` or `L1 — Level geometry & secrets authenticity`

| Story | Intent |
|-------|--------|
| L1.1 | Pushwalls: activate, move, secret score, block rules (original) |
| L1.2 | Wall height / projection constants vs `CalcProjection` + `SetupScaling` (unify with sprites) |
| L1.3 | Floor/ceiling: original solid palette indices per area (or documented Partial) |
| L1.4 | Texture page audit: door/elev/wall mismatches vs WL6 maps |
| L1.5 | Secret room playtest matrix (E1 maps) + FR matrix rows |
| L1.6 | Optional: area lighting / door side textures polish |

**Depends on:** stable sprite projection (this PR family); pack still runtime-only.

**Not in pack as a “level design JSON”:** secrets and pushwalls are **engine behaviors** + map plane encodings in GAMEMAPS, defined in original source (`WL_AGENT`/`WL_ACT1` pushwall), not a separate size table.

## Sources of truth

- Maps: `GAMEMAPS` + `MAPHEAD` (tile types)
- Walls: VSWAP wall pages
- Rules: `reference/original/WL_*.C` (pushwall, door, areas)
- Analysis plan for sprites: `.hermes/plans/2026-08-04_065054-authentic-sprite-projection.md`

## Action

When prioritized: plan PR (docs + issues only) then implement PR — same ladder style as F1–F4.
