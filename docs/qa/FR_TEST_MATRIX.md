# FR test matrix (MVP)

Status values for the **QA phase** (not delivery history):

| Status | Meaning |
|--------|---------|
| **Untested** | Not verified in Q0–Q5 work and no recent smoke/playtest note |
| **Partial** | Implemented enough to play; short of original DOS fidelity or incomplete FR wording |
| **Met** | Acceptable for MVP go/no-go (not necessarily museum-perfect) |
| **Fail** | Broken relative to that Met bar |

**Owner** = primary Q-milestone for verification (bugs can span).

Smoke column: **A** = covered or planned auto markers; **M** = manual / ear-check.

> Seeded at Q0. Update when blocks close. Game data remains user-owned.

## Platform & process

| ID | Method | Smoke | Status | Owner | Notes |
|----|--------|-------|--------|-------|-------|
| FR-PLAT-001 | Build PE32+ x64 `wolf3d.exe` | A | Met | Q1 | MinGW/Ninja delivery path |
| FR-PLAT-002 | Run as normal user | M | Untested | Q2 | No admin assumed |
| FR-PLAT-003 | No DOSBox | A/M | Met | Q0 | Native Path A |
| FR-PLAT-004 | SDL2.dll documented | M | Met | Q0/Q1 | README + player quickstart |

## Data & filesystem

