#!/usr/bin/env bash
# Bootstrap GitHub labels, milestones, issues, and Project (v2) after repo exists.
# Usage: ./scripts/bootstrap_github_project.sh OWNER/REPO
set -euo pipefail
export PATH="/c/Program Files/GitHub CLI:$PATH"

REPO="${1:?usage: $0 OWNER/REPO}"
echo "Bootstrapping $REPO"

gh repo view "$REPO" >/dev/null

create_label() {
  local name="$1" color="$2" desc="$3"
  gh label create "$name" --repo "$REPO" --color "$color" --description "$desc" 2>/dev/null \
    || gh label edit "$name" --repo "$REPO" --color "$color" --description "$desc" 2>/dev/null \
    || true
}

echo "== Labels =="
create_label "type:epic" "6F42C1" "Epic outcome"
create_label "type:story" "1D76DB" "User story"
create_label "type:task" "BFD4F2" "Implementation task"
create_label "area:platform" "0E8A16" "SDL/OS/build"
create_label "area:video" "C2E0C6" "Renderer/present"
create_label "area:audio" "FBCA04" "Sound/music"
create_label "area:gameplay" "D93F0B" "Actors/raycast/game"
create_label "area:data" "E99695" "Assets/IO"
create_label "area:docs" "D4C5F9" "Docs/governance"
create_label "priority:p0" "B60205" "MVP critical path"
create_label "priority:p1" "FF9F1C" "MVP important"
create_label "priority:p2" "FEF2C0" "After MVP core"

echo "== Stages = milestones =="
create_ms() {
  local title="$1" desc="$2"
  # create if missing
  if gh api "repos/$REPO/milestones" --jq ".[].title" 2>/dev/null | grep -Fxq "$title"; then
    echo "milestone exists: $title"
  else
    gh api "repos/$REPO/milestones" -f title="$title" -f description="$desc" >/dev/null
    echo "created milestone: $title"
  fi
}

create_ms "M0 — Repo & governance" "E0: docs, board, reference tree"
create_ms "M1 — Runnable shell" "E1: empty SDL window x64"
create_ms "M2 — Platform contracts" "E2: ID API stubs, no ASM"
create_ms "M3 — Data path" "E3: WL1 load + errors"
create_ms "M4 — 2D present & menus" "E4: title + menu"
create_ms "M5 — Move & walls" "E5: raycast walk"
create_ms "M6 — Playable combat" "E6: finish shareware episode"
create_ms "M7 — Audio" "E7: digi + music"
create_ms "M8 — Win11 productization" "E8: AppData, fullscreen, polish"

ms_number() {
  gh api "repos/$REPO/milestones" --jq ".[] | select(.title==\"$1\") | .number"
}

issue() {
  local title="$1" body="$2" labels="$3" milestone_title="$4"
  local ms label_args=()
  ms=$(ms_number "$milestone_title")
  IFS=',' read -ra parts <<< "$labels"
  for lab in "${parts[@]}"; do
    label_args+=(--label "$lab")
  done
  gh issue create --repo "$REPO" --title "$title" --body "$body" "${label_args[@]}" --milestone "$ms"
}

echo "== Epic + story issues =="

# Helper: create epic then stories, print numbers

E0=$(issue "Epic E0: Repo & governance" "$(cat <<'EOF'
## Epic goal
Establish the GitHub repository, product docs, issue taxonomy, and project board.

## Demo exit criterion
Private repo live with reference tree, docs, labels, milestones, epics/stories, and a project board.

## FR coverage
- FR-DATA-005

## Exclusions
- No application runtime code required beyond scaffolding

## Docs
- `docs/epics/E0-repo-governance.md`
EOF
)" "type:epic,area:docs,priority:p0" "M0 — Repo & governance")

issue "Story: Product docs are the source of truth" "$(cat <<'EOF'
## User story
As a maintainer, I want vision, exclusions, FRs, and ADRs in-repo so that decisions are not trapped in chat.

