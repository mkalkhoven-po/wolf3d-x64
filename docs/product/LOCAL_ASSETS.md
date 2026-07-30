# Local assets layout (this machine)

> **Policy:** The **running game always loads the official pack formats** from a legally owned WL1/WL6 folder.  
> Optional PNG sheet folders are a **human lookbook only** — never a second runtime path, never committed as commercial art dumps.

This file documents the **convention used in this workspace**. Paths may differ on other machines; the rule does not.

---

## Two different folders (do not confuse them)

| Role | This workspace | What it is | Loaded by `wolf3d.exe`? | Git |
|------|----------------|------------|-------------------------|-----|
| **Runtime pack (source of truth)** | `Wolf3dgame/` at repo root | Full registered **WL6** install: `VSWAP`, `VGAGRAPH`, `AUDIOT`, `GAMEMAPS`, heads/dicts, … | **Yes** | **Ignored** (`/Wolf3dgame/`, `*.WL6`, pack names) |
| **PNG lookbook (optional)** | `assets/` at repo root | Spritesheet / atlas previews (e.g. Guard, HUD, weapons, bosses) from community rips or screen captures | **No** | **Ignored** (`/assets/`, plus `*.png` except `docs/**/*.png`) |

```
D:\wolf3d-master\
  Wolf3dgame\          ← RUNTIME. Point --data / WOLF3D_DATA / exe-adjacent resolve here.
    VSWAP.WL6
    VGAGRAPH.WL6
    VGAHEAD.WL6
    VGADICT.WL6
    GAMEMAPS.WL6
    MAPHEAD.WL6
    AUDIOHED.WL6
    AUDIOT.WL6
    …
  assets\              ← LOOKBOOK ONLY. PNGs for eyes when implementing F1–F3.
    MS-DOS - Wolfenstein 3D - Enemies - Guard.png
    MS-DOS - Wolfenstein 3D - Miscellaneous - HUD.png
    …
  docs\product\references\fidelity\
    ref-f1.png …       ← Acceptance stills (allowed in git under docs/)
```

---

## Runtime pack (always prefer this)

### How the exe finds it
In order (simplified — see `Wolf_ResolveDataDir`):

1. `--data <dir>` or `--data=<dir>`
2. `WOLF3D_DATA` environment variable
3. CWD candidates: `./Wolf3dgame`, `./data`, …
4. Exe-adjacent / `..\Wolf3dgame` (double-click from `build\` still works)

Examples on this machine:

```bat
cd /d D:\wolf3d-master
build\wolf3d.exe --data Wolf3dgame
set WOLF3D_DATA=D:\wolf3d-master\Wolf3dgame
build\wolf3d.exe
scripts\smoke_wl6.bat
```

### What the engine reads from the pack
| Chunk kind | File(s) | Used for |
|------------|---------|----------|
| Walls + doors + sprite pages + digi samples | `VSWAP.WL*` | 3D world, SFX pages, future F2/F3 sprites |
| UI pics / status / fonts / menus | `VGAGRAPH` + `VGAHEAD` + `VGADICT` | Title, Get Psyched, STATUSBAR, CP art |
| Maps | `GAMEMAPS` + `MAPHEAD` | Levels, doors, elevators, spawners |
| Music + AdLib bulk | `AUDIOT` + `AUDIOHED` | MusicGroup IMF, PC/AdLib banks |

**Fidelity work (F1 status bar, F2 weapon frames, F3 enemies) must push pixels from these packs**, same as the original engine’s page/chunk IDs — not by blitting the PNG lookbook at runtime.

Authoritative ownership and file checklist: [`GAME_DATA.md`](GAME_DATA.md).

---

## PNG lookbook (`assets/`)

### What it is for
- Side-by-side **parity checks** while implementing F1–F3 (HUD strip layout, guard silhouettes, weapon poses).
- Quick human browsing without writing an offline VSWAP dumper first.
- Same role as screenshots under `docs/product/references/fidelity/`, but bulk sheets vs acceptance stills.

### What it is **not**
- Not a folder the game searches.
- Not a substitute for missing WL6 files.
- Not something to `git add` (commercial / ripped atlases).
- Not a second texture format the renderer should “prefer over VSWAP.”

### Safe agent / dev rules
1. **Always load / fix pack decoders** when in-game art is wrong (page index, latch ID, palette).  
2. **May open** `assets/*.png` or docs refs for visual comparison on this machine.  
3. **Never** commit `assets/`, `Wolf3dgame/`, or raw pack dumps.  
4. **Never** document a PR as “shipped assets/ care packages.” The PR ships C + docs only.

### Typical sheet names seen here (names vary)
- Enemies: Guard, Guard Dog, Officer, SS, Mutant, Ghosts  
- Bosses: Hans / Gretel Grosse, Schabbs, Giftmacher, Fettgesicht  
- Misc: HUD, HUD & Cutscenes, Menu & Miscellaneous, Objects, Weapons, Fonts  
- Player: B.J. Blazkowicz face sheets  

If a sheet is missing, dump or browse the pack; do not block implementation.

---

## Acceptance stills (git-tracked)

Player-approved frames for the fidelity ladder (OK under git because they live in **docs** and are small PEGs/PNGs used as acceptance — separate from full atlases):

- `docs/product/references/fidelity/ref-f1.png` — Get Psyched + status bar  
- `docs/product/references/fidelity/ref-f2.png` — door + FP pistol + HUD  
- `docs/product/references/fidelity/ref-f3.png` — props, corpses, MG  

In-motion route notes: [`docs/qa/E1M1_ANYPERCENT_REFERENCE.md`](../qa/E1M1_ANYPERCENT_REFERENCE.md).

Rule of thumb:

| Need | Prefer |
|------|--------|
| Engine correctness | WL6 pack decode |
| “Does HUD match the game?” pixel check | `ref-f*.png` |
| “Which frame is guard walk 2?” browse | local `assets/` sheets (optional) |

---

## Git hygiene (this repo)

Already ignored (see root `.gitignore`):

- `/Wolf3dgame/`, `/data/*` pack mounts  
- `*.WL1`, `*.WL6`, `VSWAP.*`, `VGAGRAPH.*`, audio/map names  
- `*.png` globally, with **exception** `!docs/**/*.png`  
- `/assets/` (entire lookbook tree)

Before every PR:

```bat
git status
git ls-files | findstr /i "WL6 VSWAP Wolf3dgame assets"
```

Must show **nothing** pack/lookbook related. Smoke must still pass with local pack present.

---

## FAQ

**Q: I put registered Wolf in the files — can you use it?**  
**A:** Yes. Runtime already does. `Wolf3dgame/` is the pack.  

**Q: I made an `assets/` map of PNGs — do we load that instead?**  
**A:** No. Lookbook only. Engine keeps using VSWAP/VGAGRAPH/etc.  

**Q: Should `assets/` be renamed `lookbook/`?**  
**A:** Optional. Name is fine as long as this doc is followed; folder stays gitignored.  

**Q: Can we commit “fair use” tiny HUD crops?**  
**A:** Prefer living under `docs/product/references/` with explicit acceptance purpose; never dump full atlases or WL6 binaries.
