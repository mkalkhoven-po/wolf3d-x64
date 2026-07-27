# Wolfenstein 3D — Win11 x64 Port Analysis & Planning Plan

> **For Hermes:** Planning-only document. Do **not** implement or build until explicitly approved.
> Use this as the basis for a later implementation plan (subagent-driven-development).

**Goal:** Analyse what is required to turn this 1992 Borland C++ 3.0 / 16-bit DOS raycaster into a modern **64-bit Windows 11 native executable**, and produce a phased porting plan (no code written yet).

**Architecture (target recommendation):** Keep original game logic (actors, AI, raycaster math, menus) as a portable C core; replace the entire ID Engine hardware layer with a **platform backend** (SDL2 or SDL3 recommended) that provides windowing, software framebuffer blit, audio, input, and timing. Build as MSVC or Clang/LLVM **x64**, C11/C17 (not C++-required). Drop all real-mode ASM or rewrite critical paths pure C.

**Tech stack (proposed):**
- Language: C17 (MSVC `/std:c17` or clang-cl)
- Window/input/audio: **SDL2** (mature Wolf ports use it) or SDL3
- Optional: OpenGL/D3D11 only for scaled presentation (integer scale / CRT shader) — renderer itself stays software
- Build: CMake + Ninja/MSVC
- Assets: original commercial/shareware data files8 user-supplied (not redistributed)
- Reference ports for study only (not to copy blindly): Wolf4SDL, ECWolf, Chocolate-Wolfenstein, lzwolf

---

## 1. Current context / assumptions

### What this tree is
- Original id Software source drop (2012), DOS 16-bit medium model.
- Entry: `WOLFSRC/WL_MAIN.C` → `main` → `InitGame` → `DemoLoop` → `GameLoop` → play loop → `ThreeDRefresh`.
- ~26 `.C`, ~10 `.ASM`, ~44 `.H`, Borland project `WOLF3D.PRJ`.
- Default SKU flags in `VERSION.H`: `GOODTIMES`, `ARTSEXTERN`, `DEMOSEXTERN`, `CARMACIZED` (full WL6 commercial).
- **No game data** in tree. Runtime needs `VSWAP.*`, `VGAGRAPH.*` / heads/dicts, `GAMEMAPS.*` / `MAPHEAD.*`, `AUDIO*.*`, demos, etc.
- Binary leftovers only: `OBJ/SIGNON.OBJ`, `OBJ/GAMEPAL.OBJ` (OMF blobs — not linkable on modern toolchains without extraction).

### Assumptions for the port
1. User legally owns Wolf3D and/or Spear data; we only ship code + asset loader.
2. Goal is a **faithful** playable port first (same maps, AI, weapons, timing feel), not a remaster.
3. Primary target: **Windows 11 x64** desktop `.exe`. Secondary (later): not required now.
4. We are **not** trying to keep EMS/XMS, Mode-X plane tricks, or IRQ sound blasting as-is.
5. Educational / personal use; respect id’s Limited Use license for commercial distribution questions (legal review flag — see risks).

### Scope this analysis covers
- Technical blockers (memory model, ASM, hardware I/O, types)
- Subsystem-by-subsystem replacement mapping
- Build/toolchain choices
- Asset pipeline
- Phased work plan and estimates
- Risks, open decisions, success criteria

### Out of scope (for now)
- Actual coding / building
- Multiplayer
- Higher vertical FOV / looking up-down / real 3D
- Map editor / mod packaging (unless needed for testing)

---

## 2. Problem map — why it cannot simply “recompile as 64-bit”

