# Cold start (go/no-go gate): with an empty %APPDATA% the game makes its own
# folder, runs and writes its config there. With -DMIGRATE=ON the settings
# folder from before R1 (Wolf3DWin64) is there instead: it must be moved to
# the new name with its files.
#   cmake -DEXE=wolf3d.exe -DDATA=<pack> -DAPPDATA_DIR=<scratch dir> [-DMIGRATE=ON] -P cold_start.cmake
file(REMOVE_RECURSE "${APPDATA_DIR}")
file(MAKE_DIRECTORY "${APPDATA_DIR}")
if(MIGRATE)
  file(MAKE_DIRECTORY "${APPDATA_DIR}/Wolf3DWin64")
  file(WRITE "${APPDATA_DIR}/Wolf3DWin64/savegam3.WL6" "an old save")
endif()
execute_process(
  COMMAND "${CMAKE_COMMAND}" -E env "APPDATA=${APPDATA_DIR}" SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy
          "${EXE}" --data "${DATA}" --smoke-only
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "cold start failed (${rc}):\n${out}\n${err}")
endif()
if(NOT EXISTS "${APPDATA_DIR}/Wolf3D-x64/config.ini")
  message(FATAL_ERROR "cold start: no Wolf3D-x64/config.ini under the APPDATA")
endif()
if(MIGRATE)
  if(NOT EXISTS "${APPDATA_DIR}/Wolf3D-x64/savegam3.WL6" OR EXISTS "${APPDATA_DIR}/Wolf3DWin64")
    message(FATAL_ERROR "settings migration: Wolf3DWin64 was not moved to Wolf3D-x64:\n${out}")
  endif()
  message(STATUS "settings migration OK: Wolf3DWin64 moved to Wolf3D-x64 with its files")
else()
  message(STATUS "cold start OK: ${APPDATA_DIR}/Wolf3D-x64 made and config.ini written")
endif()
