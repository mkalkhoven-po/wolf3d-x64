#!/usr/bin/env bash
set -euo pipefail
export PATH="/c/Program Files/GitHub CLI:$PATH"

REPO="${1:-mkalkhoven-po/wolf3d-x64}"

issue() {
  local title="$1" body="$2" labels="$3" milestone_title="$4"
  local label_args=()
  local existing
  existing=$(gh issue list --repo "$REPO" --state all --search "\"${title}\" in:title" --json title --jq "any(.[]; .title==\"${title}\")")
  if [ "$existing" = "true" ]; then
    echo "skip: $title"
    return 0
  fi
  IFS=',' read -ra parts <<< "$labels"
  for lab in "${parts[@]}"; do
    label_args+=(--label "$lab")
  done
  gh issue create --repo "$REPO" --title "$title" --body "$body" "${label_args[@]}" --milestone "$milestone_title"
}

issue "Epic E0: Repo & governance" "$(cat <<'EOF'
## Epic goal
Establish the GitHub repository, product docs, issue taxonomy, and project board.

## Demo exit criterion
Private repo live with reference tree, docs, labels, milestones, epics/stories, and a project board.

## FR coverage
- FR-DATA-005

## Docs
- docs/epics/E0-repo-governance.md
EOF
)" "type:epic,area:docs,priority:p0" "M0 — Repo & governance"

issue "Story: Product docs are the source of truth" "$(cat <<'EOF'
## User story
As a maintainer, I want vision, exclusions, FRs, and ADRs in-repo so that decisions are not trapped in chat.

## Functional requirements
- Process / FR-DATA-005

## Acceptance criteria
- [x] docs/product/VISION.md
- [x] docs/product/EXCLUSIONS.md
- [x] docs/product/FUNCTIONAL_REQUIREMENTS.md
- [x] docs/adr/001-platform-backend.md
- [x] docs/plans/wolf3d-win64-port-analysis.md

## Technical notes
- Epic: E0 — Milestone: M0
EOF
)" "type:story,area:docs,priority:p0" "M0 — Repo & governance"

issue "Story: Reference original tree is browsable" "$(cat <<'EOF'
## User story
As a developer, I want the unmodified original sources under reference/original so that I can port against the real lineage.

## Functional requirements
- FR-DATA-005

## Acceptance criteria
- [x] reference/original contains WOLFSRC materials
- [x] DeIce installer blobs not required in git
- [x] PORT_NOTICE.md present

## Technical notes
- Epic: E0 — Milestone: M0
EOF
)" "type:story,area:docs,priority:p0" "M0 — Repo & governance"

issue "Epic E1: Runnable shell" "$(cat <<'EOF'
## Epic goal
Win11 x64 process with SDL window and main loop (no game).

## Demo exit criterion
wolf3d.exe opens a window and exits cleanly on close.

## FR coverage
- FR-PLAT-001–004, FR-VID-004

## Dependencies
- Blocked by: E0 — Blocks: E2

## Docs
- docs/epics/E1-runnable-shell.md
EOF
)" "type:epic,area:platform,priority:p0" "M1 — Runnable shell"

issue "Story: CMake x64 build produces wolf3d.exe" "$(cat <<'EOF'
## User story
As a developer, I want CMake + MSVC/Clang x64 to build an executable so that we have a native binary skeleton.

## Functional requirements
- FR-PLAT-001–004

## Acceptance criteria
- [ ] CMake configures on Windows 11
- [ ] Release/Debug x64 build succeeds
- [ ] README documents build steps

## Technical notes
- Epic: E1 — Milestone: M1
EOF
)" "type:story,area:platform,priority:p0" "M1 — Runnable shell"

issue "Story: SDL window opens and quits cleanly" "$(cat <<'EOF'
## User story
As a developer, I want a visible window and main loop so that the platform backend is proven before game bring-up.

## Functional requirements
- FR-VID-004, FR-PLAT-002, FR-PLAT-003