| Barrier | Where it lives | Why it breaks on Win11 x64 |
|--------|----------------|----------------------------|
| 16-bit segmented memory (`far`/`near`/`_seg`/`huge`, `memptr = void _seg *`) | All ID_*, WL_DEF.H, almost every pointer | Flat 64-bit address space; keywords invalid; pointer size/semantics change |
| Real-mode / VGA hardware ports | `ID_VL*`, ASM CRT/SC/GC/PEL registers (`0x3C*`, `0xA000`) | No direct VGA I/O; protected OS |
| Mode-X planar framebuffer | `ID_VL`, `ID_VH`, scalers writing planes | Need linear 8-bit (or 32-bit) framebuffer + palette expand |
| Runtime codegen for wall scalers | `WL_SCALE.C` / `CONTIGSC.C` / `OLDSCALE.C` (`BuildCompScale`, emits x86 opcodes + `RETF`) | DEP/NX forbids execute-on-heap by default; 16-bit far calls meaningless; Carmack already called this wrong on cached CPUs |
| IRQ / interrupt vectors | `ID_IN` keyboard ISR (`getvect`/`setvect`), `ID_SD` timer0 ISR ASM | No DOS IVT; must use OS events + high-res timer thread/callback |
| Sound Blaster / AdLib / PC speaker / DMA | `ID_SD.C`, `ID_SD_A.ASM`, `DETECT.C` | Replace with modern audio API; keep Imm/AdLib *data* players in software |
| EMS/XMS page manager | `ID_PM.C/.H` | Entire subsystem can become “load VSWAP chunk into RAM” |
| Borland/DOS CRT & headers | `dos.h`, `bios.h`, `conio.h`, `alloc.h`, `dir.h`, `io.h`, `process.h` | Not available; replace with CRT + Win32/SDL |
| Inline `asm { }` (~1100+ hits) | Heaviest: `ID_SD`, `ID_CA`, `WL_DRAW`, `ID_VL`, scale files | Must rewrite pure C or tiny intrinsics |
| Standalone ASM modules (10 files) | Startup `C0.ASM`, raycast `WL_DR_A.ASM`, video blits, timer/sound, 386 check hacks | Rewrite or delete |
| Hard `sizeof` / `int` = 16-bit assumptions | Savegames, config, packed resource structs, `word`/`int` mix | On MSVC x64: `int`=32, `long`=32, pointers=64 — silent save/struct breaks |
| 8.3 DOS filenames / `findfirst` | `WL_MENU` episode detection `*.WL6` etc. | Use modern directory APIs; case-insensitive paths OK on Windows |
| Linked sign-on screen / palette only as `.OBJ` | `OBJ/SIGNON.OBJ`, `GAMEPAL.OBJ` | Need raw assets extracted or regenerated from commercial data / known dumps |
| Copy protection / beta date checks | `WL_MAIN` (`CopyProtection`, `BETA` date gate) | Strip or `#if 0` for modern build |
| Medium-model calling conventions / `far` function pointers | Actor `statetype` think/action pointers, scaler calls | Convert to plain C function pointers |

Rough inventory:
- **633+** lines matching DOS/hardware keywords (undercounts multi-line ASM blocks)
- **~1123** inline-asm-related hits in C
- **~10** ASM translation units to eliminate
- Core playable logic files that are *mostly* portable after type/pointer cleanup: `WL_ACT*`, `WL_AGENT`, `WL_STATE`, `WL_PLAY` (partial), `WL_GAME` (partial), `WL_INTER`, `WL_TEXT`, menus (UI drawing couples to VH)

---

## 3. Target architecture (recommended)

```
┌──────────────────────────────────────────────────────────┐
│  WinMain / SDL_main                                       │
│  platform/ (win_sdl): window, input, audio, time, fs      │
└─────────────────────────┬────────────────────────────────┘
                          │
┌─────────────────────────▼────────────────────────────────┐
│  id_backend (rewritten ID_* APIs, same names where useful)│
│  VL: 320x200x8 (or 320x240) RAM buffer + palette          │
│  VH: pics/latches → buffer                                │
│  SD: mix PCM + OPL emulator callback                      │
│  IN: scan codes mapped from SDL                           │
│  CA/PM: stdio/mmap file IO + malloc cache                 │
│  MM: thin malloc wrappers (or delete purge logic)         │
└─────────────────────────┬────────────────────────────────┘
                          │
┌─────────────────────────▼────────────────────────────────┐
│  game/  (ported WL_*)                                       │
│  MAIN/GAME/PLAY/DRAW/AGENT/ACT/STATE/MENU/INTER/TEXT      │
│  Raycaster pure C (from WL_DRAW + former WL_DR_A)         │
│  Scalers pure C column loops                              │
└─────────────────────────┬────────────────────────────────┘
                          │
                    User data dir
            VSWAP/VGA*/GAMEMAPS/AUDIO*/CONFIG
```

