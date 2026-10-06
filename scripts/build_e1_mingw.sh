#!/usr/bin/env bash
# Configure + build E1 shell with WinLibs MinGW + Ninja (developer helper).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

CMAKE="${CMAKE:-/c/Program Files/CMake/bin/cmake.exe}"
if [[ ! -x "$CMAKE" ]]; then
  CMAKE="$(command -v cmake)"
fi

# Optional: set WINLIBS_ROOT to a WinLibs mingw64 dir; otherwise gcc/ninja come from PATH.
WINLIBS_ROOT="${WINLIBS_ROOT:-}"
if [[ -n "$WINLIBS_ROOT" && -d "$WINLIBS_ROOT/bin" ]]; then
  export PATH="$WINLIBS_ROOT/bin:$PATH"
  CC_BIN="$WINLIBS_ROOT/bin/gcc.exe"
  CXX_BIN="$WINLIBS_ROOT/bin/g++.exe"
  NINJA_BIN="$WINLIBS_ROOT/bin/ninja.exe"
else
  CC_BIN="$(command -v gcc)"
  CXX_BIN="$(command -v g++)"
  NINJA_BIN="$(command -v ninja)"
fi

# CMake on Windows prefers native paths for toolchain files / make program
cygpath_w() {
  if command -v cygpath >/dev/null 2>&1; then cygpath -w "$1"; else echo "$1"; fi
}

mkdir -p build
"$CMAKE" -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_C_COMPILER="$(cygpath_w "$CC_BIN")" \
  -DCMAKE_CXX_COMPILER="$(cygpath_w "$CXX_BIN")" \
  -DCMAKE_MAKE_PROGRAM="$(cygpath_w "$NINJA_BIN")"

"$CMAKE" --build build -j "${WOLF_JOBS:-$(nproc 2>/dev/null || echo 4)}"

echo "OK: $ROOT/build/wolf3d.exe"