| ID | Method | Smoke | Status | Owner | Notes |
|----|--------|-------|--------|-------|-------|
| FR-DATA-001 | `--data` WL1/WL6 dir | A | Met | Q1/Q2 | WL6 preferred when owned |
| FR-DATA-002 | CLI + `WOLF3D_DATA` | A | Met | Q1/Q2 | + last_data fallback if no `--data` |
| FR-DATA-003 | Bad path lists MISSING, exit ≠0 | A | Met | Q1/Q2 | Explicit bad `--data` must not use last_data |
| FR-DATA-004 | Files under `%APPDATA%\Wolf3DWin64\` | A/M | Met | Q1/Q4 | config.ini, saves, highscores |
| FR-DATA-005 | Repo free of packs | A | Met | Q0 | git ls-tree / PR checks |
| FR-DATA-006 | GAME_DATA.md present | M | Met | Q0 | docs/product |
| FR-DATA-007 | data/ gitignore policy | M | Met | Q0 | |

## Video

| ID | Method | Smoke | Status | Owner | Notes |
|----|--------|-------|--------|-------|-------|
| FR-VID-001 | 320×200 logical FB; HUD string for status band | A/M | Met | F1 | STATUSBARPIC strip (F1); was Partial text HUD |
| FR-VID-002 | Title palette paint non-empty | A/M | Met | Q1/Q2 | |
| FR-VID-003 | Integer scale 4:3 letterbox | A/M | Met | Q2 | +/- keys, present helper |
| FR-VID-004 | Esc / window close | M | Met | Q2 | Player sign-off |
| FR-VID-005 | Alt-tab no crash | M | Partial | Q2/Q4 | Informal |

## Input

| ID | Method | Smoke | Status | Owner | Notes |
|----|--------|-------|--------|-------|-------|
| FR-IN-001 | WASD/arrows/CTRL/E/1–4 | M | Partial | Q3 | Core set present |
| FR-IN-002 | Mouse X turn when enabled | M | Partial | Q3/Q4 | config mouse_look |
| FR-IN-003 | Menu keyboard | M | Met | Q2 | up/down/enter/esc |

## Gameplay

| ID | Method | Smoke | Status | Owner | Notes |
|----|--------|-------|--------|-------|-------|
| FR-GAME-001 | Title → menu | A/M | Met | Q2 | |
| FR-GAME-002 | New Game starts | M | Met | Q2/Q3 | Player sessions |
| FR-GAME-003 | Move + wall collide | A/M | Met | Q3 | Raycast smoke |
| FR-GAME-004 | Doors + pushwalls/secrets | A/M | Partial | L1 | Doors Met (#72/#75); pushwalls → L1.1 |
| FR-GAME-005 | Pickups | A/M | Met | Q3 | UI; digi pickup SFX quiet (honest digimap) |
| FR-GAME-006 | Enemies die | A/M | Met | Q3 | Teleport fixed; sprites = F3/#73 |
| FR-GAME-007 | Weapons hitscan/melee | A/M | Met | Q3 | Empty-gun blocks fire; knife view = F2 |
| FR-GAME-008 | Lives / elevator / episode | A/M | Met | Q3 | Death/R/GO; elev E → N next map (player #56) |

## Audio

| ID | Method | Smoke | Status | Owner | Notes |
|----|--------|-------|--------|-------|-------|
| FR-AUD-001 | Silent fallback | A | Met | Q1/Q4 | |
| FR-AUD-002 | Digi weapons/doors | A/M | Met | Q4 | 46 samples smoke |
| FR-AUD-003 | IMF music menu/game | A/M | Partial | Q4 | MusicGroup audible (#69); soft-OPL not NukedOPL |

## Persistence

| ID | Method | Smoke | Status | Owner | Notes |
|----|--------|-------|--------|-------|-------|
| FR-SAVE-001 | F5/F9 versioned save | A/M | Met | Q4 | `saveN.w64` magic W64S |
| FR-SAVE-002 | Config + high scores | A/M | Met | Q4 | config.ini + highscores.dat |

## Non-functional

| ID | Method | Smoke | Status | Owner | Notes |
|----|--------|-------|--------|-------|-------|
| NFR-001 | Frame dt clamp | M | Partial | Q3/Q5 | ~decoupled present |
| NFR-002 | No JIT heap-exec | M | Met | Q0 | Software path |
| NFR-003 | Clear English errors | A/M | Met | Q1/Q2 | missing data |

## Fidelity ladder (F1–F3 visual parity)

Reference stills: `docs/product/references/fidelity/ref-f{1,2,3}.png`.  
Implementation epics after Q3 playtest closure; bugs #73/#74 feed F3/Q4.

| ID | Method | Smoke | Status | Owner | Notes |
|----|--------|-------|--------|-------|-------|
| FR-FID-001 | Get Psyched! + red preload bar after New Game | A/M | Met | F1 | GETPSYCHEDPIC; bar 0x37/0x32 (#95) |
| FR-FID-002 | STATUSBARPIC layout vs ref-f1/f2 | A/M | Met | F1 | pack status strip + live digits |
| FR-FID-003 | Digit/face/weapon latch art | A/M | Met | F1 | N_*/FACE*/KNIFE* gears |
| FR-FID-010 | FP weapon in 3D view | A/M | Met | F2 | VSWAP SimpleScaleShape; #F2 |
| FR-FID-011 | View weapon switches 1–4 | A/M | Met | F2 | keys 1–4 + best_weapon |
| FR-FID-012 | Attack frame cycle | A/M | Met | F2 | weapon_frame 1..4 over attack_cd |
| FR-FID-020 | Static props VSWAP sprites | A/M | Met | F3 | decor + pickups from STAT pages |
| FR-FID-021 | Enemy living sprites | A/M | Met | F3 | guard/dog/ss/ofc/boss front frames |
| FR-FID-022 | Corpse/blood after kill | A/M | Met | F3 | ST_DEAD stays drawable |
| NFR-FID-001 | Same camera plane as walls | A/M | Met | F3 | billboard shares CAMERA_PLANE |

## Original weapons (F4+)

| ID | Method | Smoke | Status | Owner | Notes |
|----|--------|-------|--------|-------|-------|
| FR-FID-030 | Dual pistol FP pair centered | M | Untested | F4 | Depends on F2; mirror VSWAP pistol |
| FR-GAME-010 | Alternate fire; dmg=pistol; faster rate; 1 ammo/shot | A/M | Untested | F4 | No double-damage frame |



## Level geometry (L1)

| ID | Method | Smoke | Status | Owner | Notes |
|----|--------|-------|--------|-------|-------|
| FR-FID-040 | Wall/sprite shared projection | A/M | Partial | L1 | Unify post-#108 |
| FR-FID-041 | Floor/ceil solid palette | M | Partial | L1 | MVP 0x19/0x18 today |
| FR-FID-042 | Wall/door page audit E1 | M | Untested | L1 | |
| FR-GAME-011 | Pushwall secret playtest | M | Untested | L1 | |

## QA-only

| ID | Requirement | Status | Owner |
|----|-------------|--------|-------|
| FR-QA-001 | Repeatable local smoke script documents PASS/FAIL | Met | Q1 |
| FR-QA-002 | This matrix kept current through Q5 + F ladder | In progress | Q0–Q5/F |