**Presentation:**
- Internal resolution stay **320×200** (Mode 13h-like logical) or Mode-X logical 320×200 with letterbox — match original aspect (usually 4:3 stretched).
- Host window scales with integer scale + optional bilinear; handle DPI awareness (`SDL_WINDOW_ALLOW_HIGHDPI` careful with logical size).

**Do not** attempt:
- WOW64 16-bit (not supported on x64 Windows)
- Decompiling OMF and linking into MSVC
- Keeping executable heap scalers without a pure-C fallback (skip entirely)

---

## 4. Subsystem replacement plan

### 4.1 Types & headers (foundation)
**Files:** `ID_HEADS.H`, `WL_DEF.H`, new `wl_types.h`

| Old | New |
|-----|-----|
| `boolean` enum | `bool` / `int` with explicit 0/1; care with on-disk sizeof |
| `byte` | `uint8_t` |
| `word` | `uint16_t` |
| `longword` | `uint32_t` |
| `fixed` (`long`) | `int32_t` (keep 16.16) |
| `memptr` / `far *` / `_seg *` | `void *` / `uint8_t *` |
| 16-bit `int` used as storage | audit → `int16_t` where serialized or map-local |
| Borland includes | strip; standard + platform headers |

Deliverable: one compile TU that includes cleaned headers with **zero** `far`/`near`/`dos.h`.

### 4.2 Memory manager (`ID_MM`)
**Strategy:** Replace with thin wrappers (`MM_GetPtr` → `calloc`, `MM_FreePtr` → `free`). Drop purge levels, locked blocks, sorting, EMS hooks.
- Game never depends on thrashing semantics if RAM is plentiful.
- Call sites stay for low churn.

### 4.3 Page manager (`ID_PM`)
**Strategy:** Simplified VSWAP reader.
- Parse page file header once; keep all pages or LRU cache of chunks.
- `PM_GetPage` returns pointer to decompressed/raw chunk in RAM.
- Remove XMS/EMS branches entirely.
- Sound pages and sprite pages must remain lockable during playback/frame (simple refcount).

### 4.4 Cache / assets (`ID_CA`)
**Needs:**
- Binary readers with **explicit endian** (x86 LE OK) and sized types for Huffman/Carmackized map compression (`CARMACIZED`).
- File open paths: search `basepath` + extension (`WL6`/`WL1`/`SOD`/`SDM`) like original `CheckForEpisodes`.
- Map plane loading into `mapsegs[]` as free-standing allocations (not segment offsets).
- Graphics chunks: load into RAM; remove “grstarts” far pointers.

**Binary objects:**
- Extract palette from `GAMEPAL.OBJ` or known GAMEPAL.bin dumps; sign-on from `SIGNON` or `TITLEPIC` path only if needed.
- Prefer loading from VGAGRAPH when `ARTSEXTERN` already expects external art — verify against shareware WL1 set.

### 4.5 Video (`ID_VL` / `ID_VH` + ASM)
**Replace with:**
- `uint8_t framebuffer[320 * 200]` (or height including status bar layout matching original 320×200 with status bar region).
- Palette `SDL_Color[256]` updated on `VL_SetPalette` / fades.
- Present path: convert 8-bit → 32-bit ARGB each frame (or `SDL_PIXELFORMAT_INDEX8` streaming texture if supported).
- `VL_WaitVBL` → vsync or frame limiter (see timing).
- `VH_UpdateScreen` dirty-rects → full frame upload (simpler first).
- Pic drawing: rewrite plane-aware blits as linear chunky blits (port known-good logic from Wolf4SDL as reference algorithm, rewritten).

**Drop:** CRTC start address flip, pel panning ports, screen-to-screen VGA copies.

### 4.6 Wall scalers (`WL_SCALE` / haus)
**Strategy (Carmack-correct):** pure C loop scaler:
```c
// conceptual
for each destination y:
    sample source texel row via fixed step
    write dest pixel with optional shading table
```
- Omit `BuildCompScale` machine-code emit entirely.
- Same for sprite scalers (`WOLFHACK` / `WHACK_A.ASM` `MapRow` → C).

