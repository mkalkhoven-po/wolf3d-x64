# Epic index

## Delivery (done M0–M8)

Epics map 1:1 to milestones **M0–M8**. GitHub Project: [Wolf3D Win64 Port](https://github.com/users/mkalkhoven-po/projects/1).

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
| F4 | [Dual pistols / akimbo](F4-dual-pistols.md) · #96 | Optional original: mirror pistol pair, alt-fire, same dmg/shot · **planned** |

References: `docs/product/references/fidelity/ref-f{1,2,3}.png`.  
In-motion E1M1 any% flow (doors → elevator, HUD/gun/enemy glance): [`docs/qa/E1M1_ANYPERCENT_REFERENCE.md`](../qa/E1M1_ANYPERCENT_REFERENCE.md).  
Runtime pack = WL6 under `Wolf3dgame/` (not `assets/` PNG sheets) — [`docs/product/LOCAL_ASSETS.md`](../product/LOCAL_ASSETS.md).

## Level geometry authenticity (L1)

**Planned epic** — maps/shell feel: pushwalls/secrets, wall projection, floor/ceiling, texture audit.

| Milestone | Epic brief | Focus |
|-----------|------------|--------|
| L1 | [Level geometry & secrets](L1-level-geometry-secrets.md) · **#109** | Pushwalls #110 · projection #111 · floor/ceil #112 · textures #113 · matrix #114 |

**Recommended order among open work** (see L1 brief): playtest residuals → **L1.1 pushwalls** → L1.2 projection → L1.3–4 shell → **F4** dual pistols → **Q4** audio → L1.5 + **Q5**.

Gap seed (historical): [LEVEL_GEOMETRY_AUTHENTICITY.md](LEVEL_GEOMETRY_AUTHENTICITY.md) (points at L1).
