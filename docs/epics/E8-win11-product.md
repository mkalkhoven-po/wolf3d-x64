# E8 — Win11 productization

## Goal
Make the build comfortable as a daily desktop app.

## User stories
- As a player, my settings and saves land in AppData.
- As a player, I can toggle fullscreen and integer scales.
- As a player, DPI/scaling does not destroy aspect incorrectly.

## Functional requirements
- FR-DATA-004
- FR-SAVE-002
- FR-VID-003–005
- FR-PLAT-002, FR-PLAT-004

## Exclusions
- Installer/MSI (zip + readme OK)
- Code signing (optional later)
- Store submission

## Acceptance
- [ ] AppData config/saves verified
- [ ] Fullscreen toggle + windowed restore
- [ ] README “player quickstart” complete
- [ ] No writes next to Program Files when installed read-only
