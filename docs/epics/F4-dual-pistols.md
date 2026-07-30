# F4 — Dual pistols / pistols akimbo (original weapon)

## Origin
Player idea (locked for this port): first **original** weapon variant after stock fidelity.  
**Not** classic DOS loadout — optional progression flavor that still *feels* Wolfenstein.

## Depends on
- **F2 (#77)** — solid **single-pistol** first-person rest + fire frames in the 3D view.  
  Do **not** implement duals before one clean pistol viewmodel exists (mirroring a bad single makes two bad guns).

## Goal
Two pistols on-screen, **centered** as a pair: mirror the VSWAP pistol frame beside the primary, shove the duo so the split between barrels sits where the single snout was (or slightly lower so the F1 status bar stays clear).

Combat signature:

| Parameter | Dual pistols (v1) | Single pistol (stock) |
|-----------|-------------------|------------------------|
| Damage **per bullet** | **Same** as pistol | baseline |
| Fire pattern | **Alternate** L → R → L … | one barrel |
| Cadence | **Slightly faster** than single (DPS↑ from rate only) | baseline `attack_cd` |
| Ammo | **1 round per shot** (shared pool) — burns clips faster | same |
| SFX | **One** digi tick per shot (not double-stacked gunshot) | pistol digi |

Balance target: dual DPS sits **between** pistol and machine gun — never chaingun-tier via naive double damage.

## Art / data policy
- Runtime pixels from **WL6 pack** (`VSWAP` pistol pages) only — see [`docs/product/LOCAL_ASSETS.md`](../product/LOCAL_ASSETS.md).
- **Mirror in software** (horizontal flip or second draw offset); no new committed PNGs / no commercial dumps under `assets/`.
- Status-bar weapon glyph: preferred dedicated dual icon if a free latch page exists; else reuse pistol glyph until art exists (document Partial).

## User stories
1. **Weapon identity** — dual pistols as their own selectable arm (e.g. key **5** or unlock after a defined pickup / best_weapon rule). Must not break knife/pistol/MG/chain 1–4.
2. **Viewmodel** — two pistol sprites (primary + mirrored companion), pair **centered** on the 3D view above STATUSBAR.
3. **Fire clock** — alternate barrels; dmg/shot = pistol; period shorter than single; 1 ammo per tick; empty → existing empty-gun / knife rules.
4. **Smoke / FR** — automated or manual checks for select + fire + no free damage doubling.

## FRs
- **FR-FID-030** Dual-pistol first-person view (two barrels, centered as a pair).
- **FR-GAME-010** Dual-pistol fire: alternate barrels; damage per shot equals pistol; cadence faster than single; shared ammo, one round per shot.
- Ties **FR-FID-010–012** (F2 prerequisite), **FR-GAME-007**, **FR-IN-001**.

## Acceptance
- [ ] Requires F2 pistol rest/fire present and selectable
- [ ] Two pistols visible when dual equipped; pair looks centered (not only a bar icon)
- [ ] Left/right alternate on hold/tap fire; no double-damage single frame
- [ ] Per-shot damage matches pistol; duals feel snappier, not broken
- [ ] Ammo decrements 1 per shot; empty-gun path still sane
- [ ] One pistol digi (or equivalent) per shot — no double SFX stack
- [ ] Smoke green; no pack/lookbook binaries in git
- [ ] FR matrix rows FR-FID-030 / FR-GAME-010 updated Met/Partial

## Out of scope (v1)
- Unique VSWAP dual weapon pages / new enemy drop art  
- Independent per-hand recoil or jam states  
- True simultaneous two-projectile hitscan  
- Full campaign rebalance of all guns  
- Committing Spriters Resource sheets from `assets/`

## Implementation sketch (for later coding epic)
```
// view: after F2 single pistol blit
draw pistol at (cx + off, y)          // right / primary
draw_mirror pistol at (cx - off, y) // left companion
// center = midpoint between grips ≈ former single snout x

// fire:
if (dual && attack ready && ammo > 0) {
  side = next_barrel ^= 1;   // alternate
  fire_hitscan(dmg_pistol);  // same dmg
  ammo--;
  attack_cd = pistol_cd * k; // k in ~0.70–0.85
  play digi once;
}
```
Tune `k` and `off` in one place; document chosen constants in PR body.

## Suggested selection / unlock (decide at implement time)
Pick **one** and stick to it in code + help strip:
- **A.** Key `5` when `best_weapon` / flag unlocks duals  
- **B.** Pistol slot becomes dual after unique pickup (new static type or secret)  
- **C.** Debug/cheat only until campaign placement exists  

Default recommend **A + simple unlock flag** for smoke/manual test.

## GitHub
- Milestone: **F4 — Dual pistols (akimbo)**
- Epic: #96
- Stories: #97 (id+select), #98 (viewmodel), #99 (fire clock), #100 (smoke/FR)


## Status
**PLANNED** — docs + board only until F2 ships and user says implement go.
