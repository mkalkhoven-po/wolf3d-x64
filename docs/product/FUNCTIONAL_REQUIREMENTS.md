# Functional requirements — MVP

IDs are referenced from GitHub user stories.  
**Verification status** for the post-E8 phase lives in [`docs/qa/FR_TEST_MATRIX.md`](../qa/FR_TEST_MATRIX.md) (Untested / Partial / Met / Fail).

## Platform & process

| ID | Requirement |
|----|-------------|
| FR-PLAT-001 | Application builds as a 64-bit Windows executable (x64). |
| FR-PLAT-002 | Application runs on Windows 11 without administrator rights. |
| FR-PLAT-003 | Application does not require DOSBox or 16-bit subsystems. |
| FR-PLAT-004 | Dependencies required at runtime are documented (e.g. `SDL2.dll` if dynamic). |

## Data & filesystem

| ID | Requirement |
|----|-------------|
| FR-DATA-001 | User can point the app at a directory containing shareware WL1 data files. |
| FR-DATA-002 | Supported discovery via `--data <path>` and/or `WOLF3D_DATA` environment variable. |
| FR-DATA-003 | If required files are missing, app exits non-zero and lists missing names. |
| FR-DATA-004 | Config and saves write under `%APPDATA%\Wolf3DWin64\` (name finalised in implementation). |
| FR-DATA-005 | Repository and release artifacts never include commercial game data binaries. |
| FR-DATA-006 | Product docs list the required user-supplied data files and when in the roadmap they must be available (see `docs/product/GAME_DATA.md`). |
| FR-DATA-007 | Local optional `data/` folder (if used) remains gitignored; setup never instructs committing `.WL*` / `VSWAP` assets. |

## Video

| ID | Requirement |
|----|-------------|
| FR-VID-001 | Internal renderer produces a 320×200 (logical) 8-bit view consistent with Wolf layout (status bar region included as in original). |
| FR-VID-002 | Palette updates and fades are visible (menu/title/game). |
| FR-VID-003 | Frame is presented in a Win32 window scaled with correct 4:3 intent (integer scale preferred). |
| FR-VID-004 | User can close the window or use an in-game/menu quit path to exit cleanly. |
| FR-VID-005 | Alt-tab / focus loss does not crash the process. |

## Input

| ID | Requirement |
|----|-------------|
| FR-IN-001 | Keyboard controls map to original actions (move, turn, shoot, open, weapons). |
| FR-IN-002 | Mouse X contributes to turning when enabled. |
| FR-IN-003 | Menu navigation works with keyboard. |

## Gameplay

| ID | Requirement |
|----|-------------|
| FR-GAME-001 | Title/demo/menu loop reachable at startup when data present. |
| FR-GAME-002 | User can start a new shareware game from the menu. |
| FR-GAME-003 | Player can move and collide with walls on level geometry loaded from data. |
| FR-GAME-004 | Doors **and pushwalls/secrets** behave per original rules for loaded maps (push motion, blocking, secret tally). |
| FR-GAME-005 | Static pickups (ammo, keys, health, treasures) function. |
| FR-GAME-006 | Enemies spawn, chase/attack, and can be killed. |
| FR-GAME-007 | Weapons and hitscan/melee match original campaign expectations for WL1. |
| FR-GAME-008 | Level completion / death / lives flow allows finishing the shareware episode. |

## Audio (MVP ladder)

| ID | Requirement |
|----|-------------|
| FR-AUD-001 | App runs without audio device (silent fallback) without crashing. |
| FR-AUD-002 | Digitized weapon/enemy/UI sounds play when device available. |
| FR-AUD-003 | AdLib music plays in menus/game when enabled (may lag digi in schedule). |

## Persistence

| ID | Requirement |
|----|-------------|
| FR-SAVE-001 | User can save and load a game using a versioned modern format. |
| FR-SAVE-002 | High scores / config (view size, sensitivity, input toggles) persist across runs. |

## Non-functional

| ID | Requirement |
|----|-------------|
| NFR-001 | Steady logic timing target ~70 Hz original tick base (presentation may be decoupled). |
| NFR-002 | No execute-permission required on heap for scalers (no JIT codegen). |
| NFR-003 | Clear English error strings for missing data and fatal init failures. |

## Fidelity ladder (post-MVP — reference screenshots)

Player reference stills live under `docs/product/references/fidelity/` (`ref-f1.png`…`ref-f3.png`).  
Epic briefs: [`docs/epics/F1-status-hud-get-psyched.md`](../epics/F1-status-hud-get-psyched.md), [`F2-weapon-view.md`](../epics/F2-weapon-view.md), [`F3-world-statics-corpses.md`](../epics/F3-world-statics-corpses.md), [`F4-dual-pistols.md`](../epics/F4-dual-pistols.md).

| ID | Requirement |
|----|-------------|
| FR-FID-001 | After New Game / map load, show pack **Get Psyched!** interstitial (`GETPSYCHEDPIC`) before play view, with a **red** left-to-right preload progress bar under the banner (original palette `0x37`/`0x32`, same red family as the lettering). |
| FR-FID-002 | In-play bottom strip uses original **STATUSBARPIC** layout (FLOOR / SCORE / LIVES / face / HEALTH% / AMMO / weapon glyph). |
| FR-FID-003 | HUD numbers, BJ face, and weapon icon use pack latch/digit art (not only the temporary bitmap font strip). |
| FR-FID-010 | First-person weapon (hands + gun) drawn in the 3D view region above the status bar. |
| FR-FID-011 | View-weapon art switches with inventory / keys 1–4. |
| FR-FID-012 | Attack animation plays at least one cycle per weapon class on fire. |
| FR-FID-020 | Map statics (lamps, plants, flags, cups, etc.) draw from VSWAP sprites, not solid color cubes. |
| FR-FID-021 | Living enemies draw from VSWAP sprite frames; kinds are visually distinguishable. |
| FR-FID-022 | Killed enemies leave corpse/blood sprites for the rest of the map session. |
| NFR-FID-001 | Sprite scale/occlusion uses the same camera plane as walls (no z-fight “teleport” look). |

## Original weapons (post-fidelity, optional)

Player-directed **mod-original** features. Require stock weapon view (F2) before dual art.

| ID | Requirement |
|----|-------------|
| FR-FID-030 | Dual-pistol first-person view shows **two** VSWAP pistol barrels (one mirrored), centered as a pair above the status bar when duals are equipped. |
| FR-GAME-010 | Dual pistols: **alternate** barrel fire; **damage per shot equals** single pistol; cadence **slightly faster** than single (DPS up via rate only); **one ammo** per shot; one gunshot SFX per shot (no double-stack). |

## Level geometry authenticity (L1)

| ID | Requirement |
|----|-------------|
| FR-FID-040 | Wall strip height uses documented projection constants shared with world sprites (`CalcProjection` / `SetupScaling` family). |
| FR-FID-041 | Floor and ceiling fills use original solid palette indices (textured floors out of scope unless noted Partial). |
| FR-FID-042 | Wall and door VSWAP pages match map tile kinds on E1 smoke path (no wrong lock/elev art). |
| FR-GAME-011 | Pushwalls/secrets: player can activate, wall slides per original rules, score/tally updates if applicable. |