## Functional requirements
- Process / FR-DATA-005

## Exclusions
- Runtime features

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
- FR-DATA-005 (no game binaries)

## Exclusions
- Editing original sources in place for the modern port

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
`wolf3d.exe` opens a window and exits cleanly on close.

## FR coverage
- FR-PLAT-001, FR-PLAT-002, FR-PLAT-003, FR-PLAT-004, FR-VID-004

## Exclusions
- Assets, audio, gameplay

## Docs
- `docs/epics/E1-runnable-shell.md`

## Dependencies
- Blocked by: E0
- Blocks: E2
EOF
)" "type:epic,area:platform,priority:p0" "M1 — Runnable shell"

issue "Story: CMake x64 build produces wolf3d.exe" "$(cat <<'EOF'
## User story
As a developer, I want CMake + MSVC/Clang x64 to build an executable so that we have a native binary skeleton.

## Functional requirements
- FR-PLAT-001–004

## Exclusions
- Game code

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

## Exclusions
- Rendering game content (clear colour OK)

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
- `docs/epics/E2-platform-contracts.md`

## Dependencies
- Blocked by: E1
- Blocks: E3–E5
EOF
)" "type:epic,area:platform,priority:p0" "M2 — Platform contracts"

issue "Story: Sized integer headers replace Borland types" "$(cat <<'EOF'
## User story
As a developer, I want fixed-width types (byte/word/longword/fixed) so that 64-bit builds do not corrupt logic or files.

## Functional requirements
- FR-PLAT-001, NFR-002

## Exclusions
- Full game port

## Acceptance criteria
- [ ] Headers compile under MSVC x64
- [ ] No `far`/`near`/`dos.h` required

## Technical notes
- Epic: E2 — Milestone: M2
EOF
)" "type:story,area:platform,priority:p0" "M2 — Platform contracts"

issue "Story: Stub VL/IN/SD/MM/PM/CA APIs link" "$(cat <<'EOF'
## User story
As a developer, I want stub implementations of ID engine entry points so that game TUs can be brought over incrementally.

## Functional requirements
- NFR-003

## Exclusions
- Real codecs/audio/raycast

## Acceptance criteria
- [ ] Stubs exist under src/id and src/platform
- [ ] Mapping table original→new documented

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
- `docs/epics/E3-data-path.md`

## Dependencies
- Blocked by: E2
- Blocks: E4–E6
EOF
)" "type:epic,area:data,priority:p0" "M3 — Data path"

issue "Story: --data / WOLF3D_DATA locates shareware folder" "$(cat <<'EOF'
## User story
As a player, I want to point the app at my shareware folder so that my legally owned data is used without shipping assets.

## Functional requirements
- FR-DATA-001, FR-DATA-002

## Exclusions
- Auto-installers downloading pirated data

## Acceptance criteria
- [ ] `--data <path>` works
- [ ] `WOLF3D_DATA` works
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

## Exclusions
- GUI file picker (CLI/env enough for MVP)

## Acceptance criteria
- [ ] Non-zero exit when incomplete set
- [ ] Lists missing expected names (VSWAP/VGA*/MAP*)

## Technical notes
- Epic: E3 — Milestone: M3
EOF
)" "type:story,area:data,priority:p0" "M3 — Data path"

issue "Story: Load map planes / VSWAP header smoke test" "$(cat <<'EOF'
## User story
As a developer, I want map decompress + VSWAP header parse so that later rendering uses real geometry and chunks.

## Functional requirements
- FR-DATA-001

## Exclusions
- Full graphic drawing

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
- `docs/epics/E4-2d-menus.md`
EOF
)" "type:epic,area:video,priority:p0" "M4 — 2D present & menus"

issue "Story: Present 320x200x8 framebuffer with palette" "$(cat <<'EOF'
## User story
As a player, I want correctly coloured frames so that classic art is readable.

