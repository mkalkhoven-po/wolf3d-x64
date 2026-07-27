# E2 — Platform contracts

## Goal
Introduce modern typed headers and stub implementations of ID-style APIs (`VL`, `IN`, `SD`, `MM`, `PM`, `CA`, `US`) so game TUs can start linking without DOS headers.

## User stories
- As a developer, I can compile a tiny `main` that calls `Quit`, timed tick stub, and video stub present.
- As a developer, no translation unit requires `dos.h`, `far`, or ASM.

## Functional requirements
- FR-PLAT-001–003
- NFR-002 (no JIT)
- NFR-003 (Quit/error path prints or boxes a message)

## Exclusions
- Real asset codecs, real audio mixer, real raycaster

## Acceptance
- [x] `src/` layout exists with platform + id stub libraries
- [x] Clean compile; zero ASM in build
- [x] Documented mapping table: original module → new file (`docs/architecture/ID_MODULE_MAP.md`)
