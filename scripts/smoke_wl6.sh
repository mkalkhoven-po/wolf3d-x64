#!/usr/bin/env bash
# Wolf3D Win64 — local smoke harness (Q1).
# Runs good-path --smoke-only + bad-path failure check + log marker asserts.
# Never commits or requires packing commercial assets into git.
#
# Usage:
#   scripts/smoke_wl6.sh
#   scripts/smoke_wl6.sh --data D:/path/to/Wolf3dgame
#   scripts/smoke_wl6.sh --exe build/wolf3d.exe --data Wolf3dgame
#   WOLF3D_DATA=D:/games/wl6 scripts/smoke_wl6.sh
#   scripts/smoke_wl6.sh --no-build
#
# Exit: 0 all PASS; non-zero on first failure class (see docs/qa/SMOKE.md).

set -uo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

EXE_DEFAULT="build/wolf3d.exe"
EXE="$EXE_DEFAULT"
DATA="${WOLF3D_DATA:-}"
DO_BUILD=1
FAIL=0
PASS_N=0
BAD_PATH="${SMOKE_BAD_PATH:-__wolf3d_smoke_no_such_data_dir__}"

pass() { echo "PASS: $*"; PASS_N=$((PASS_N + 1)); }
fail() { echo "FAIL: $*"; FAIL=1; }

usage() {
  cat <<'EOF'
Usage: scripts/smoke_wl6.sh [options]

Options:
  --data PATH   Game data directory (else WOLF3D_DATA, else ./Wolf3dgame, else ./data)
  --exe PATH    Path to wolf3d.exe (default: build/wolf3d.exe)
  --no-build    Do not run cmake --build
  --bad-path P  Override path used for negative test
  -h, --help    This help

Exit codes:
  0  all checks passed
  1  harness/setup failure (missing exe/data after resolution)
  2  good-path smoke process failed (nonzero exit)
  3  good-path missing required log markers
  4  bad-path did not fail as expected (wanted exit 2)
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --data)
      DATA="${2:-}"; shift 2 ;;
    --exe)
      EXE="${2:-}"; shift 2 ;;
    --no-build)
      DO_BUILD=0; shift ;;
    --bad-path)
      BAD_PATH="${2:-}"; shift 2 ;;
    -h|--help)
      usage; exit 0 ;;
    *)
      echo "Unknown arg: $1" >&2
      usage >&2
      exit 1 ;;
  esac
done

# Resolve data dir
if [[ -z "$DATA" ]]; then
  if [[ -d "$ROOT/Wolf3dgame" ]]; then
    DATA="$ROOT/Wolf3dgame"
  elif [[ -d "$ROOT/data" ]] && ls "$ROOT/data"/VSWAP.* >/dev/null 2>&1; then
    DATA="$ROOT/data"
  fi
fi

if [[ -z "$DATA" ]]; then
  fail "No data directory. Set --data or WOLF3D_DATA (e.g. Wolf3dgame)."
  echo "RESULT: FAILED (setup)"
  exit 1
fi

if [[ ! -d "$DATA" ]]; then
  fail "Data path is not a directory: $DATA"
  echo "RESULT: FAILED (setup)"
  exit 1
fi
pass "data dir: $DATA"

# Optional rebuild
if [[ "$DO_BUILD" -eq 1 ]]; then
  if command -v cmake >/dev/null 2>&1 && [[ -f "$ROOT/build/build.ninja" || -f "$ROOT/build/CMakeCache.txt" ]]; then
    if cmake --build "$ROOT/build" -j 8 >/tmp/wolf3d-smoke-build.log 2>&1; then
      pass "cmake --build"
    else
      fail "cmake --build failed (see /tmp/wolf3d-smoke-build.log)"
      tail -40 /tmp/wolf3d-smoke-build.log 2>/dev/null || true
      echo "RESULT: FAILED (build)"
      exit 1
    fi
  else
    echo "INFO: skip auto-build (no cmake build tree); use existing exe"
  fi
fi