## Acceptance criteria
- [ ] Window opens
- [ ] Close via chrome X exits 0 without hang
- [ ] Focus loss does not crash

## Technical notes
- Epic: E1 — Milestone: M1
EOF
)" "type:story,area:platform,priority:p0" "M1 — Runnable shell"

issue "Epic E2: Platform contracts" "$(cat <<'EOF'
## Epic goal
Modern typed headers + stub ID-style APIs; no DOS/ASM in the build.

## Demo exit criterion
Tiny main calls Quit/video/time stubs; clean link; zero ASM.

## FR coverage
- FR-PLAT-001–003, NFR-002, NFR-003

## Docs
- docs/epics/E2-platform-contracts.md
EOF
)" "type:epic,area:platform,priority:p0" "M2 — Platform contracts"

issue "Story: Sized integer headers replace Borland types" "$(cat <<'EOF'
## User story
As a developer, I want fixed-width types (byte/word/longword/fixed) so that 64-bit builds do not corrupt logic or files.

## Functional requirements
- FR-PLAT-001, NFR-002

## Acceptance criteria
- [ ] Headers compile under MSVC x64
- [ ] No far/near/dos.h required

## Technical notes
- Epic: E2 — Milestone: M2
EOF
)" "type:story,area:platform,priority:p0" "M2 — Platform contracts"

issue "Story: Stub VL/IN/SD/MM/PM/CA APIs link" "$(cat <<'EOF'
## User story
As a developer, I want stub implementations of ID engine entry points so that game TUs can be brought over incrementally.

## Functional requirements
- NFR-003

## Acceptance criteria
- [ ] Stubs exist under src/id and src/platform
- [ ] Mapping table original to new documented

## Technical notes
- Epic: E2 — Milestone: M2
EOF
)" "type:story,area:platform,priority:p0" "M2 — Platform contracts"

issue "Epic E3: Data path" "$(cat <<'EOF'
## Epic goal
Find WL1 data, load core files, fail clearly if missing.

## Demo exit criterion
Valid WL1 folder loads map/VSWAP smoke test; invalid folder lists missing files.

## FR coverage
- FR-DATA-001–003, FR-DATA-005

## Docs
- docs/epics/E3-data-path.md
EOF
)" "type:epic,area:data,priority:p0" "M3 — Data path"

issue "Story: --data / WOLF3D_DATA locates shareware folder" "$(cat <<'EOF'
## User story
As a player, I want to point the app at my shareware folder so that my legally owned data is used without shipping assets.

## Functional requirements
- FR-DATA-001, FR-DATA-002

## Acceptance criteria
- [ ] --data <path> works
- [ ] WOLF3D_DATA works
- [ ] Documented in README

## Technical notes
- Epic: E3 — Milestone: M3
EOF
)" "type:story,area:data,priority:p0" "M3 — Data path"

issue "Story: Missing data reports filenames and nonzero exit" "$(cat <<'EOF'
## User story
As a player, I want clear missing-file errors so that I know what to copy from my install.

## Functional requirements
- FR-DATA-003, NFR-003

## Acceptance criteria
- [ ] Non-zero exit when incomplete set
- [ ] Lists missing expected names

## Technical notes
- Epic: E3 — Milestone: M3
EOF
)" "type:story,area:data,priority:p0" "M3 — Data path"

issue "Story: Load map planes / VSWAP header smoke test" "$(cat <<'EOF'
## User story
As a developer, I want map decompress + VSWAP header parse so that later rendering uses real geometry and chunks.

## Functional requirements
- FR-DATA-001

## Acceptance criteria
- [ ] Carmackized/RLEW path verified on a known map
- [ ] Chunk counts printed or asserted

## Technical notes
- Epic: E3 — Milestone: M3
EOF
)" "type:story,area:data,priority:p0" "M3 — Data path"

issue "Epic E4: 2D present & menus" "$(cat <<'EOF'
## Epic goal
Chunky framebuffer + palette; title and main menu navigation.

