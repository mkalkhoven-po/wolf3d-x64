# Go / no-go (Q5)

**Signed off: Go (2026-10-11)**, after P5e, against the faithful-port bar ([ADR-002](../adr/002-faithful-fixed-point-port.md)).

The bar for this go/no-go is a faithful, playable x64 port of the original engine for the shareware and the full game, on Windows 11 now and on Linux with the R1 release track (license, icon, download package, Linux build). The full game is not played through before release: the maintainer and the players test it live after release, and file what they find as issues.

| Area | FR band | Result (Go / No-Go / Conditional) | Evidence / notes |
|------|---------|-------------------------------------|-------------------|
| Platform | FR-PLAT-* | Go | Native x64 build for Windows (MinGW, MSVC in CI; Linux in R1); every test and smoke run from a non-elevated account (FR-PLAT-002) |
| Data | FR-DATA-* | Go | WL1 (Apogee 1.4 shareware) and WL6 (registered 1.4) found and checked; `bad_data` and `cold_start` CTests; data never in the repo |
| Video / present | FR-VID-* | Go | Original screens throughout, pinned by screenshot tests; fill-screen and fullscreen settings; alt-tab checked by the maintainer (2026-10-11) |
| Input | FR-IN-* | Go | Original keys, mouse and Customize controls; game pad (twin-stick, menus); maintainer playtest 2026-10-11 |
| Gameplay | FR-GAME-* | Go | The original engine in fixed point: all 8 original demos end in sync at their view sizes (S7); E1M1 any% route; intermission, victory, high scores, attract loop (P5c, P5d); maintainer playtests through P5 |
| Audio | FR-AUD-* | Go | Own OPL2 emulator for AdLib music and effects (ADR-003); digitized sound as the SB Pro; level music table identical to the original; Sound menu switches and the volume sliders checked through the mixer (smoke); maintainer ear checks (music, effects) |
| Saves / config | FR-SAVE-*, FR-DATA-004 | Go | Ten named slots per pack, save format v2 (a game saved halfway through a demo loads and ends identically); config and high scores in `%APPDATA%\Wolf3D-x64\` |
| Smoke harness | FR-QA-001 | Go | 48 CTest cases with both packs (unit, engine, menus by scripted input, demo sync, smoke, cold start); Windows CI |

## Gates

- [x] Open `severity:crash` = 0 (2026-10-11: no open issue carries it)
- [x] Open `severity:blocker` = 0 (2026-10-11)
- [x] Q1 smoke green on maintainer machine (2026-10-11, `ctest` 48/48 with WL6 and WL1)
- [x] Q2–Q4 checklists completed or residual risk listed: every item passes, see [PLAYTEST_MANUAL.md](PLAYTEST_MANUAL.md) (2026-10-11)
- [x] FR matrix reviewed; Partial items conscious (2026-10-11)
- [x] Cold start with empty `%APPDATA%\Wolf3D-x64\` once (automated: the `cold_start` CTest)

## Sign-off

| Role | Name | Date | Decision |
|------|------|------|----------|
| Maintainer | mkalkhoven-po | 2026-10-11 | **Go** |

## Residual risk (accepted 2026-10-11)

- **The full game (WL6) is not played through before release.** Engine and screens are shared with the shareware and checked on WL6 data (all demos, menus, screens); the episodes are tested live after release by the maintainer and the players, with bugs filed as issues.
- **Demo sync depends on the view size**, as in the original: at the default size a few demos drift (documented in S7).
- **Public release** (license, icon, download, SmartScreen note for the unsigned exe, the Linux build): the R1 track.
