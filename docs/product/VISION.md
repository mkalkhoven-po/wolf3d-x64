# Vision

## One sentence

Ship a **native Windows 11 x64** executable that plays Wolfenstein 3D (shareware-first) using a faithful software raycaster and user-owned game data — no DOSBox.

## Goals

1. **Native** — PE32+ x64 app; double-click or CLI; no 16-bit subsystem.
2. **Faithful** — Same maps, weapons, enemy behaviour, and approximate 70 Hz game timing as the original logic.
3. **Clean platform boundary** — Game logic separated from video/audio/input/time backends (SDL2).
4. **Good Win11 citizen** — No admin required; config/saves under `%APPDATA%`; clear errors when data files are missing.
5. **Traceable work** — Epics → user stories → FRs; exclusions explicit.

## Non-goals (see EXCLUSIONS.md)

Remaster rendering, multiplayer, looking up/down, shipping commercial assets, keeping Borland/ASM/IRQ paths.

## Success (MVP)

- Playable shareware episode end-to-end on Win11 x64.
- Keyboard + mouse turn.
- Menu → game → save/load (new format) → quit.
- Digitized sound minimum; AdLib music allowed to trail slightly if needed.

## Strategy defaults

| Decision | Default |
|----------|---------|
| Lineage | Path A — port from this tree; study open ports as oracle only |
| Backend | SDL2 |
| Renderer | Software 8-bit + host scale |
| MVP SKU | WL1 shareware |
| Saves | New versioned format in AppData |
| ASM | None in final tree |