### 4.7 Raycaster (`WL_DRAW` + `WL_DR_A.ASM`)
**Strategy:**
- Port `AsmRefresh` control flow to C (Wolf4SDL already has a known mapping of the same algorithm).
- Keep hit callbacks: `HitHorizWall`, `HitVertWall`, doors, pushwalls.
- Careful: original uses many globals (`viewx`, `viewy`, `pixx`, `tilehit`, …) — keep as file-scope for fidelity or pack into a `rayframe_t` later.
- Fixed-point: ensure 32-bit `fixed` multiplies use `int64_t` intermediates where 16-bit code used 32-bit `long` carefully (watch overflow).

### 4.8 Input (`ID_IN`)
- Map SDL scancodes → original `sc_*` constants (table).
- Mouse relative mode + mouselook-as-turn (original mouse x → turn).
- Joystick via SDL_GameController optional.
- Remove keyboard ISR; poll or event queue each frame into `Keyboard[]` / `LastScan`.

### 4.9 Sound & music (`ID_SD`)
Hardest non-graphics subsystem.

| Original | Modern |
|----------|--------|
| PC speaker | Square-wave synth in callback or drop |
| AdLib OPL2 music + FX | **Nuked OPL3** or **dbopl** emulator fed IMF/ADL-like streams as original data expects |
| Digitised SB DMA | Queue PCM from VSWAP sound pages; SDL audio callback mixer |
| Timer0 ISR rates | SD audio device callback at e.g. 44.1/48 kHz; internal tick accumulator for original `TickBase` (usually 70) |

Preserve:
- Sound priority, positional stereo macros (`PlaySoundLocGlobal`) if cheap (pan).
- Music mode switches from menu.

Phases:
1. Silent build (stubs) → 2. Digi sounds → 3. AdLib SFX → 4. Music.

### 4.10 Timing / game tic
- Original ties logic to timer interrupts (~70 Hz Wolf).
- Port: fixed timestep `while (accumulated >= tic_length) { PlayLoop pieces; }` or match Wolf4SDL’s `SD_GetTimeCount` abstraction using `QueryPerformanceCounter` / `SDL_GetTicks64`.
- Decouple render FPS from tic rate (optional tactical improvement) or keep lockstep for authenticity (decision below).

### 4.11 Game logic (`WL_AGENT`, `WL_ACT1/2`, `WL_STATE`, `WL_PLAY`, `WL_GAME`)
Mostly mechanical:
- Strip `far` from `statetype` function pointers.
- Ensure actor lists use flat pointers.
- Random: `US_RndT` simple table port from ASM.
- Debug cheats (`goobers`) keep behind `#ifdef`.
- Death cam, victory, spear gates compile behind existing ifdefs.

### 4.12 Menus / UI (`WL_MENU`, `WL_INTER`, `WL_TEXT`)
- Heavy pic / font drawing dependencies on VH — works once VH is chunky.
- File picker for savegames: replace DOS `findfirst` with `FindFirstFileW` or `SDL` + `dirent` polyfill; store saves under `%APPDATA%/Wolf3DPort/` (good Win11 citizen).
- Control panel sound tests call SD stubs until ready.

### 4.13 Config / savegames
- **Do not** `write(..., sizeof(struct))` with native layouts.
- Define packed on-disk formats with fixed widths (`#pragma pack` or manual serialize).
- Option A: break compatibility, versioned new format.
- Option B: byte-compatible with DOS saves (harder; requires exact field widths/order).
- Plan default: **new versioned format** + optional importer later.

### 4.14 Product matrix (ifdefs)
First milestone: **one SKU only** — shareware `UPLOAD`/`WL1` *or* full `GOODTIMES`/`WL6` (pick based on test data available).

Defer: Spear, Japan, Spanish strings, demo recording perfection.

Centralise in `config_game.h` generated by CMake options:
`-DWOLF_GOODTIMES=ON` etc.

### 4.15 Startup / process model
- Replace `C0.ASM` / Borland startup with CRT `main` or `SDL_main`.
- Remove `Patch386` / `jabhack` entirely.
- Graceful missing-data dialog (MessageBox / console) listing required files.

