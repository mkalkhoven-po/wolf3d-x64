# QA docs index

Phase 2 of the Wolf3D x64 port: **testing, playtest, and bugfix**.

| Doc | Purpose |
|-----|---------|
| [FR_TEST_MATRIX.md](FR_TEST_MATRIX.md) | Every MVP FR → method, status, owner Q-epic |
| [SMOKE.md](SMOKE.md) | Automated harness `scripts/smoke_wl6.*` |
| [PLAYTEST_MANUAL.md](PLAYTEST_MANUAL.md) | Manual checklists (Q2–Q4) |
| [E1M1_ANYPERCENT_REFERENCE.md](E1M1_ANYPERCENT_REFERENCE.md) | Floor-1 any% route + fidelity notes (Short + stills map) |
| [BUG_TRIAGE.md](BUG_TRIAGE.md) | Severity, labels, project Status hygiene |
| [GO_NO_GO.md](GO_NO_GO.md) | Stabilization sign-off (Q5) |

## GitHub

| Board | URL |
|-------|-----|
| **Delivery (M0–M8, done)** | https://github.com/users/mkalkhoven-po/projects/1 |
| **QA & Stabilization (Q0–Q5)** | https://github.com/users/mkalkhoven-po/projects/2 |
| Repo issues | https://github.com/mkalkhoven-po/wolf3d-x64/issues |
| Milestones | https://github.com/mkalkhoven-po/wolf3d-x64/milestones |

## Epics (repo issues)

| Milestone | Epic issue |
|-----------|------------|
| Q0 | #41 QA governance & FR matrix |
| Q1 | #45 Automated smoke harness |
| Q2 | #49 Present & data playtest |
| Q3 | #53 World & combat playtest |
| Q4 | #57 Audio & UX playtest |
| Q5 | #60 Stabilization & go/no-go |

## Rules

- **Never commit** commercial/shareware packs (`Wolf3dgame/`, `VSWAP.*`, etc.).
- File bugs with **type:bug** + severity label; set Project #2 **Status → Done** when the issue closes.
- Prefer cradle PR text `Fixes #N` for code defects.
