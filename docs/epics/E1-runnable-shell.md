# E1 — Runnable shell

## Goal
Produce a Win11 x64 process with an SDL window and main loop (no game).

## User stories
- As a developer, I can configure CMake and build `wolf3d.exe` x64 on Windows 11.
- As a developer, I can launch the exe and see a window that closes on request.

## Functional requirements
- FR-PLAT-001, FR-PLAT-002, FR-PLAT-003, FR-PLAT-004
- FR-VID-004 (quit path — window close)

## Exclusions
- No asset loading, audio, input gameplay, or renderer content (clear colour only)

## Acceptance
- [ ] CMake build succeeds with MSVC or Clang x64
- [ ] Window opens; closes via chrome X without hang
- [ ] README documents bootstrap build steps
