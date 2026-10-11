# Epic index

## Faithful port roadmap (current plan)

Direction ([ADR-002](../adr/002-faithful-fixed-point-port.md)): reimplement gameplay from the original `WL_*.C` logic — 16.16 fixed-point, 70 Hz tics, original state tables and `US_RndT` — so behaviour matches the DOS game. **WL1 shareware first, then WL6.** The reference sources in `reference/original/` are the oracle.

| Phase | Scope | Status |
|-------|-------|--------|
| P1 | Safety net: warnings, ASan/UBSan option, decoder unit tests + CTest, CI build, script fixes | **Done** |
| P2 | Honest docs: known gaps, `GO_NO_GO.md` assessment, ADR-002, FR matrix corrections, outdated plan header and WL1/WL6 wording | **Done** |
| P3 | WL1 data layer: WL1 and WL6 graphics layouts selected by pack extension (`src/id/gfxv.h`); layout is validated against the pack and unsupported versions fail with a clear error | **Done** for the Apogee 1.4 shareware pack (smoke passes on WL1 and WL6); manual WL1 playthrough not yet done |
| P4 | Engine core: fixed-point + 70 Hz loop, original object/AI/state tables, player code, fixed-point renderer (re-run the L1.5 matrix) | **Done** (S0–S7): the faithful engine is the only engine and passes the fidelity gate (all eight original demos in sync, golden frames, E1M1, L1.5); a manual playtest sign-off is the last open check |
| P5 | Game flow: real menus (episode and skill choice, save slots with names, sound and control settings with **separate music and effects volume**), intermission, attract loop + demo playback; then **Q4** audio and **Q5** go/no-go | **In progress**: P5a, P5b done; P5c next |
| R1 | Release readiness (after P5): a shareable download where players drop in their own WL1/WL6 files, for **Windows and Linux** | Planned (see the R1 breakdown below) |
| P6 | WL6: all episodes / 60 maps, remaining enemies and chunks | Planned |
| F4 | Dual pistols — optional, **last**, only after P6 | Deferred |

P1 delivered: `-Wall -Wextra -Wpedantic` (`/W4` on MSVC) with zero project warnings, `-DWOLF_SANITIZE=ON`, `tests/test_decoders.c` via CTest, optional `smoke_only` CTest when `WOLF3D_DATA` is set at configure time, Windows CI (green).

### P4 breakdown (engine core)

Built in parallel with the legacy engine (`--engine=legacy|orig`) until S6, which made the faithful engine the only one; details of the decisions are in ADR-002. Each step lists what it replaces from the original sources.

