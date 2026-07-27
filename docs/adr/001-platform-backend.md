# ADR-001: Platform backend & port strategy

## Status

Accepted (defaults for MVP)

## Context

Original sources target Borland C++ 3.x, 16-bit DOS, VGA Mode-X, IRQ audio, and segmented memory. We need a Win11 x64 native app.

## Decisions

1. **Path A** — Port from `reference/original` lineage; open-source ports are study oracles only unless a later ADR changes license strategy.
2. **Backend library: SDL2** — window, presentation, input, audio device, timing. No hand-rolled DirectX required for MVP.
3. **Renderer: software** — chunky 8-bit framebuffer + palette; scale to window (integer scale preferred).
4. **No ASM** in the modern tree; raycast and scalers in C.
5. **No JIT scalers** — simple C column loops (Carmack note).
6. **MVP SKU: WL1 shareware** data layout; GOODTIMES/WL6 and Spear deferred.
7. **Saves: new format** in AppData; DOS save import optional later.
8. **Build: CMake + MSVC or Clang x64.**

## Consequences

- All `ID_VL` / `ID_SD` / `ID_IN` / EMS/XMS paths rewritten or stubbed behind SDL.
- Effort concentrated on asset codecs + game logic bring-up, not OS boilerplate.
- Shipping may need `SDL2.dll` beside the exe unless static-linked later.

## Alternatives rejected (for now)

| Alt | Why not MVP |
|-----|-------------|
| Raw Win32 + D3D11 + WASAPI | More code, same outcome |
| Base entire repo on Wolf4SDL | Faster, but license/genealogy ADR needed first |
| Keep DOS + DOSBox | Not a native port |
