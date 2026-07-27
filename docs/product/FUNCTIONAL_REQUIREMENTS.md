# Functional requirements — MVP

IDs are referenced from GitHub user stories. Status is aspirational until implemented.

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
| FR-GAME-004 | Doors and pushwalls behave per original rules for loaded maps. |
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
