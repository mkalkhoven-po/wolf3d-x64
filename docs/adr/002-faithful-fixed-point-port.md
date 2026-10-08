# ADR-002: Faithful fixed-point port, WL1 first then WL6

## Status

Accepted (2026-10-06). Amends ADR-001 items 1 and 6; does not change the SDL2 / software-renderer / no-ASM decisions.

## Context

M0–M8 delivered a playable shell, but gameplay was written as a float / frame-time (`dt`) reimplementation, not a port of `WL_STATE.C`, `WL_ACT1.C`, `WL_ACT2.C`, `WL_AGENT.C` and `WL_PLAY.C`. Consequences found in the 2026-10 audit:

- Enemy AI is a placeholder: straight-line chase, melee only; no ranged attacks, patrol, ambush, door use, random damage or difficulty.
- Behaviour is not deterministic, so original demos cannot be replayed and fidelity can only be chased epic by epic (F3, L1).
- Chunk, sound and map tables exist for WL6 only; WL1 is detected by file extension but not supported.

## Decision

1. **Faithful port.** Reimplement gameplay from the original logic: 16.16 fixed-point, 70 Hz tics, the original `statetype` tables as data, `US_RndT` random table. `reference/original/` is the oracle; a change is correct when it matches the original.
2. **WL1 first, then WL6.** WL1 (shareware, 10 maps) is the first complete target; WL6 (60 maps, all episodes) follows. Data tables are selected at runtime by pack extension.
3. **Fidelity check.** Original demo lumps replaying to the same end state are the decisive test once P4 lands.
4. **Optional extras last.** Non-original features (F4 dual pistols) wait until after WL6.
5. **Unchanged from ADR-001:** SDL2 backend, software renderer, no ASM/JIT, new save format in AppData. Build is CMake with MinGW/GCC or MSVC (CI uses MSVC).
6. **Parallel engines.** The faithful engine is built next to the legacy float engine behind `--engine=legacy|orig` and becomes the default only after the fidelity gate passes; the legacy code is then deleted.
7. **Canonical 320x200 renderer.** The game always renders a pixel-exact 320x200 8-bit frame; gameplay (including the player's hit test, which uses the renderer's projection) depends only on that frame's geometry. Higher resolutions are presentation only (scaling of the canonical frame); any true high-resolution render mode is an optional later feature outside the faithful path.
8. **Old saves are dropped** at the switch-over; save format v2 stores original-style state.

## Phases

P1 safety net (done) → P2 honest docs (this change) → P3 WL1 data layer → P4 engine core → P5 game flow, Q4, Q5 → P6 WL6 → F4. Details and status: [`docs/epics/README.md`](../epics/README.md).

## Consequences

- P4 replaces most of `src/game/wl_play.c` and the float path in `src/game/wl_draw.c`; the L1.5 playtest matrix is re-run on the new renderer.
- Existing smoke markers and the AppData save format are kept; save compatibility across P4 is not guaranteed.
- Statuses in the FR matrix and `GO_NO_GO.md` reflect the faithful-port bar, so several FRs drop from Met to Partial until P3–P5 land.

## Out of scope

Spear of Destiny, network play, demo recording, DOS ASM or JIT scalers.
