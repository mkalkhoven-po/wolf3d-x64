# E1M1 any% reference route (visual + flow)

Accurate playtest / fidelity reference for **shareware floor 1** (map index 0).  
Use with the F1–F3 ladder and Q3 stories — **not** a substitute for stills or the original sources.

## Provenance (what this is / is not)

| Field | Value |
|-------|--------|
| **Primary video** | https://www.youtube.com/shorts/Dyr8gXPKghU |
| **Title (oEmbed)** | `*WR-TIE* E1M1 in 19 Seconds! #wolfenstein3d #shorts` |
| **Channel** | Vintage Valiant (`@VintageValiant`) |
| **Length** | ~27 s container; claim “19 s” route time |
| **Capture style** | Vertical Short crop of classic **320×200-class** play (not our Win64 window chrome) |
| **Captured / noted** | 2026-07-29 session (Hermes) |
| **Transcript** | **Disabled** — no speech captions available |
| **How reviewed** | Metadata + sampled still frames from the Short (not landmarked frame-perfect TAS) |
| **Still art gold** | Player PNG refs under [`docs/product/references/fidelity/`](../product/references/fidelity/) (`ref-f1`…`ref-f3`) |
| **Code gold** | `reference/original/*` (e.g. doors/`DOORWALL`, `PreloadUpdate` 0x37/0x32, status latch) |

### Explicit limits
- **Not** full audio fingerprinting (music / digi): use ear playtest + #74 / DigiMap docs.
- **Not** a CC-license dump of the video files — **link only**; do not commit mp4/frames into git.
- **Not** proof of every E1M1 tile (any% skips most of the map).
- Route timing and WR claim are the **uploader’s**; we care about **what Boss is shown**, not beating the clock.
- VR/Short letterboxing may crop borders; **status bar strip and weapon snout remain readable**.

---

## Route class

**Any% / blind-kill thin route** on E1M1:

- Goal: open doors → reach **elevator switch** → exit floor as fast as possible.
- **Combat is optional**; speed runs may finish with **SCORE still 0**.
- **Ammo / health** can change slightly (e.g. nick of damage, a few shots) without “clearing” the map.
- Completing the floor does **not** require mopping guards, treasures, or secrets.

This is the preferred **manual smoke path** for “can I finish map 0?” after door/elev fixes — complementary to killing everything for Q3 #55.

---

## Observed HUD (frames across the Short)

Blue **STATUSBAR** strip matches original layout (F1 target), not a temporary text HUD.

| Field | Start-ish frames | Late frames (near elev) |
|-------|------------------|-------------------------|
| FLOOR | 1 | 1 |
| SCORE | 0 | 0 (any% — no/few kills scored) |
| LIVES | 3 | 3 |
| Face | BJ idle portrait | Same region; expression set not freeze-framed |
| HEALTH | 100% | **~93%** (took at least one hit) |
| AMMO | 8 | **~6** (spent a couple of rounds) |
| Weapon icon | Pistol silhouette on bar | Pistol |