---

## 5. Recommended approach choices (decisions to lock before coding)

| # | Decision | Recommendation | Alternatives |
|---|----------|----------------|--------------|
| D1 | Backend library | **SDL2** | SDL3, raw Win32+XAudio2+WASAPI (more work) |
| D2 | Strategy | **Clean-room style port of this tree** with API-compatible ID layer | Fork/adapt Wolf4SDL and only dig into this tree for missing bits (faster, less “our port”) |
| D3 | Renderer | Software 8-bit + GPU scale | Immediate OpenGL textured renderer (not faithful) |
| D4 | ASM | Delete all; C only | Keep MSVC asm (x64 ASM rewrite of raycaster — high cost, low gain) |
| D5 | Wall scale | Simple C loops | JIT scalers (deny on Win11 without VirtualAlloc+RX pages drama) |
| D6 | Audio stack | SDL audio + Nuked OPL | Silent MVP first |
| D7 | Toolchain | CMake + MSVC 2022 x64 | clang-cl, MinGW-w64 (MinGW often easier with SDL packages) |
| D8 | Fidelity | Lockstep ~70 Hz logic | Uncapped render + fixed logic |
| D9 | Data packaging | User points at installed GOG/Steam/CD folder | Ship shareware WL1 if license permits redistrib (usually separate) |
| D10 | DPI/window | Integer scale 2x–5x, aspect-correct 4:3 | Stretch full window |

**Major strategic fork (must decide):**
- **Path A — “Museum port”:** Modernise *this* codebase file-by-file. Educational, slowest, highest authenticity to *these* sources.
- **Path B — “Product port”:** Use Wolf4SDL/ECWolf architecture lessons or as base; treat this tree as reference oracle. Faster to a Win11 `.exe`.

Plan below is written for **Path A** (user asked about *this* code) but every phase lists Path B accelerate option.

---

## 6. Phased work plan (analysis-level tasks — not implementation yet)

> When implementation is approved later, each phase should be split into 2–5 minute tasks with tests. Here phases are HLD milestones.

### Phase 0 — Prep & inventory (0.5–1 day)
- [ ] Confirm legal rate: personal educational build OK; shipping constraints open.
- [ ] Acquire test data: shareware WL1 minimum; full WL6 if targeting GOODTIMES.
- [ ] Snapshot tree; add git if missing; never commit game binaries.
- [ ] Catalogue every `.ASM` export and every ISR/hardware call (table → backend stub).
- [ ] Extract binary assets from OMF (`SIGNON`, `GAMEPAL`) to raw bins for inspection tools.
- [ ] Play baseline in DOSBox with original binary for behavioral reference (timing, sounds, door speed).

**Exit:** Asset pack on disk + Excel/markdown inventory of external dependencies.

### Phase 1 — Skeleton Win64 app (1–2 days)
- [ ] CMake project producing `wolf3d.exe` x64 subsystem console+windows.
- [ ] SDL window clears grey; main loop exits cleanly.
- [ ] Directory layout:
  ```
  /CMakeLists.txt
  /src/game/          (WL_* later)
  /src/id/            (rewritten managers)
  /src/platform/sdl/  (backend)
  /third_party/       (SDL, opl)
  /docs/
  ```
- [ ] CI optional later.

**Exit:** Double-click or `.\wolf3d.exe` opens empty window on Win11.

### Phase 2 — Types + stub ID API (2–4 days)
- [ ] Introduce nested headers; purge Borland syntax so MSVC parses game headers.
- [ ] Stub: `Quit`, `VL_*`, `IN_*`, `SD_*`, `MM_*`, `PM_*`, `CA_*`, `US_*`.
- [ ] Port `main` shell without demos: init stubs → spin → quit.
- [ ] Replace `DemoLoop` with temporary “press Esc” until assets load.

**Exit:** Links pure C, no ASM in build.