## Demo exit criterion
Title colours look right; menu navigate; quit works.

## FR coverage
- FR-VID-001–005, FR-IN-003, FR-GAME-001–002

## Docs
- docs/epics/E4-2d-menus.md
EOF
)" "type:epic,area:video,priority:p0" "M4 — 2D present & menus"

issue "Story: Present 320x200x8 framebuffer with palette" "$(cat <<'EOF'
## User story
As a player, I want correctly coloured frames so that classic art is readable.

## Functional requirements
- FR-VID-001, FR-VID-002, FR-VID-003

## Acceptance criteria
- [ ] Palette fades/title art visible
- [ ] Integer scale 4:3 intent

## Technical notes
- Epic: E4 — Milestone: M4
EOF
)" "type:story,area:video,priority:p0" "M4 — 2D present & menus"

issue "Story: Main menu keyboard navigation" "$(cat <<'EOF'
## User story
As a player, I want to navigate the main menu with the keyboard so that I can start or quit.

## Functional requirements
- FR-IN-003, FR-GAME-001, FR-GAME-002, FR-VID-004

## Acceptance criteria
- [ ] Cursor moves
- [ ] New Game selectable
- [ ] Quit exits cleanly

## Technical notes
- Epic: E4 — Milestone: M4
EOF
)" "type:story,area:gameplay,priority:p0" "M4 — 2D present & menus"

issue "Epic E5: Move & walls" "$(cat <<'EOF'
## Epic goal
Software raycaster + player movement; walk first shareware map.

## Demo exit criterion
Walk E1M1 walls; open a door.

## FR coverage
- FR-GAME-003–004, FR-IN-001–002, FR-VID-001–003, NFR-001–002

## Docs
- docs/epics/E5-move-walls.md
EOF
)" "type:epic,area:gameplay,priority:p0" "M5 — Move & walls"

issue "Story: Raycast textured walls in pure C" "$(cat <<'EOF'
## User story
As a player, I want to see level walls in first person so that the core Wolf fantasy works.

## Functional requirements
- FR-GAME-003, FR-VID-001, NFR-002

## Acceptance criteria
- [ ] Textured walls match reference layout
- [ ] No ASM in renderer

## Technical notes
- Epic: E5 — Milestone: M5
EOF
)" "type:story,area:gameplay,priority:p0" "M5 — Move & walls"

issue "Story: Player collide, turn, and open doors" "$(cat <<'EOF'
## User story
As a player, I want to move, turn (keyboard/mouse), and open doors so that I can traverse a map.

## Functional requirements
- FR-GAME-003, FR-GAME-004, FR-IN-001, FR-IN-002

## Acceptance criteria
- [ ] Collision with walls
- [ ] At least one door opens and is passable
- [ ] Mouse X turns when enabled

## Technical notes
- Epic: E5 — Milestone: M5
EOF
)" "type:story,area:gameplay,priority:p0" "M5 — Move & walls"

issue "Epic E6: Playable combat" "$(cat <<'EOF'
## Epic goal
Actors, pickups, weapons; finishable shareware episode.

## Demo exit criterion
One documented full shareware episode run.

## FR coverage
- FR-GAME-005–008, FR-SAVE-001

## Docs
- docs/epics/E6-playable-combat.md
EOF
)" "type:epic,area:gameplay,priority:p0" "M6 — Playable combat"

issue "Story: Pickups, keys, and locked doors" "$(cat <<'EOF'
## User story
As a player, I want ammo/health/keys and locked doors so that progression matches shareware maps.

## Functional requirements
- FR-GAME-005, FR-GAME-004

## Acceptance criteria
- [ ] Pickups apply
- [ ] Keys unlock corresponding doors

## Technical notes
- Epic: E6 — Milestone: M6
EOF
)" "type:story,area:gameplay,priority:p0" "M6 — Playable combat"

issue "Story: Enemies and weapons make combat work" "$(cat <<'EOF'
## User story
As a player, I want guards/dogs and weapons so that levels are threatening and completable.

