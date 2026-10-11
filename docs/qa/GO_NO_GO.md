# Go / no-go (Q5)

**Assessment as of 2026-10-11** (after P5e), against the faithful-port bar ([ADR-002](../adr/002-faithful-fixed-point-port.md)). The decision is the maintainer's: see **Sign-off** below.

The bar for this go/no-go is the shareware episode and the full game's engine as a faithful, playable port on Windows 11. Shipping a public download (license, icon, package, Linux) is the R1 release track; the full WL6 playthrough is P6.

| Area | FR band | Result (Go / No-Go / Conditional) | Evidence / notes |
|------|---------|-------------------------------------|-------------------|
| Platform | FR-PLAT-* | Go | Native Win64 build (MinGW, MSVC in CI); every test and smoke run from a non-elevated account (FR-PLAT-002) |
| Data | FR-DATA-* | Go | WL1 (Apogee 1.4 shareware) and WL6 (registered 1.4) found and checked; `bad_data` and `cold_start` CTests; data never in the repo |
| Video / present | FR-VID-* | Conditional | Original screens throughout, pinned by screenshot tests; fill-screen and fullscreen settings. Alt-tab (FR-VID-005) still only informally checked: item 7 of the Q2 checklist |
| Input | FR-IN-* | Go | Original keys, mouse and Customize controls; game pad (twin-stick, menus); maintainer playtest 2026-10-11 |
| Gameplay | FR-GAME-* | Go | The original engine in fixed point: all 8 original demos end in sync at their view sizes (S7); E1M1 any% route; intermission, victory, high scores, attract loop (P5c, P5d); maintainer playtests through P5 |
| Audio | FR-AUD-* | Go | Own OPL2 emulator for AdLib music and effects (ADR-003); digitized sound as the SB Pro; level music table identical to the original; Sound menu switches and the volume sliders checked through the mixer (smoke); maintainer ear checks (music, effects) |
| Saves / config | FR-SAVE-*, FR-DATA-004 | Go | Ten named slots per pack, save format v2 (a game saved halfway through a demo loads and ends identically); config and high scores in `%APPDATA%\Wolf3DWin64\` |
| Smoke harness | FR-QA-001 | Go | 48 CTest cases with both packs (unit, engine, menus by scripted input, demo sync, smoke, cold start); Windows CI |

## Gates

- [x] Open `severity:crash` = 0 (2026-10-11: no open issue carries it)
- [x] Open `severity:blocker` = 0 (2026-10-11)
- [x] Q1 smoke green on maintainer machine (2026-10-11, `ctest` 48/48 with WL6 and WL1)
- [ ] Q2–Q4 checklists completed or residual risk listed: automated and playtested items marked in [PLAYTEST_MANUAL.md](PLAYTEST_MANUAL.md); open for the maintainer: Q2 item 7 (alt-tab), Q4 item 3 (all sound off)
- [x] FR matrix reviewed; Partial items conscious (2026-10-11)
- [x] Cold start with empty `%APPDATA%\Wolf3DWin64\` once (automated: the `cold_start` CTest)

## Sign-off

| Role | Name | Date | Decision |
|------|------|------|----------|
| Maintainer | | | |

## Residual risk (proposed for acceptance)

- **FR-VID-005** alt-tab: informal only until the Q2 checklist's item 7 is done.
- **The full game (WL6)**: engine and screens are shared with the shareware and checked on WL6 data (demos, menus, screens), but all six episodes have not been played through: P6.
- **Demo sync depends on the view size**, as in the original: at the default size a few demos drift (documented in S7).
- **Public release** (license, icon, download, SmartScreen note for the unsigned exe, Linux): the R1 track after P5.
