# Q1 — Automated smoke harness

## Goal
One-button local smoke with clear PASS/FAIL (no commercial data in git).

## Stories
- #46 smoke_wl6 good + bad path  
- #47 log marker asserts  
- #48 QA runbook docs  

## FRs
FR-PLAT-001, FR-DATA-001–005, FR-AUD-001, FR-DATA-004 smoke path, FR-QA-001  

## Implementation
- `scripts/smoke_wl6.sh` — full harness (markers + exits)
- `scripts/smoke_wl6.bat` — Windows entry (Git Bash preferred)
- `docs/qa/SMOKE.md` — runbook

## Acceptance
- [x] Script in `scripts/`  
- [x] Documented in `docs/qa/SMOKE.md`  
- [x] Green on maintainer WL6 pack  
- [x] Bad path exits 2 and fails harness if not  