if [[ ! -f "$EXE" ]]; then
  # try ROOT-relative
  if [[ -f "$ROOT/$EXE" ]]; then
    EXE="$ROOT/$EXE"
  fi
fi
if [[ ! -f "$EXE" ]]; then
  fail "exe not found: $EXE (build first or pass --exe)"
  echo "RESULT: FAILED (setup)"
  exit 1
fi
pass "exe: $EXE"

# PE32+ check when `file` exists
if command -v file >/dev/null 2>&1; then
  FT="$(file "$EXE" 2>/dev/null || true)"
  if echo "$FT" | grep -q 'PE32+'; then
    pass "exe looks PE32+"
  else
    echo "INFO: file(1) did not report PE32+ ($FT) — continuing"
  fi
fi

export SDL_AUDIODRIVER="${SDL_AUDIODRIVER:-directsound}"

GOOD_LOG="${TMPDIR:-/tmp}/wolf3d-smoke-good-$$.log"
BAD_LOG="${TMPDIR:-/tmp}/wolf3d-smoke-bad-$$.log"
cleanup() { rm -f "$GOOD_LOG" "$BAD_LOG" 2>/dev/null || true; }
trap cleanup EXIT

set +e
"$EXE" --data "$DATA" --smoke-only >"$GOOD_LOG" 2>&1
GOOD_EC=$?
set -e

echo "INFO: good-path exit=$GOOD_EC (log $(wc -c <"$GOOD_LOG" 2>/dev/null || echo 0) bytes)"
if [[ "$GOOD_EC" -eq 0 ]]; then
  pass "good-path exit 0"
else
  fail "good-path exit=$GOOD_EC (want 0)"
  echo "----- good-path log (tail) -----"
  tail -50 "$GOOD_LOG" || true
  echo "RESULT: FAILED (good-path exit)"
  exit 2
fi

# Required markers for Q1 #47 — must all appear in good log.
# Keep list stable; bump docs/qa/SMOKE.md when changing.
REQUIRED_MARKERS=(
  "wolf3d-win64 smoke OK"
  "TITLEPIC loaded"
  "VSWAP chunks="
  "VGAGRAPH open"
  "Audio digi:"
  "Audio smoke:"
  "silent fallback play: OK"
  "UserData smoke:"
  "config roundtrip OK"
  "save/load OK"
  "Engine smoke: demo 0"
  "Engine smoke: save v2 roundtrip OK"
  "AdLib effect:"
  "digi extra screams="
  "Present smoke:"
  "logical=320x200"
  "windowed_client="
)

MISSING=0
for m in "${REQUIRED_MARKERS[@]}"; do
  if grep -F -q -- "$m" "$GOOD_LOG"; then
    pass "marker: $m"
  else
    fail "missing marker: $m"
    MISSING=1
  fi
done

if [[ "$MISSING" -ne 0 ]]; then
  echo "----- good-path log (full head) -----"
  head -80 "$GOOD_LOG" || true
  echo "RESULT: FAILED (markers)"
  exit 3
fi

# Bad path: must be exit 2 and must NOT print smoke OK
set +e
"$EXE" --data "$BAD_PATH" --smoke-only >"$BAD_LOG" 2>&1
BAD_EC=$?
set -e
echo "INFO: bad-path exit=$BAD_EC path=$BAD_PATH"

if [[ "$BAD_EC" -eq 2 ]]; then
  pass "bad-path exit 2"
else
  fail "bad-path exit=$BAD_EC (want 2)"
  tail -20 "$BAD_LOG" || true
  echo "RESULT: FAILED (bad-path exit)"
  exit 4
fi

if grep -F -q "wolf3d-win64 smoke OK" "$BAD_LOG"; then
  fail "bad-path log unexpectedly contains smoke OK"
  echo "RESULT: FAILED (bad-path markers)"
  exit 4
else
  pass "bad-path has no smoke OK"
fi

if [[ "$FAIL" -ne 0 ]]; then
  echo "RESULT: FAILED ($PASS_N passes before fail tally)"
  exit 1
fi

echo "RESULT: PASSED ($PASS_N checks)"
exit 0