## Functional requirements
- FR-VID-001, FR-VID-002, FR-VID-003

## Exclusions
- 3D raycast

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

## Exclusions
- Full options persistence

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
- `docs/epics/E5-move-walls.md`
EOF
)" "type:epic,area:gameplay,priority:p0" "M5 — Move & walls"

issue "Story: Raycast textured walls in pure C" "$(cat <<'EOF'
## User story
As a player, I want to see level walls in first person so that the core Wolf fantasy works.

## Functional requirements
- FR-GAME-003, FR-VID-001, NFR-002

## Exclusions
- JIT scalers; enemies

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

## Exclusions
- Combat

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
- `docs/epics/E6-playable-combat.md`
EOF
)" "type:epic,area:gameplay,priority:p0" "M6 — Playable combat"

issue "Story: Pickups, keys, and locked doors" "$(cat <<'EOF'
## User story
As a player, I want ammo/health/keys and locked doors so that progression matches shareware maps.

## Functional requirements
- FR-GAME-005, FR-GAME-004

## Exclusions
- Spear-only items

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

## Exclusions
- AI rebalance; new weapons

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

## Exclusions
- DOS save compatibility

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
- `docs/epics/E7-audio.md`
EOF
)" "type:epic,area:audio,priority:p1" "M7 — Audio"

issue "Story: Silent fallback and digitized SFX" "$(cat <<'EOF'
## User story
As a player, I want weapon/enemy sounds when possible and silent stability otherwise.

## Functional requirements
- FR-AUD-001, FR-AUD-002

## Exclusions
- Music

## Acceptance criteria
- [ ] No audio device → still plays
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

## Exclusions
- Hardware OPL

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
- `docs/epics/E8-win11-product.md`
EOF
)" "type:epic,area:platform,priority:p1" "M8 — Win11 productization"

issue "Story: AppData config and modern saves" "$(cat <<'EOF'
## User story
As a player, I want settings and saves in AppData so that Program Files stays read-only friendly.

## Functional requirements
- FR-DATA-004, FR-SAVE-001, FR-SAVE-002

## Exclusions
- DOS CONFIG byte compatibility

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

## Exclusions
- CRT shader required

## Acceptance criteria
- [ ] Fullscreen toggle
- [ ] Windowed restore
- [ ] Aspect intent preserved

## Technical notes
- Epic: E8 — Milestone: M8
EOF
)" "type:story,area:video,priority:p1" "M8 — Win11 productization"

echo "== Project board =="
# Create organization/user project v2
OWNER="${REPO%/*}"
PROJECT_TITLE="Wolf3D Win64 Port"
# Try create project (user owner)
PROJ_JSON=$(gh project create --owner "$OWNER" --title "$PROJECT_TITLE" --format json 2>/dev/null || true)
if [ -z "$PROJ_JSON" ]; then
  echo "WARNING: could not create project via gh project create; trying list existing"
  gh project list --owner "$OWNER" --format json || true
else
  echo "$PROJ_JSON"
  PROJ_NUM=$(echo "$PROJ_JSON" | python -c "import sys,json; print(json.load(sys.stdin).get('number',''))")
  PROJ_ID=$(echo "$PROJ_JSON" | python -c "import sys,json; print(json.load(sys.stdin).get('id',''))")
  echo "Project number=$PROJ_NUM id=$PROJ_ID"
  # Link project to repo
  gh project link "$PROJ_NUM" --owner "$OWNER" --repo "$REPO" 2>/dev/null || true
  # Add all open issues to project
  gh issue list --repo "$REPO" --state open --limit 100 --json number,url --jq '.[]|.url' | while read -r url; do
    gh project item-add "$PROJ_NUM" --owner "$OWNER" --url "$url" 2>/dev/null || true
    echo "added $url"
  done
fi

echo "DONE bootstrap for $REPO"