| Step | Scope | Depends on | Status |
|------|-------|------------|--------|
| S0 | Foundations: 16.16 fixed-point (sign-magnitude `FixedByFrac`), sin/cos/tan tables, `US_RndT` table, 70 Hz tic clock, unit tests; spike: locate `T_DEMO0..3` chunks in WL1 and WL6 | P3 | **Done** (#128) |
| S1 | Data model (`objtype`, `statetype`, `gametype`) and level load (`SetupGameLevel`, `ScanInfoPlane`, areas, doors, statics, difficulty spawns) | S0 | **Done** (#129) |
| S2 | `WL_STATE.C` (movement, chase/dodge/run, sight, damage) + `WL_ACT1.C` + shareware actors of `WL_ACT2.C` as state-table data; deterministic trace tests | S1 | **Done**: S2a (#130) WL_STATE.C, WL_ACT1.C (doors, pushwalls), all 303 non-SPEAR states (generated), soldier/dog/ghost/projectile AI; S2b boss specials, Hitler morph, death cam, BJ victory |
| S3 | `WL_AGENT.C`: player movement/collision, weapons, `Cmd_Fire`/`Cmd_Use`, damage, items | S1 (hit test needs S4a) | **Done** (#132): movement, collision, turning, strafing, `Cmd_Use` (doors, locks, elevators, pushwalls), items, weapon frames, hit test, victory spin |
| S4 | Renderer: S4a exact `TransformActor`/`CalcHeight`/projection; S4b raycaster in C (spec: `WL_DR_A.ASM`); S4c pixel-exact wall/sprite scaling (no compiled scalers) | S1; S4a before S3 | **S4a done** (#133): view size and projection constants, view variables, `TransformActor`/`TransformTile`/`CalcHeight`, `CalcRotate`, `DrawScaleds` visibility (activation, `FL_VISABLE`, item pickup, back-to-front order). **S4b done** (#134): `AsmRefresh` ported register by register, wall/door/pushwall posts, column scaler and VGA masks, floor/ceiling, into the 320x200 frame. **S4c done** (#135): `ScaleShape`/`SimpleScaleShape` from the compiled-shape sprite format with wall occlusion, `DrawPlayerWeapon`, `ThreeDRefresh` |
| S5 | `PlayLoop`: `CalcTics`, original actor update order, doors/pushwalls, HUD hooks | S2, S3, S4 | **S5a done**: `PlayLoop`, `CalcTics`, `PollControls` (keyboard, mouse, joystick, demo record and playback), palette shifts, MLI cheat, music, `PlayDemo`; **7 of 8 original demos end exactly on their last input frame** (in sync), WL1 demo 0 drifts (S7). **S5b done**: `wolf3d.exe --engine=orig` plays the faithful engine (SDL keyboard and mouse as DOS scan codes, 70 Hz clock, screenbuf presentation with palette shifts and FizzleFade, status bar, sound and music, `GameLoop`, `Died`, `DrawPlayScreen`); `--demo`, `--map`, `--skill`, `--viewsize`, `--screenshot`, `--frames`. Menus, intermission and the attract loop are P5 (placeholder screens until then) |
| S6 | Switch default to the new engine, delete legacy float gameplay/draw, save format v2, update smoke markers | S5 | **Done**: the title and stand-in menu start the faithful engine (New Game, Load Game), any game option starts it directly, Esc returns to the menu; legacy gameplay, renderer, status bar, weapon and sprite code deleted (`--engine=legacy` refused); save format v2 (`orig_save.c`: the original `SaveTheGame` contents field by field with pointers as indices, plus the `US_RndT` index, `killerobj`, `LastAttacker`), F8 quick save and F9 quick load; a save made halfway through each original demo loads and plays on to the same end state; smoke suite and markers run the faithful engine |
| S7 | Fidelity gate: original demos replay without desync; local golden frame hashes; E1M1 any% route; L1.5 matrix re-run | S5 | **Done** (`tests/test_orig_fidelity.c`, local: needs the packs): **all eight original demos end exactly on their last input frame at view sizes 7, 12, 14, 17 and 19**. The original plays and records demos at whatever view size is set, and the size changes play (the hit test; which tiles the rays reach, so actor activation and bonus pickup); at the default 15 the shareware demo 0 ends 97 frames early, as the original would at that size. Golden hashes of frames 1, 100, 300 and 600 of every demo (view size 19). L1.5 on E1M1 automated: 5 secrets, a blocked pushwall stays (NOWAYSND), a free one slides and frees its cell, ceiling 0x1d and floor 0x19. E1M1 to the elevator: a bot plays it through the real controls (arrows, Space, Ctrl as scan codes) on hard while recording a demo (1381 frames, 17 kills; godmode, as a bot does not dodge); the demo replays to the same frame. Not automated: a human any% run and the manual L1.5 walk-through |

Engine notes so far (details in the code comments and PRs):
- **Platform wiring (S5b):** hardware and presentation work goes through `orig_hooks.h`; `src/platform/win_sdl/orig_runner.c` fills the hooks for `wolf3d.exe`, and `orig_bridge.c` reaches the shared id layer (the legacy and faithful headers clash, so they never meet in one file). Headless runs (tests) leave the hooks unset. `PlayDemo` draws the play screen after `NewGame`, as the original does.
- **Audio (after S5b, [ADR-003](../adr/003-own-opl2-emulator.md)):** an OPL2 emulator of our own (`src/id/opl2.c`) plays the AdLib music and the AdLib-only effects; `src/id/sd_adlib.c` ports the AdLib half of `ID_SD.C` (140 Hz effects, 700 Hz sequencer). `SD_PlaySound` keeps separate digitized and AdLib channels with their priorities, `SD_SoundPlaying` reports the AdLib effect only (what `UpdateFace` and `Died` test), and digitized sounds are placed in stereo by `SetSoundLoc` / `UpdateSoundLoc`. Shareware AUDIOT holds 11 of the 27 songs (the others are empty), and its AUDIOHED follows the 1.4 (WL6) layout, not `AUDIOWL1.H` in the reference sources.
- **Game pad (after S7):** SDL GameController (XInput on Windows), twin-stick. The left stick strafes through `controlstrafe`, an input that is not in the original (it has one sideways value, `controlx`, that turns or strafes); `PollControls` bounds it like the others and clears it when a demo records or plays, so demos and the fidelity gate are untouched. The mapping (`src/platform/win_sdl/pad_map.c`) has no SDL in it and is unit-tested; `wolf_pad.c` reads the pad; since P5b the control panel reads it like the original's joystick (`Pad_Menu`).
- **State tables** are generated from `WL_ACT2.C` by `scripts/gen_orig_states.sh`; do not edit `orig_states.c/.h` by hand.
- **Original quirks kept on purpose:** a standing dog (info tiles 134–137) spawns nothing and overwrites the previous actor's fields (no supported pack uses those tiles); `bo_clip2` statics keep the slot's previous flags; patrols can share or clear `actorat` spots; a pushwall moves three tiles when `pwallstate` lands exactly on 256 (e.g. one or three tics per frame) and two otherwise; `CheckLine` compares a door's position with the absolute line coordinate, so a door stops blocking sight early; `ALTELEVATORTILE` equals `AREATILE`, so an elevator used while standing in area 0 leads to the secret level; in `GunAttack` a nearer target behind a wall hides farther visible ones; `UpdateFace` draws from `US_RndT` every tic unless the gatling pickup sound is still playing, so sound lengths affect the random sequence; rays mark only the tiles they enter, so the player's own tile is not in `spotvis` unless a ray re-enters it; a new pushwall post does not set `lastside`, so a pushwall in the first column is never flushed by the next wall.
- **Divergences (to confirm with demos in S7):** reads outside the map are bounds-checked (invalid maps only); an actor on a non-floor tile (WL6 map 17: a boss on an ambush tile, area 255) reads as "not connected to the player" where the original read unrelated memory; using a pushwall while it slides away reads its tile (`0xc0 | tile`) as a door past the end of `doorobjlist`, which the original then wrote to and the port ignores; a door or pushwall hit in view column 0 that matches the previous frame's last post read `wallheight[-1]` and keeps its own height in the port; the shareware EXE was built with `UPLOAD` (`WOLF1VER.H`): `A_DeathScream` then skips the secret-level scream and its random draw and picks from two guard screams, so the engine has an `upload` switch for WL1 packs.
- **Renderer dependencies (S4 has to keep them):** while drawing, the original marks actors active and `FL_VISABLE` and sets their `viewx`/`transx` (used by `DoActor`, `T_Shoot` and the player's `GunAttack`/`KnifeAttack`), and picks up bonus items (`GetBonus` from `DrawScaleds`); `centerx`/`shootdelta` come from the view size.

Risks: renderer-set `viewx`/`FL_VISABLE` feed the player's hit test; the menu view size changes `centerx`/`shootdelta`/the projection and so the hit test, so demo replay (S7) has to use the view size the demos were recorded with; sign-magnitude fixed-point rounding; trig tables built with Borland floating point (check against `WOLF3D.EXE` locally); boss throw and death-cam angles from `atan2` kept in a float (x87 vs SSE rounding); 16-bit overflow assumptions; the assembly raycaster.

### P5 breakdown (game flow)

The screens around the game, ported into the engine: they draw into the same 320x200 frame (`screenbuf`) as the 3D view, so they run headless and are testable; the platform only shows the frame, sets the palette and reports input.

| Step | Scope | Status |
|------|-------|--------|
| P5a | UI toolkit: `ID_VH.C` / `ID_VL.C` / `ID_US_1.C` drawing (pictures, the proportional fonts, bars and lines, the print window, palette fades), every VGAGRAPH name for both packs, the status bar and "Get Psyched!" drawn by the engine, `Message()` | **Done**: `src/orig/orig_vh.c`, `orig_menu.c` (window drawing, `Message`), `orig_inter.c` (`PreloadGraphics`); `gfxv.h` has all `GFXV_WL6.H` names, and the 1.4 shareware is the same layout shifted by 12 after its 12 extra help pictures (every picture checked by size in both packs); fades as the original (`DrawPlayScreen`, `PlayDemo`, `PlayLoop`'s fade-in, Get Psyched); `Died`'s red fizzle in the engine; `CheckKeys` keeps `LastScan` from before the MLI cheat clears it, as the original |
| P5b1 | `WL_MENU.C` control panel: main menu, New Game (episode, skill), Sound, Load / Save with ten named slots (`US_LineInput`), Change View, View Scores (`DrawHighScores`, the original table), Quit and End Game with the original messages, `CheckKeys` with F1-F10, pause and the BAT message; the title page leads to it; the stand-in menu is gone | **Done**: `src/orig/orig_menu.c` with the menus' input as the original read it (keyboard table, mouse, joystick) plus a pad (`pad_menu` hook); scripted-input tests on both packs (`test_orig_menu`) with the screens pinned after checking them by eye; per-pack save slots and high scores in `%APPDATA%` |
| P5b2 | Control menu (mouse / joystick, sensitivity, customize keys) plus separate music / effects volume and pad sensitivity; Read This! / F1 help screens (`WL_TEXT.C`) | **Done**: the Control menu, Mouse Sensitivity and Customize controls as the original (settings kept in `config.ini`); Read This! / F1 and `EndText` from `src/orig/orig_text.c` (the shareware only: the registered data's help text is a leftover copy of the shareware's, laid out for pictures VGAGRAPH.WL6 does not have, so with WL6 the main menu has the registered 1.4's nine lines); not in the original: Sound opens a page with the original sound screen plus music and effects volume apart (the OPL2 emulator gives channel 0, the game's AdLib effects, apart from the music's channels 1-8); the main menu's Change View became Video (the original view size screen, fill the screen with sharp bilinear scaling, fullscreen); Control's last line sets the controller look speed; the mouse wheel moves through the menus; a mouse or pad button held on a switch toggles it once |
| P5c | `WL_INTER.C`: level completed (tallies, bonuses, par times), victory, high scores and name entry; death cam text | Planned |
| P5d | `WL_MAIN.C` DemoLoop: title, credits, high scores, demos at the player's view size | Planned |
| P5e | Q4 audio pass, Q5 go/no-go | Planned |

### Known gaps
- Menus: the original control panel runs from the title page (P5b). The intermission and high score entry (P5c) and the credits / high scores / demos between title pages (P5d) are still to come.

### R1 breakdown (release readiness, after P5)

| Item | Scope |
|------|-------|
| License | GPL-2.0 (id's 2012 source release is GPL-2.0); `LICENSE` and notices |
| GitHub | README refreshed for players, screenshots, repo made presentable (and public when ready) |
| Identity | an original pixel-art icon drawn in code (no id artwork; "Wolfenstein" is a ZeniMax trademark), version info in the exe, window title without "(faithful engine)" |
| Package | CI builds a zip on a version tag (exe, `SDL2.dll`, README, LICENSE, an empty `data\` folder "put your WL1/WL6 files here") and attaches it to a GitHub release; game data is never bundled |
| Drop-in | a first-run "choose your game folder" dialog when no data is found |
| Linux | data files found whatever their case (`vswap.wl6`), the "no data" message through SDL, DPI setup only on Windows, a Linux CI build, a tarball or AppImage beside the zip; the renderer stays SDL's choice per platform (Direct3D on Windows, OpenGL on Linux) |
| Windows | a note on SmartScreen for the unsigned exe |
- Demos depend on the view size, as in the original: at the default 15 the shareware demo 0 ends 97 frames early (see S7). The attract loop (P5) will play them at the player's view size, as the original did.
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

**Recommended order of open work:** P1 → P2 → P3 (WL1) → P4 → P5 (incl. **Q4**, **Q5**) → **R1** (release, Windows + Linux) → P6 (WL6) → **F4**. The older L1-first order is superseded.

Gap seed (historical): [LEVEL_GEOMETRY_AUTHENTICITY.md](LEVEL_GEOMETRY_AUTHENTICITY.md) (points at L1).
