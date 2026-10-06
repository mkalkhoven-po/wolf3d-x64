# Go / no-go (Q5)

**Not signed off.** The table is an assessment as of **2026-10-06** against the faithful-port bar ([ADR-002](../adr/002-faithful-fixed-point-port.md)); the maintainer decision happens in Q5 / #60 after P5.

| Area | FR band | Result (Go / No-Go / Conditional) | Notes / open bugs |
|------|---------|-------------------------------------|-------------------|
| Platform | FR-PLAT-* | Conditional | FR-PLAT-002 (no admin) untested |
| Data | FR-DATA-* | Go | WL6 and WL1 (Apogee 1.4 shareware) smoke pass; other WL1 versions rejected |
| Video / present | FR-VID-* | Go | FR-VID-005 alt-tab only informally checked |
| Input | FR-IN-* | Conditional | Core keys + mouse turn; no Control menu (stub) |
| Gameplay | FR-GAME-* | No-Go | Enemy AI is a placeholder (melee only); no demos/intermission |
| Audio | FR-AUD-* | Conditional | Soft-OPL music is Partial (#74) |
| Saves / config | FR-SAVE-*, FR-DATA-004 | Go | Own `saveN.w64` format; Load menu entry is a stub |
| Smoke harness | FR-QA-001 | Go | 18 markers; P1 added unit tests (CTest) and Windows CI |

## Gates

- [ ] Open `severity:crash` = 0  
- [ ] Open `severity:blocker` = 0  
- [x] Q1 smoke green on maintainer machine (2026-10-06, `ctest` `smoke_only` against WL6)  
- [ ] Q2–Q4 checklists completed or residual risk listed  
- [ ] FR matrix reviewed; Partial items conscious (reviewed 2026-10-06; statuses adjusted)  
- [ ] Cold start with empty `%APPDATA%\Wolf3DWin64\` once  

## Sign-off

| Role | Name | Date | Decision |
|------|------|------|----------|
| Maintainer | | | |

## Residual risk (accepted)

Nothing accepted yet. Known Partial FRs: FR-VID-005, FR-IN-001, FR-IN-002, FR-GAME-006, FR-GAME-007, FR-AUD-003, NFR-001. Known gaps are listed in [`docs/epics/README.md`](../epics/README.md).