Parity checks for **F1** (#76 / #80 / #81): all slots present; live digit updates; pistol glyph on bar.

---

## First-person weapon (F2)

- **Hands + pistol** centered in the 3D view above the status bar at all sampled mid-run frames.
- Weapon is **not** only the status-bar icon.
- Rest snout / grip visible while moving; fire animation not the focus of this any% clip (route over TF computes).
- Target: **FR-FID-010**…**012**, epic **#77**.

---

## Doors & walls

From start / mid frames:

- **Teal metal doors** with pad / latch styling; open panels swing clear of the walk corridor.
- Open doorways show **depth into next rooms** (blue/grey further halls) — not flat locked-key art on every leaf.
- Wall set = bold industrial teal / copper panels with rivets; door pages distinct from plain walls.

Port notes (already in playability history):

- Door open/walk: Q3 playtest; elevator **lock=5** must not demand keys (#72).
- Draw path must use **DOORWALL** page pick by lock, not wall tile 90 for every door (#75).
- Elevator switch tile **21 solid** — no walk-through (#75).

---

## Elevator end (exit pad)

Near-end frames show the classic exit motif (partial list for QA eyes):

- Grey / steel elevator bank wall with **red circular switch** (hab) and diamond / hazard plating.
- Adjacent **red banner / flag** counts and teal door leaves in the same sightline.
- Player faces switch with **pistol still in view**; status bar still fully painted.
- Activation is **use key on switch** (original elev cable), not walk-through geometry.

### Port completion contract (this repo)
1. Open any elevator **door** (lock type 5 / `dr_elevator`) without a key.  
2. Approach solid elevator **tile 21**; press **E / Space**.  
3. UI cookie: **LEVEL COMPLETE - PRESS N** (exact string may vary; no broken `=` glyphs).  
4. **N** only advances when `level_complete` is set → next map loads + level music if enabled.  

Stories: Q3 **#56** (retest after #72/#75). Manual radar: this Short is a **visual end-pose check**, not a timer race.

---

## Enemies & bodies (F3)

| Observation | Implication for port |
|-------------|----------------------|
| Living uniformed figures at distance down a teal hall | Must read as **sprites**, not solid cubes → **FR-FID-021**, bug **#73**, epic **#78** / story **#86** |
| Any% leaves most actors alone | Corpses optional on this path; full body fidelity uses other refs (`ref-f3.png`) + kill tests |
| SCORE 0 | Don’t require kill count for “map complete” acceptance |

Props glimpsed (eagle mural / wall emblem, lamp green, banners): **FR-FID-020** / story **#85** — still **`ref-f3.png`** is richer for prop density.

---

## Suggested manual checklist (E1M1 any%)

Use on latest `main` with user WL6 (`--data Wolf3dgame` or double-click exe resolving pack).

| # | Action | Pass if |
|---|--------|---------|
| 1 | New Game → map 0 | Spawn playable; optional Get Psyched (F1 #79) |
| 2 | Open first plain doors with E/Space | Swing open; walk through |
| 3 | Move without falling through walls | Solid collision |
| 4 | Reach exit surroundings | Elev wall **solid**; red switch usable |
| 5 | E on elevator switch | Level-complete cue |
| 6 | N | Next map (or shareware end message if beyond pack maps) |
| 7 | Glance HUD during run | Floor/lives/health/ammo regions make sense (text bar OK until F1) |
| 8 | Glance weapon during run | FP gun is **F2 gap** until implemented; note Partial |

**Do not fail any%** solely because SCORE=0 or few corpses.

Optional combat branch (broader #55): kill ≥1 guard after elev route once, check ammo drop + corpse Partial/#73.

---

## Map onto GitHub / FRs

| Area | Issues / epics | FR IDs |
|------|----------------|--------|
| Status bar full strip | F1 #76, stories #80–#81 | FR-FID-002, FR-FID-003, FR-VID-001 |
| Get Psyched + **red** bar | F1 #79 | FR-FID-001 (0x37/0x32) |
| FP hands/gun | F2 #77, #82–#84 | FR-FID-010…012 |
| Props / enemies / corpses | F3 #78, #85–#87; bug #73 | FR-FID-020…022, NFR-FID-001 |
| Elevator + episode N | Q3 #56 | FR-GAME-008 |
| Doors / move | Q3 #54 (done), residual polish | FR-GAME-003/004 |
| Soft-OPL music feel | #74 / Q4 | FR-AUD-003 Partial |

---

## Related player stills (same visual bar, higher pixel control)

| File | Role |
|------|------|
| `docs/product/references/fidelity/ref-f1.png` | Get Psyched + status bar |
| `docs/product/references/fidelity/ref-f2.png` | Door pad + FP pistol + full HUD |
| `docs/product/references/fidelity/ref-f3.png` | Props, corpses, MG icon on bar |

Prefer stills for **pixel / palette** acceptance; prefer this doc for **route + flow + in-motion check**.

---

## Maintenance

- If the Short goes private/dead, keep this doc: frame notes + oEmbed title are already captured; re-verify with another E1M1 any% if needed.
- Do **not** embed large binary screenshots ripped from third-party videos without need — link + our own `ref-f*.png` already cover art.
- When F1–F3 land, update FR matrix rows from Untested/Fail → Partial/Met using **both** this route and stills.