### Phase 3 — File IO + asset load (3–6 days)
- [ ] Extension detection (WL1/WL6…).
- [ ] Load MAPHEAD/GAMEMAPS; decompress Carmack/RLEW; dump map plane ASCII for verification.
- [ ] Load VSWAP header; print chunk counts vs known good.
- [ ] Load one pic chunk; dump PNG via offline tool for visual verify.
- [ ] Palette load + show solid palette bars in window.

**Exit:** Known map 0 walls match remainder reference dump.

### Phase 4 — Video + 2D UI path (4–7 days)
- [ ] Chunky framebuffer present @60–70 Hz.
- [ ] Fades, `VWB_DrawPic`, fonts, status bar reclamations.
- [ ] Title / credits / menu screens navigate without 3D.

**Exit:** Can start game from menu into a “loading level…” state.

### Phase 5 — Raycaster + player move (5–10 days)
- [ ] Port projection tables (`finetangent`, sin/cos).
- [ ] C raycast walls + doors + pushwalls.
- [ ] Floor/ceiling solid colors.
- [ ] Player collision `WL_AGENT` minimal.
- [ ] Status bar HUD numbers.

**Exit:** Walk E1M1 (or shareware map) walls correct, doors open.

### Phase 6 — Sprites, actors, combat (5–10 days)
- [ ] Sprite projection + scale C.
- [ ] Static objects (AMMO, keys, food).
- [ ] Guards/dogs FSM from `WL_ACT2`.
- [ ] Weapons, hitscan, damage, death.

**Exit:** Finishable shareware episode mechanically.

### Phase 7 — Audio (4–8 days)
- [ ] Digitized effects.
- [ ] AdLib SFX + music via OPL emu.
- [ ] Menu volume/device options simplified (always “SDL”).

**Exit:** Recognizable music + shoot sounds.

### Phase 8 — Polish Win11 product (3–5 days)
- [ ] `%APPDATA%` config/saves; relative data path argv + env `WOLF3D_DATA`.
- [ ] Fullscreen toggle, integer scale, aspect.
- [ ] Alt-tab / focus loss pause.
- [ ] DPI awareness manifest.
- [ ] Icon, version resource, crash message.
- [ ] Optional: game controller.
- [ ] Remove leftover DOS debug or gate with `-debug`.
- [ ] Performance pass (no heap alloc per frame).

**Exit:** Acceptable daily-driver playable build.

### Phase 9 — SKU expansion (optional)
- Spear, full 6 episodes, language packs.

### Path B accelerate note
If schedule pressures: start from Wolf4SDL (GPLv2 — **license compatibility with id’s non-GPL documentation must be lawyered**) or study-only; or ECWolf (more features). Path A avoids third-party license entanglement with this tree’s license but costs calendar time.

---

## 7. Files — fate map (this tree)

### Rewrite almost completely (platform)
| File | Fate |
|------|------|
| `ID_VL.C/.H`, `ID_VL_A.ASM` | Rewrite chunky SDL backend |
| `ID_VH.C/.H`, `ID_VH_A.ASM` | Rewrite blits |
| `ID_IN.C/.H` | Rewrite SDL input |
| `ID_SD.C/.H`, `ID_SD_A.ASM`, `DETECT.C` | Rewrite audio + timer |
| `ID_MM.C/.H` | Thin malloc |
| `ID_PM.C/.H` | Simple VSWAP cache |
| `ID_CA.C/.H` | Port codecs + modern IO (large but doable) |
| `ID_US_1.C`, `ID_US_A.ASM`, `ID_US.H` | Port rnd/UI helpers; drop ASM |
| `C0.ASM`, `H_LDIV.ASM`, `JABHACK.ASM`, `WL_ASM.ASM` | Delete |
| `WL_DR_A.ASM`, `WHACK_A.ASM` | Rewrite C |
| `WL_SCALE.C`, `CONTIGSC.C`, `OLDSCALE.C`, `WOLFHACK.C` | One C scaler module |
| `MUNGE.C` | Port if still needed for pic prep |

