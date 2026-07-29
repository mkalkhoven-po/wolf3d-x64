# F3 — World statics, props & enemy bodies

## Source of truth (player screenshots)
In-room fidelity reference: grey walls, **door + switch plates**, floor lamps, **potted plants**, gold cup / pickups on floor, wall flags/banners, living **guards with distinct uniforms**, **corpses with blood** after kills (not solid color cubes).

## Goal
Replace solid-color billboards with **VSWAP sprites** for:
1. Static decorative / pickup props (`statinfo` SPR_STAT_*).
2. Enemy living frames (directional minimal set: at least front/side where pages exist).
3. Death / body / blood frames left on the floor (corpses society persistence per map session).

Also insure non-door elevator switch plates use correct wall/door page art (already partly DOORWALL).

## User stories
- Props: lamps, plants, tables, flags appear from pack, not mono cubes.
- Pickups remain collectable and look like original art.
- Enemies are distinguishable (guard vs dog etc.).
- Killing leaves a corpse sprite; corpse does not block forever incorrectly.
- Related bug #73 subsumed / closed by this epic when Met.

## FRs
- **FR-FID-020** Static objects drawn from VSWAP sprite pages per map plane1.
- **FR-FID-021** Enemy living sprites from VSWAP (distinctkinds).
- **FR-FID-022** Death/corpse (and blood) sprites remain after kill.
- **NFR-FID-001** Sprite scaling/occlusion uses same camera plane as walls (no teleport z-fight).
- Ties **FR-GAME-005/006**, supersedes Partials that were “colored blips”.

## Acceptance
- [ ] Room screenshot parity “looks like Wolf” for props + bodies
- [ ] #73 closed or reduced to minor frame-count polish
- [ ] Perf acceptable on target Win11 laptop
- [ ] Smoke still green

## Out of scope (later)
- Full AI state machine / smooth rotations across all angles path
- Pushwalls / secret art unless trivial with current maps

## Related route reference
Any% may show **distant living guards with SCORE still 0** — do not require mop-up to validate sprites; full corpse density → `ref-f3.png` + kill tests. See [`docs/qa/E1M1_ANYPERCENT_REFERENCE.md`](../qa/E1M1_ANYPERCENT_REFERENCE.md).

## GitHub

- Epic: #78
- Stories: #85, #86, #87
