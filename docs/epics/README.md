# Epic index

## Faithful port roadmap (current plan)

Direction ([ADR-002](../adr/002-faithful-fixed-point-port.md)): reimplement gameplay from the original `WL_*.C` logic — 16.16 fixed-point, 70 Hz tics, original state tables and `US_RndT` — so behaviour matches the DOS game. **WL1 shareware first, then WL6.** The reference sources in `reference/original/` are the oracle.

| Phase | Scope | Status |
|-------|-------|--------|
| P1 | Safety net: warnings, ASan/UBSan option, decoder unit tests + CTest, CI build, script fixes | **Done** |
| P2 | Honest docs: known gaps, `GO_NO_GO.md` assessment, ADR-002, FR matrix corrections, outdated plan header and WL1/WL6 wording | **Done** |
| P3 | WL1 data layer: WL1 and WL6 graphics layouts selected by pack extension (`src/id/gfxv.h`); layout is validated against the pack and unsupported versions fail with a clear error | **Done** for the Apogee 1.4 shareware pack (smoke passes on WL1 and WL6); manual WL1 playthrough not yet done |
| P4 | Engine core: fixed-point + 70 Hz loop, original object/AI/state tables, player code, fixed-point renderer (re-run the L1.5 matrix) | Planned |
| P5 | Game flow: real menus, intermission, attract loop + demo playback; then **Q4** audio and **Q5** go/no-go | Planned |
| P6 | WL6: all episodes / 60 maps, remaining enemies and chunks | Planned |
| F4 | Dual pistols — optional, **last**, only after P6 | Deferred |

P1 delivered: `-Wall -Wextra -Wpedantic` (`/W4` on MSVC) with zero project warnings, `-DWOLF_SANITIZE=ON`, `tests/test_decoders.c` via CTest, optional `smoke_only` CTest when `WOLF3D_DATA` is set at configure time, Windows CI (green).

### P4 breakdown (engine core)

Built in parallel with the legacy engine (`--engine=legacy|orig`); details of the decisions are in ADR-002. Each step lists what it replaces from the original sources.

| Step | Scope | Depends on |
|------|-------|------------|
| S0 | Foundations: 16.16 fixed-point (sign-magnitude `FixedByFrac`), sin/cos/tan tables, `US_RndT` table, 70 Hz tic clock, unit tests; spike: locate `T_DEMO0..3` chunks in WL1 and WL6 | P3 |
| S1 | Data model (`objtype`, `statetype`, `gametype`) and level load (`SetupGameLevel`, `ScanInfoPlane`, areas, doors, statics, difficulty spawns) | S0 |
| S2 | `WL_STATE.C` (movement, chase/dodge/run, sight, damage) + `WL_ACT1.C` + shareware actors of `WL_ACT2.C` as state-table data; deterministic trace tests | S1 |
| S3 | `WL_AGENT.C`: player movement/collision, weapons, `Cmd_Fire`/`Cmd_Use`, damage, items | S1 (hit test needs S4a) |
| S4 | Renderer: S4a exact `TransformActor`/`CalcHeight`/projection; S4b raycaster in C (spec: `WL_DR_A.ASM`); S4c pixel-exact wall/sprite scaling (no compiled scalers) | S1; S4a before S3 |
| S5 | `PlayLoop`: `CalcTics`, original actor update order, doors/pushwalls, HUD hooks | S2, S3, S4 |
| S6 | Switch default to the new engine, delete legacy float gameplay/draw, save format v2, update smoke markers | S5 |
| S7 | Fidelity gate: original demos replay without desync; local golden frame hashes; E1M1 any% route; L1.5 matrix re-run | S5 |

Risks: renderer-set `viewx`/`FL_VISABLE` feed the player's hit test; sign-magnitude fixed-point rounding; trig tables built with Borland floating point (check against `WOLF3D.EXE` locally); 16-bit overflow assumptions; the assembly raycaster.

### Known gaps (why M0–M8 is "shell delivered", not "complete")
- Enemy AI is a placeholder (straight-line chase, melee only; no ranged attacks, patrol, ambush, door use or difficulty).
- Float / frame-time gameplay: not deterministic, no demo compatibility.
- Menus: Sound, Control, Load and View are stubs; no intermission, attract loop, demos or cheats.
- Only the Apogee 1.4 shareware WL1 layout is supported; other WL1 versions are rejected at startup.

## Shell delivered (M0–M8)