### Port with mechanical edits
| File | Notes |
|------|------|
| `WL_MAIN.C` | main/init/config; strip DOS |
| `WL_GAME.C` | level setup, demos |
| `WL_PLAY.C` | frame loop |
| `WL_DRAW.C` | keep structure; call C refresh |
| `WL_AGENT.C` | player |
| `WL_ACT1.C`, `WL_ACT2.C` | actors |
| `WL_STATE.C` | AI helpers |
| `WL_MENU.C/.H` | menus + findfirst |
| `WL_INTER.C`, `WL_TEXT.C` | intermission/help |
| `WL_DEBUG.C` | optional |
| `WL_DEF.H`, `VERSION.H`, gfx/audio/map headers | types + ifdefs |
| `FOREIGN.H` / `SPANISH.H` / `F_SPEAR.H` | strings as needed |

### Keep as data headers only
`GFXV_*.H`, `AUDIO*.H`, `MAPS*.H`, `*.EQU` (EQU can drop with ASM)

---

## 8. External dependencies to acquire / vendor

| Dep | Purpose | Notes |
|-----|---------|-------|
| SDL2 (or 3) | window/window/audio | Dynamic `SDL2.dll` next to exe or static |
| Nuked-OPL3 / dbopl | AdLib | Small, common in DOS game ports |
| CMake 3.20+ | build | |
| MSVC 2022 Build Tools | x64 compiler | Windows 11 SDK |
| (optional) stb_image_write | debug PNG dumps | |
| (optional) zlib | only if you add PNG; not needed for original data |

No DirectX SDK required if SDL used.

Game data (user supply):
```
*.WL6 or *.WL1 (and matching):
  VSWAP.WL6
  VGAGRAPH.WL6 / VGAHEAD.WL6 / VGADICT.WL6
  GAMEMAPS.WL6 / MAPHEAD.WL6
  AUDIOHED.WL6 / AUDIOT.WL6
  (demos may be embedded chunks)
CONFIG.WL6 created by game
```

---

## 9. Testing & validation plan (for when you build)

### Automated (early)
- Unit tests for:
  - Carmack/RLEW decompress vs golden blobs
  - Fixed-point angle tables checksums
  - Save serialize roundtrip
- “Headless” map load test exit code 0

### Golden behavioral checks
- Demo playback (`PlayDemo`) vs expected end position / score if deterministic
- Screenshot compare title screen palette
- E1L1 speedrun path: keys, door, elevator

### Manual play checklist
- All weapons; knife melee
- Secrets pushwalls
- Boss fights / needle / gift if full version
- Death, load/save, resize window midgame, alt-tab
- Over 1 hour soak (memory leaks)

### Perf budgets (Win11 modest laptop)
- 320×200 sim ≤ mid CPU single core < 5%; 4× scale present < 2% GPU

---

## 10. Effort estimate (Path A, one experienced dev)

| Phase | Calendar (part-time hobby) | Focused full-time |
|-------|----------------------------|-------------------|
| 0 inventory | 1 day | 0.5 |
| 1 skeleton | 1–2 d | 1 |
| 2 stubs/types | 3–5 d | 2 |
| 3 assets | 5–8 d | 3–4 |
| 4 2D video/menu | 5–8 d | 3–4 |
| 5 raycast/move | 7–12 d | 5–7 |
| 6 actors/combat | 7–12 d | 5–7 |
| 7 audio | 5–10 d | 3–5 |
| 8 polish | 4–7 d | 2–3 |
| **Total** | **~6–12 weeks hobby** | **~4–6 weeks FTE** |

Path B (honest reuse of existing SDL port knowledge / base): often **1–2 weeks** to playable if license strategy allows.

Risk buffer +30–50% for obscure bugs (pushwall edge cases, BJ deathcam, Spear-only codepaths).

---

## 11. Risks & tradeoffs

| Risk | Impact | Mitigation |
|------|--------|------------|
| Underestimating ESA/planar graphics assumptions | Wrong pics/colors | Early still dumps; compare DOSBox screenshots |
| Fixed-point overflow when widening ints | Wall wobble, wrong hit | Use `int32_t`/`int64_t` discipline; golden demos |
| Audio timing desync | Wrong music tempo / PC spkr artifacts | Drive music off same tic counter as logic |
| DEP and leftover self-mod code smuggled in | Crash security | Zero JIT; static C only |
| License: id Limited Use + third-party GPL mix | Cannot ship preferred base | Path A pure; or legal review for Path B |
| No SIGNON source | Missing boot muck collage | Scratch logo or extract OMF data |
| Focusing on 64-bit *itself* | Red herring | The work is **DOS abstraction removal**; 64-bit is free once flat C + sized types done |
| Over-engineering ECS/renderer | Delay | YAGNI — faithful software renderer first |
| Savegame `sizeof` footgun | Corrupt saves | Explicit serializers only |

