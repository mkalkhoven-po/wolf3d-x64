@echo off
REM Wolf3D x64 — Windows entry for scripts\smoke_wl6.sh (Q1)
REM Prefers Git Bash when available; rushed users can call:
REM   build\wolf3d.exe --data %WOLF3D_DATA% --smoke-only

setlocal EnableExtensions
set "ROOT=%~dp0.."
pushd "%ROOT%" >nul

set "BASH="
if exist "%ProgramFiles%\Git\bin\bash.exe" set "BASH=%ProgramFiles%\Git\bin\bash.exe"
if not defined BASH if exist "%ProgramFiles(x86)%\Git\bin\bash.exe" set "BASH=%ProgramFiles(x86)%\Git\bin\bash.exe"
if not defined BASH if exist "%LOCALAPPDATA%\Programs\Git\bin\bash.exe" set "BASH=%LOCALAPPDATA%\Programs\Git\bin\bash.exe"

if defined BASH (
  "%BASH%" "%ROOT%\scripts\smoke_wl6.sh" %*
  set "EC=%ERRORLEVEL%"
  popd >nul
  exit /b %EC%
)

echo smoke_wl6.bat: Git Bash not found — falling back to bare good/bad smoke.
echo Install Git for Windows for full marker checks, or run scripts\smoke_wl6.sh from bash.

if not defined WOLF3D_DATA (
  if exist "Wolf3dgame\VSWAP.WL6" set "WOLF3D_DATA=%CD%\Wolf3dgame"
  if not defined WOLF3D_DATA if exist "Wolf3dgame\VSWAP.WL1" set "WOLF3D_DATA=%CD%\Wolf3dgame"
)
if not defined WOLF3D_DATA (
  echo FAIL: set WOLF3D_DATA or place Wolf3dgame next to repo root.
  popd >nul
  exit /b 1
)
if not exist "build\wolf3d.exe" (
  echo FAIL: build\wolf3d.exe missing — build first.
  popd >nul
  exit /b 1
)

echo Running good path: build\wolf3d.exe --data "%WOLF3D_DATA%" --smoke-only
build\wolf3d.exe --data "%WOLF3D_DATA%" --smoke-only
if errorlevel 1 (
  echo FAIL: good-path smoke failed errorlevel=%ERRORLEVEL%
  popd >nul
  exit /b 2
)

echo Running bad path...
build\wolf3d.exe --data __wolf3d_smoke_no_such_data_dir__ --smoke-only
if errorlevel 2 if not errorlevel 3 (
  echo PASS: bad-path exit 2
  echo RESULT: PASSED ^(bare bat fallback — use bash for marker asserts^)
  popd >nul
  exit /b 0
)
echo FAIL: bad-path expected exit 2, got %ERRORLEVEL%
popd >nul
exit /b 4
