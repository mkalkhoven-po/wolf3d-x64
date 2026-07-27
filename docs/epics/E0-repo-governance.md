# E0 — Repo & governance

## Goal
Establish the GitHub repository, product docs, issue taxonomy, and project board so delivery can proceed in thin vertical slices.

## User stories
- As a maintainer, I can find vision, exclusions, FRs, and ADRs in-repo without chat archaeology.
- As a contributor, I can open a story issue from a template with FR/exclusion sections.
- As a maintainer, I can track M0–M8 on a single project board.

## Functional requirements
- FR-DATA-005 (no game binaries in repo)
- Process only — no runtime FRs

## Exclusions
- No application code required in this epic beyond repo scaffolding
- No CI gate required yet

## Acceptance
- [ ] Private repo `wolf3d-win64` exists
- [ ] `reference/original` contains the original source tree
- [ ] Docs present: VISION, EXCLUSIONS, FRs, ADR-001, epic briefs, port analysis
- [ ] Labels + milestones M0–M8 created
- [ ] Epic and story issues filed
- [ ] GitHub Project board contains those issues