---

## 12. Open questions (need user decisions before implementation plan hardens)

1. **Path A (port this tree)** vs **Path B (base on existing modern port)**?
2. Target SKU for MVP: **shareware WL1** or **full WL6** or **Spear**?
3. Audio MVP: **silent**, digi-only, or full AdLib music day one?
4. Windowed only first, or fullscreen required?
5. Must original DOS savegames load?
6. Any need for map editor / mod hooks?
7. Distribution: private exe only, or public source repo? (license!)
8. Accept SDL2 dependency DLL, or static single-exe preference?
9. Is “exactly this source lineage” a credit/education goal, or is “play Wolf on Win11” the real goal?

---

## 13. Suggested decision defaults (if user wants zero bikeshedding)

- Path A educational, **but** allow reading Wolf4SDL as algorithm reference
- MVP: **WL1 shareware** data for easy testing
- Backend: **SDL2**, dynamic DLL
- Silent → digi → OPL music ladder
- New save format in AppData
- MSVC x64 + CMake
- Integer 4:3 scale, vsync on
- No ASM in final tree

---

## 14. Success criteria (definition of done)

**MVP**
- [ ] Native x64 `.exe` runs on Win11 without DOSBox/emulation
- [ ] Loads user-provided shareware data from configurable path
- [ ] Full first episode playable end-to-end
- [ ] Keyboard + mouse turn
- [ ] Menus, save/load (new format), quit clean
- [ ] No admin rights, no writes next to Program Files without consent
- [ ] Avg frame pacing stable; no audio crackle under load

**Stretch**
- [ ] Full WL6 + SOD
- [ ] Controllers
- [ ] Demo playback parity
- [ ] Optional CRT shader
- [ ] Automated decompress + demo tests in CI

---

## 15. What we explicitly do *not* need

- A 16-bit compiler or Borland C++
- EMS/XMS emulation
- Real VGA Mode-X
- Self-modifying scaler code
- IRQ emulation
- Supporting Windows 32-bit-only APIs first (can do dual later if wanted)
- Rewriting AI “better” on first port

---

## 16. Next step after you approve direction

1. Answer open questions §12 (or accept defaults §13).
2. Produce a **second** plan: bite-sized implementation tasks (Phase 0–1 only) with exact file paths under a new `src/` layout.
3. Only then write code.

---

## Appendix A — Runtime data flow (parity target)

```
user input → IN_* 
     ↓
PlayLoop: Doors_Think, actors state machines, agent attack
     → madenoise / area connectivity
     ↓
ThreeDRefresh:
     wall raycast columns → ScalePost
     TransformActors → scaled sprites
     DrawPlayBorder / status
     ↓
VH/VL present → SDL texture → window
     ↓
SD mixer concurrently from tic/audio thread
```

## Appendix B — Critical “compatibility” types to standardize early

```c
typedef uint8_t  byte;
typedef uint16_t word;
typedef uint32_t longword;
typedef int32_t  fixed;
typedef int16_t  s16; // for map coords storage if needed
typedef uint8_t *byte_p;
typedef void    *memptr;
```

Any struct written to disk gets a parallel `*_disk_t` with fixed layout.

## Appendix C — Existing reference ports (study list)

| Project | Relevance |
|---------|-----------|
| Wolf4SDL | Closest spiritual SDL port of this architecture |
| ECWolf | Advanced, ZDoom-ish; less “lineage pure” |
| Chocolate-Wolfenstein-3D | Faithful-ish |
| lzwolf / lzdoom cousins | Feature heavy |
| DOSBox-X + original EXE | Behavioral oracle |

Use as **oracles and algorithm notes**, not paste sources, unless license path chosen.

---

*End of analysis plan. No source was modified beyond this document.*