Epics map 1:1 to milestones **M0–M8**. GitHub Project: [Wolf3D Win64 Port](https://github.com/users/mkalkhoven-po/projects/1). The platform shell, data loading, renderer and audio path exist; gameplay fidelity is addressed by P3–P5 above.

| Milestone | Epic | Demo exit |
|-----------|------|-----------|
| M0 | [E0 Repo & governance](E0-repo-governance.md) | Private repo, docs, board populated |
| M1 | [E1 Runnable shell](E1-runnable-shell.md) | Empty Win11 window quits cleanly |
| M2 | [E2 Platform contracts](E2-platform-contracts.md) | Headers + stubs link; no ASM |
| M3 | [E3 Data path](E3-data-path.md) | User assets + loads map/VSWAP |
| M4 | [E4 2D present & menus](E4-2d-menus.md) | Title + menu navigate |
| M5 | [E5 Move & walls](E5-move-walls.md) | Walk walls/doors |
| M6 | [E6 Playable combat](E6-playable-combat.md) | Combat / episode slice |
| M7 | [E7 Audio](E7-audio.md) | Digi + music path |
| M8 | [E8 Win11 productization](E8-win11-productization.md) | AppData, fullscreen, DPI |

## QA & stabilization (Q0–Q5)

Project: [Wolf3D Win64 — QA & Stabilization](https://github.com/users/mkalkhoven-po/projects/2).  
Docs: [`docs/qa/`](../qa/README.md).

| Milestone | Epic brief | Epic issue |
|-----------|------------|------------|
| Q0 | [QA governance](Q0-qa-governance.md) | #41 |
| Q1 | [Smoke harness](Q1-smoke-harness.md) | #45 |
| Q2 | [Present & data playtest](Q2-present-data-playtest.md) | #49 |
| Q3 | [World & combat playtest](Q3-world-combat-playtest.md) | #53 **Done** |
| Q4 | [Audio & UX playtest](Q4-audio-ux-playtest.md) | #57 |
| Q5 | [Stabilization](Q5-stabilization.md) | #60 |

Labels: `type:epic` / `type:story` / `type:bug` / `type:test` · `area:qa` · `severity:*`.  
Bug workflow: [BUG_TRIAGE.md](../qa/BUG_TRIAGE.md).

## Visual fidelity (F1–F4)

Post-MVP ladder driven by player reference screenshots (official/DOS often used as look-target).  
Board: [Project #1 delivery](https://github.com/users/mkalkhoven-po/projects/1) (implementation) + QA Project #2 for residual bugs.

| Milestone | Epic brief | Focus |
|-----------|------------|--------|
| F1 | [Status HUD & Get Psyched!](F1-status-hud-get-psyched.md) · #76 | `GETPSYCHEDPIC`, `STATUSBARPIC`, digits/face/weapon icon · **Done #95** |
| F2 | [First-person weapon view](F2-weapon-view.md) · #77 | Hands+gun overlay, fire frames, 1–4 switch · **Done #103** |
| F3 | [World statics & corpses](F3-world-statics-corpses.md) · #78 | VSWAP props/enemies/bodies · **Done #104** · authentic projection **#108** |
| F4 | [Dual pistols / akimbo](F4-dual-pistols.md) · #96 | Optional original: mirror pistol pair, alt-fire, same dmg/shot · **deferred — last, after P6** |

References: `docs/product/references/fidelity/ref-f{1,2,3}.png`.  
In-motion E1M1 any% flow (doors → elevator, HUD/gun/enemy glance): [`docs/qa/E1M1_ANYPERCENT_REFERENCE.md`](../qa/E1M1_ANYPERCENT_REFERENCE.md).  
Runtime pack = WL6 under `Wolf3dgame/` (not `assets/` PNG sheets) — [`docs/product/LOCAL_ASSETS.md`](../product/LOCAL_ASSETS.md).

## Level geometry authenticity (L1)

**Implemented** on the current float renderer (L1.1–L1.5). P4 replaces that renderer with a fixed-point one and re-runs the L1.5 matrix; the L1 stories stay closed.

| Milestone | Epic brief | Focus |
|-----------|------------|--------|
| L1 | [Level geometry & secrets](L1-level-geometry-secrets.md) · **#109** | Pushwalls #110 · projection #111 · floor/ceil #112 · textures #113 · matrix #114 |

**Recommended order of open work:** P1 → P2 → P3 (WL1) → P4 → P5 (incl. **Q4**, **Q5**) → P6 (WL6) → **F4**. The older L1-first order is superseded.

Gap seed (historical): [LEVEL_GEOMETRY_AUTHENTICITY.md](LEVEL_GEOMETRY_AUTHENTICITY.md) (points at L1).
