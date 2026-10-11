# Cold start (go/no-go gate): with an empty %APPDATA% the game makes its own
# folder, runs and writes its config there.
#   cmake -DEXE=wolf3d.exe -DDATA=<pack> -DAPPDATA_DIR=<scratch dir> -P cold_start.cmake
file(REMOVE_RECURSE "${APPDATA_DIR}")
file(MAKE_DIRECTORY "${APPDATA_DIR}")
execute_process(
  COMMAND "${CMAKE_COMMAND}" -E env "APPDATA=${APPDATA_DIR}" SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy
          "${EXE}" --data "${DATA}" --smoke-only
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "cold start failed (${rc}):\n${out}\n${err}")
endif()
if(NOT EXISTS "${APPDATA_DIR}/Wolf3DWin64/config.ini")
  message(FATAL_ERROR "cold start: no Wolf3DWin64/config.ini under the empty APPDATA")
endif()
message(STATUS "cold start OK: ${APPDATA_DIR}/Wolf3DWin64 made and config.ini written")
