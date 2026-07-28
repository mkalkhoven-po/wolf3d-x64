# E6 — Playable combat

## Goal
Actors, pickups, weapons; finishable shareware episode.

## User stories
- As a player, I can collect keys/ammo/health and fight guards/dogs.
- As a player, I can complete the shareware episode including level transitions and death/lives.

## Functional requirements
- FR-GAME-005–008
- FR-SAVE-001 (basic save/load mid-epic or trailing story) — deferred trailing

## Exclusions
- Boss-edge polish beyond needed to finish WL1
- Spear bosses
- DOS save compatibility
- Full VSWAP sprite frames (billboard colours for E6)

## Acceptance
- [x] Shootables can die (hitscan); player can die and restart/life flow works
- [x] Keys open locked doors (lock types 1–4 mapped to key bits)
- [x] Elevator tile use → level complete; N advances through maps 0–9
- [x] Episode maps 0–9 load in smoke; death/respawn path verified

## Controls
| Input | Action |
|-------|--------|
| WASD / arrows | Move / turn |
| Ctrl / LMB | Shoot |
| E / Space | Use door / elevator |
| 1–4 | Knife / pistol / MG / chain |
| R | Respawn after death (if lives left) |
| N | Next map after elevator |
| Esc | Exit to menu/OS |

## Implementation notes
- `src/game/wl_play.c` — enemies, pickups, keys, shoot, lives
- `src/game/wl_draw.c` — billboard sprites for actors/pickups + HUD bar
- `src/game/wl_game.c` — full combat loop + episode advance