## Functional requirements
- FR-GAME-006, FR-GAME-007

## Acceptance criteria
- [ ] Enemies spawn and can kill / be killed
- [ ] Knife + guns function

## Technical notes
- Epic: E6 — Milestone: M6
EOF
)" "type:story,area:gameplay,priority:p0" "M6 — Playable combat"

issue "Story: Finish shareware episode with lives/death flow" "$(cat <<'EOF'
## User story
As a player, I want death, lives, and level completion so that I can finish the shareware episode.

## Functional requirements
- FR-GAME-008, FR-SAVE-001

## Acceptance criteria
- [ ] Elevator/episode path works
- [ ] Death/lives flow works
- [ ] Basic save/load mid-run

## Technical notes
- Epic: E6 — Milestone: M6
EOF
)" "type:story,area:gameplay,priority:p0" "M6 — Playable combat"

issue "Epic E7: Audio" "$(cat <<'EOF'
## Epic goal
SDL audio with digi sounds; AdLib music via OPL emu when ready.

## Demo exit criterion
Shooting sounds right; silent fallback if no device; music enabled path works or explicitly deferred.

## FR coverage
- FR-AUD-001–003

## Docs
- docs/epics/E7-audio.md
EOF
)" "type:epic,area:audio,priority:p1" "M7 — Audio"

issue "Story: Silent fallback and digitized SFX" "$(cat <<'EOF'
## User story
As a player, I want weapon/enemy sounds when possible and silent stability otherwise.

## Functional requirements
- FR-AUD-001, FR-AUD-002

## Acceptance criteria
- [ ] No audio device still plays
- [ ] Digi SFX for shoot/open door

## Technical notes
- Epic: E7 — Milestone: M7
EOF
)" "type:story,area:audio,priority:p1" "M7 — Audio"

issue "Story: AdLib music via OPL emulator" "$(cat <<'EOF'
## User story
As a player, I want classic menu/game music so that atmosphere matches the original.

## Functional requirements
- FR-AUD-003

## Acceptance criteria
- [ ] Music starts in menu or game
- [ ] Can be toggled off

## Technical notes
- Epic: E7 — Milestone: M7
EOF
)" "type:story,area:audio,priority:p1" "M7 — Audio"

issue "Epic E8: Win11 productization" "$(cat <<'EOF'
## Epic goal
Daily-driver desktop behaviour: AppData, fullscreen, DPI, polished quit.

## Demo exit criterion
Player quickstart README; settings/saves in AppData; fullscreen toggle.

## FR coverage
- FR-DATA-004, FR-SAVE-002, FR-VID-003–005, FR-PLAT-002, FR-PLAT-004

## Docs
- docs/epics/E8-win11-product.md
EOF
)" "type:epic,area:platform,priority:p1" "M8 — Win11 productization"

issue "Story: AppData config and modern saves" "$(cat <<'EOF'
## User story
As a player, I want settings and saves in AppData so that Program Files stays read-only friendly.

## Functional requirements
- FR-DATA-004, FR-SAVE-001, FR-SAVE-002

## Acceptance criteria
- [ ] Config created under %APPDATA%
- [ ] Save/load roundtrip

## Technical notes
- Epic: E8 — Milestone: M8
EOF
)" "type:story,area:platform,priority:p1" "M8 — Win11 productization"

issue "Story: Fullscreen, integer scale, DPI-safe present" "$(cat <<'EOF'
## User story
As a player, I want fullscreen and sensible scaling so that the game is comfortable on modern displays.

## Functional requirements
- FR-VID-003, FR-VID-005, FR-PLAT-002

## Acceptance criteria
- [ ] Fullscreen toggle
- [ ] Windowed restore
- [ ] Aspect intent preserved

## Technical notes
- Epic: E8 — Milestone: M8
EOF
)" "type:story,area:video,priority:p1" "M8 — Win11 productization"

echo "Open issues: $(gh issue list --repo "$REPO" --state open --limit 200 --json number --jq 'length')"
