# L1 — Level geometry & secrets authenticity

## Origin
Player request: plan level-layout authenticity (secrets, textures, wall height, floor/ceiling) **alongside** F1–F4 and Q4–Q5 — not buried as a footnote.

## Goal
Maps and world shell feel as close to original DOS Wolf as practical:

1. **Pushwalls / secrets** — activate, slide, score, blocking (original rules).
2. **Wall projection** — height/FOV constants toward `CalcProjection` + `SetupScaling` (unify with authentic sprite projection #108).
3. **Floor / ceiling** — original solid palette (or documented Partial if textured floors out of scope).
4. **Texture placement** — wall/door/elev pages match WL6 map tiles (audit + fix mismatches).
5. **Secret playtest** — E1 checklist + FR matrix rows.

## Depends on
- Maps already load (`GAMEMAPS`/`MAPHEAD`) — E3 Met.
- Doors/elev mostly Met (#72/#75, #105–#107).
- World sprites on authentic projection path (#108) — **share** wall height constants with L1.2 (do not fork two projection systems).
- Pack remains runtime-only (`Wolf3dgame/`); never commit WL6.

## Does **not** block
- **F4** dual pistols (viewmodel/combat) — independent.
- **Q4** audio/AppData playtest — independent (except shared smoke).
- **Q5** go/no-go — can run after or in parallel once blockers cleared.

## Priority among open work (historical)

Superseded by the faithful-port roadmap in [README.md](README.md) (P1–P6, F4 last). Kept for context; L1 is implemented.

| Order | Work | Why |
|-------|------|-----|
| 1 | Playtest / residual bugs from F3–#108 | Keep main fun to play |
| 2 | **L1.1 pushwalls** (this epic) | Biggest missing *level* verb; FR-GAME-004 Partial |
| 3 | **L1.2 wall projection unify** | Locks sprite+wall feel together |
| 4 | L1.3–L1.4 floor/ceil + texture audit | Visual shell |
| 5 | **F4** dual pistols | Creative original weapon (planned #96) |
| 6 | **Q4** audio (#74 OPL) + AppData | Soft-OPL already known Partial |
| 7 | L1.5 secret matrix + **Q5** go/no-go | Sign-off |

User may reorder; default **“next epic”** after playtest stabilizes = **L1.1** unless they name F4/Q4.

## User stories

### L1.1 — Pushwalls / secrets
- Push marked walls (original control + tile encoding).
- Wall slides into free space; blocks correctly; secret tally/score if original does.
- Cannot push into invalid cells; sound/cue if original has one.

### L1.2 — Wall height / projection constants
- Document and implement constants vs `WL_MAIN.C` `CalcProjection` / `SetupScaling(viewwidth*1.5)`.
- Walls and world sprites share one height family (post-#108).
- Smoke or debug print depth→line_h at known tiles.

### L1.3 — Floor & ceiling presentation
- Solid floor/ceiling colors match original palette indices for default/areas (research `WL_DRAW` / area tables).
- Textured floors/ceilings **out of scope** unless trivial; document Partial.

### L1.4 — Texture page audit
- Spot-check E1M1+ walls, locked/elev doors, dark areas vs pack + screenshots.
- Fix wrong `(tile-1)*2+side` or door page picks; no purple-index shading regressions.

### L1.5 — Secret / geometry playtest matrix
- Manual checklist: known E1 secrets, push counts, stuck cases.
- FR matrix + playtest manual updated.

### L1.6 — Optional polish (defer)
- Area-based ceiling/floor shifts, door side trim, lighting.

## FRs (new / updated)

| ID | Requirement |
|----|-------------|
| FR-GAME-004 | *(extend)* Pushwalls behave per original for loaded maps (not doors-only Met). |
| FR-FID-040 | Wall strip height uses documented projection constants shared with world sprites. |
| FR-FID-041 | Floor and ceiling fills use original solid palette (or explicit Partial). |
| FR-FID-042 | Wall/door VSWAP pages match map tile kinds (audit on E1). |
| FR-GAME-011 | Secrets/pushwalls: activate, motion, score/tally as original. |

## Acceptance (epic)
- [x] L1.1: at least one E1 secret pushwall completable end-to-end
- [x] L1.2: single projection note in code + walls/sprites not fighting each other
- [x] L1.3: floor/ceil colors signed off or Partial documented
- [x] L1.4: no known wrong-door/wrong-wall page on E1M1 smoke path
- [x] L1.5: matrix + playtest steps landed
- [x] Smoke green; no pack binaries in git

## Out of scope
- Rewriting commercial maps / committing GAMEMAPS
- Full textured floor/ceiling engine
- Spear-only map features
- Dual pistols (F4), AdLib richness (Q4/#74)

## Sources of truth
- `GAMEMAPS` + `MAPHEAD` (tile encoding)
- VSWAP wall pages
- `reference/original/WL_AGENT.C` / `WL_ACT1.C` / `WL_DRAW.C` / `WL_MAIN.C` (pushwall, projection)
- Gap seed: this file (was `LEVEL_GEOMETRY_AUTHENTICITY.md`)
- Sprite projection: `.hermes/plans/2026-08-04_065054-authentic-sprite-projection.md`, #108

## Implementation sketch (later code)

```
// Pushwall (conceptual)
// Map plane marks pushable wall; on Use while facing:
//   if free cell in push dir: animate tile move over N tics; secret++
// Projection:
//   line_h = f(depth)  // same f for walls and ScaleShape height input
// Floor/ceil:
//   fill view y<mid / y>=mid with area palette (original tables)
```

## GitHub
- Milestone: **L1 — Level geometry & secrets** (milestone **20**)
- Epic: **#109**
- Stories: **#110** L1.1 pushwalls · **#111** L1.2 projection · **#112** L1.3 floor/ceil · **#113** L1.4 textures · **#114** L1.5 matrix
- Boards: Project #1 + #2 cards added

## Status
**IMPLEMENTED** (code) — pushwalls + projection note + floor/ceil palette + tile&63. Close board on merge.

## Playtest matrix (L1.5)

Re-run on the faithful engine in P4/S7. The automated column is
`tests/test_orig_fidelity.c` (`orig_fidelity_maps`, `orig_fidelity_maps_wl1`),
on the real E1M1 of each pack.

| Check | By hand | Expected | Automated (S7) |
|-------|---------|----------|----------------|
| Map 0 secrets counted | Finish the floor, count | E1M1 **5** secrets | `secrettotal == 5` |
| Push free wall | Face a pushwall, **Space** | PUSHWALLSND, secret count + 1 | yes |
| Push blocked | A wall or blocking static behind it | NOWAYSND, no move | yes (pushwall 30,22: a static behind) |
| Wall slides | Wait | 2 tiles (3 when `pwallstate` lands on 256) | yes: 2 tiles |
| Walk into vacated cell | Step in | Cell walkable | `tilemap` and `actorat` clear |
| Ceiling color E1M1 | Look up | Palette **0x1d** (vgaCeiling[0]) | top row of the view |
| Floor color | Look down | **0x19** | bottom row of the view |
| Wall tex while pushing | Mid-push | `tile & 63` page (no 0xC0 garbage) | frame drawn mid-slide (UBSan clean) |

Smoke: `scripts/smoke_wl6.sh --data Wolf3dgame` (the legacy `Pushwall smoke:`
marker was retired with the legacy engine in P4/S6).
