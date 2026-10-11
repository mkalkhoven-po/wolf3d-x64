cmake_minimum_required(VERSION 3.20)
project(wolf3d-x64 VERSION 0.9.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
if(UNIX AND NOT APPLE)
  add_compile_definitions(_DEFAULT_SOURCE) # POSIX calls (readlink, opendir, strcasecmp) under strict C17
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

option(WOLF_FETCH_SDL2 "Download SDL2 via CMake FetchContent" ON)
option(WOLF_SANITIZE "Build with ASan/UBSan (GCC/Clang only)" OFF)
option(WOLF_BUILD_TESTS "Build unit tests and register CTest cases" ON)

if(WOLF_FETCH_SDL2)
  include(FetchContent)
  set(SDL_SHARED ON CACHE BOOL "" FORCE)
  set(SDL_STATIC OFF CACHE BOOL "" FORCE)
  set(SDL_TEST OFF CACHE BOOL "" FORCE)
  set(SDL2_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
  FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-2.30.11
    GIT_SHALLOW    TRUE
  )
  FetchContent_MakeAvailable(SDL2)
else()
  find_package(SDL2 REQUIRED CONFIG)
endif()

set(WOLF_ID_SOURCES
  src/id/id_heads.c
  src/id/id_us.c
  src/id/id_mm.c
  src/id/id_pm.c
  src/id/id_ca.c
  src/id/id_vl.c
  src/id/id_vh.c
  src/id/id_in.c
  src/id/id_sd.c
  src/id/opl2.c
  src/id/sd_adlib.c
  src/id/wl_data.c
  src/id/gamepal.c
)

set(WOLF_PLATFORM_SOURCES
  src/platform/win_sdl/platform_sdl.c
  src/platform/win_sdl/wolf_userdata.c
  src/platform/win_sdl/wolf_present.c
  src/platform/win_sdl/wolf_winutil.c
  src/platform/win_sdl/wolf_pad.c
  src/platform/win_sdl/pad_map.c
  src/platform/win_sdl/wolf_icon.c
  src/platform/win_sdl/wolf_message.c
  src/platform/win_sdl/wolf_folder.c
)

# Faithful engine (P4, ADR-002). No SDL dependency; built next to the legacy engine.
# The core has no other dependencies; the game files use the shared id layer
# (map loading in id_ca.c, Quit in id_us.c).
set(WOLF_ORIG_CORE_SOURCES
  src/orig/wl_fixed.c
  src/orig/wl_tables.c
  src/orig/id_us_rnd.c
  src/orig/wl_tics.c
)
set(WOLF_ORIG_SOURCES
  ${WOLF_ORIG_CORE_SOURCES}
  src/orig/orig_act1.c
  src/orig/orig_act2.c
  src/orig/orig_agent.c
  src/orig/orig_draw.c
  src/orig/orig_game.c
  src/orig/orig_hooks.c
  src/orig/orig_inter.c
  src/orig/orig_loop.c
  src/orig/orig_menu.c
  src/orig/orig_play.c
  src/orig/orig_save.c
  src/orig/orig_scale.c
  src/orig/orig_state.c
  src/orig/orig_states.c
  src/orig/orig_text.c
  src/orig/orig_vh.c
)

add_executable(wolf3d
  src/main.c
  ${WOLF_ID_SOURCES}
  ${WOLF_PLATFORM_SOURCES}
  ${WOLF_ORIG_SOURCES}
  src/platform/win_sdl/orig_bridge.c
  src/platform/win_sdl/orig_runner.c
)

target_include_directories(wolf3d PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/src/id
  ${CMAKE_CURRENT_SOURCE_DIR}/src/orig
  ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/win_sdl
)

target_compile_definitions(wolf3d PRIVATE SDL_MAIN_HANDLED)

# The exe's icon and version information (Windows); the icon is drawn by scripts/make_icon.py.
if(WIN32)
  enable_language(RC)
  set(WOLF_RES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/res)
  configure_file(res/wolf3d-x64.rc.in ${CMAKE_CURRENT_BINARY_DIR}/wolf3d-x64.rc @ONLY)
  target_sources(wolf3d PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/wolf3d-x64.rc)
endif()
target_link_libraries(wolf3d PRIVATE SDL2::SDL2)
if(WIN32)
  target_link_libraries(wolf3d PRIVATE ole32 uuid) # the folder dialog (IFileOpenDialog)
endif()

# Applies to project targets only, never to SDL2.
function(wolf_apply_hardening target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    if(WOLF_SANITIZE)
      target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
      target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endif()
  endif()
endfunction()

wolf_apply_hardening(wolf3d)

if(UNIX OR MINGW)
  target_link_libraries(wolf3d PRIVATE m)
endif()

if(WIN32)
  # a windowed program: no console window when double-clicked (output: Wolf_SetupOutput)
  set_target_properties(wolf3d PROPERTIES WIN32_EXECUTABLE TRUE)
  if(MSVC)
    target_link_options(wolf3d PRIVATE /ENTRY:mainCRTStartup) # main(), not WinMain
  endif()
endif()

if(WIN32 AND TARGET SDL2)
  add_custom_command(TARGET wolf3d POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:SDL2>
      $<TARGET_FILE_DIR:wolf3d>
    COMMENT "Copying SDL2 shared library next to wolf3d"
  )
endif()

# The release package (cpack, after a build): the game, SDL2.dll on Windows,
# the papers and an empty data folder. Never any game data.
install(TARGETS wolf3d RUNTIME DESTINATION .)
if(WIN32 AND TARGET SDL2)
  install(FILES $<TARGET_FILE:SDL2> DESTINATION .)
endif()
configure_file(packaging/README.txt.in ${CMAKE_CURRENT_BINARY_DIR}/README.txt @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/README.txt DESTINATION .)
install(FILES LICENSE DESTINATION . RENAME LICENSE.txt)
install(FILES THIRD_PARTY_NOTICES.md DESTINATION . RENAME THIRD_PARTY_NOTICES.txt)
install(FILES packaging/data-README.txt DESTINATION data RENAME README.txt)

if(WIN32)
  set(CPACK_GENERATOR ZIP)
  set(WOLF_PACKAGE_OS windows)
else()
  set(CPACK_GENERATOR TGZ)
  set(WOLF_PACKAGE_OS linux)
endif()
set(CPACK_PACKAGE_NAME wolf3d-x64)
set(CPACK_PACKAGE_VENDOR "Wolf3D x64")
set(CPACK_PACKAGE_FILE_NAME "wolf3d-x64-${PROJECT_VERSION}-${WOLF_PACKAGE_OS}-x64")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY ON)
set(CPACK_STRIP_FILES ON)
include(CPack)

if(WOLF_BUILD_TESTS)
  enable_testing()

  add_executable(test_decoders
    tests/test_decoders.c
    src/id/id_ca.c
    src/id/id_us.c
    src/id/wl_data.c
    src/platform/win_sdl/wolf_winutil.c
  )
  target_include_directories(test_decoders PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/id
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/win_sdl
  )
  if(UNIX OR MINGW)
    target_link_libraries(test_decoders PRIVATE m)
  endif()
  wolf_apply_hardening(test_decoders)
  add_test(NAME decoders COMMAND test_decoders)

  # finding the game files whatever the case of their names (Linux file systems)
  add_executable(test_data_path tests/test_data_path.c src/id/wl_data.c src/platform/win_sdl/wolf_winutil.c)
  target_include_directories(test_data_path PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/id
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/win_sdl
  )
  wolf_apply_hardening(test_data_path)
  add_test(NAME data_path COMMAND test_data_path)

  add_executable(test_orig_core tests/test_orig_core.c ${WOLF_ORIG_CORE_SOURCES})
  target_include_directories(test_orig_core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/id
    ${CMAKE_CURRENT_SOURCE_DIR}/src/orig
  )
  if(UNIX OR MINGW)
    target_link_libraries(test_orig_core PRIVATE m)
  endif()
  wolf_apply_hardening(test_orig_core)
  add_test(NAME orig_core COMMAND test_orig_core)

  # Faithful-engine tests: the engine plus the shared id layer for map loading.
  # Each takes an optional --data DIR to run over a real pack.
  set(WOLF_ORIG_TESTS orig_level orig_ai orig_player orig_draw orig_walls orig_frame orig_loop orig_fidelity orig_ui orig_menu)
  foreach(name IN LISTS WOLF_ORIG_TESTS)
    add_executable(test_${name}
      tests/test_${name}.c
      ${WOLF_ORIG_SOURCES}
      src/id/id_ca.c
      src/id/gamepal.c
      src/id/id_pm.c
      src/id/id_us.c
      src/id/wl_data.c
      src/platform/win_sdl/wolf_winutil.c
    )
    target_include_directories(test_${name} PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/src
      ${CMAKE_CURRENT_SOURCE_DIR}/src/id
      ${CMAKE_CURRENT_SOURCE_DIR}/src/orig
      ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/win_sdl
    )
    if(UNIX OR MINGW)
      target_link_libraries(test_${name} PRIVATE m)
    endif()
    wolf_apply_hardening(test_${name})
    add_test(NAME ${name} COMMAND test_${name})
  endforeach()

  # The OPL2 emulator and the AdLib side of the sound manager (no SDL).
  add_executable(test_opl2 tests/test_opl2.c src/id/opl2.c)
  target_include_directories(test_opl2 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/id)
  add_executable(test_sd_adlib
    tests/test_sd_adlib.c
    src/id/sd_adlib.c
    src/id/opl2.c
    src/id/wl_data.c
    src/platform/win_sdl/wolf_winutil.c
  )
  target_include_directories(test_sd_adlib PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/id
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/win_sdl
  )
  foreach(name IN ITEMS test_opl2 test_sd_adlib)
    if(UNIX OR MINGW)
      target_link_libraries(${name} PRIVATE m)
    endif()
    wolf_apply_hardening(${name})
  endforeach()
  # The legacy engine is gone (P4/S6): asking for it fails before any data is read.
  add_test(NAME engine_legacy_refused COMMAND wolf3d --engine=legacy)
  set_tests_properties(engine_legacy_refused PROPERTIES WILL_FAIL TRUE)
  # FR-DATA-003: a --data folder without a pack fails with an error (never falls back to last_data)
  add_test(NAME bad_data COMMAND wolf3d --data ${CMAKE_CURRENT_BINARY_DIR}/no_such_pack --smoke-only)
  set_tests_properties(bad_data PROPERTIES WILL_FAIL TRUE)
  add_test(NAME opl2 COMMAND test_opl2)
  add_executable(test_pad_map tests/test_pad_map.c src/platform/win_sdl/pad_map.c)
  target_include_directories(test_pad_map PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/win_sdl)
  if(UNIX OR MINGW)
    target_link_libraries(test_pad_map PRIVATE m)
  endif()
  wolf_apply_hardening(test_pad_map)
  add_test(NAME pad_map COMMAND test_pad_map)
  add_test(NAME sd_adlib COMMAND test_sd_adlib)

  add_executable(test_demo_chunks
    tests/test_demo_chunks.c
    src/id/id_ca.c
    src/id/id_us.c
    src/id/wl_data.c
    src/platform/win_sdl/wolf_winutil.c
  )
  target_include_directories(test_demo_chunks PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/id
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/win_sdl
  )
  wolf_apply_hardening(test_demo_chunks)

  # The faithful engine in the real executable: one demo and 300 frames of a game,
  # unclocked and without a visible window. Demo fidelity is checked by orig_loop_maps.
  function(wolf_add_orig_runner_tests suffix data)
    add_test(NAME orig_runner_demo${suffix}
      COMMAND wolf3d --data "${data}" --engine=orig --demo 0 --fast)
    add_test(NAME orig_runner_game${suffix}
      COMMAND wolf3d --data "${data}" --engine=orig --fast --frames 300
              --screenshot 300 "${CMAKE_CURRENT_BINARY_DIR}/orig_runner_game${suffix}.bmp")
    set_tests_properties(orig_runner_demo${suffix} orig_runner_game${suffix} PROPERTIES
      ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
  endfunction()

  # Needs a user-supplied pack; skipped when WOLF3D_DATA is not set at configure time.
  if(DEFINED ENV{WOLF3D_DATA})
    wolf_add_orig_runner_tests("" "$ENV{WOLF3D_DATA}")
    add_test(NAME smoke_only COMMAND wolf3d --data "$ENV{WOLF3D_DATA}" --smoke-only)
    if(WIN32)
      add_test(NAME cold_start COMMAND ${CMAKE_COMMAND} -DEXE=$<TARGET_FILE:wolf3d>
               "-DDATA=$ENV{WOLF3D_DATA}" -DAPPDATA_DIR=${CMAKE_CURRENT_BINARY_DIR}/cold_appdata
               -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/cold_start.cmake)
      add_test(NAME appdata_migration COMMAND ${CMAKE_COMMAND} -DEXE=$<TARGET_FILE:wolf3d>
               "-DDATA=$ENV{WOLF3D_DATA}" -DAPPDATA_DIR=${CMAKE_CURRENT_BINARY_DIR}/migrate_appdata
               -DMIGRATE=ON -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/cold_start.cmake)
    endif()
    set_tests_properties(smoke_only PROPERTIES
      ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
    add_test(NAME demo_chunks COMMAND test_demo_chunks --data "$ENV{WOLF3D_DATA}")
    add_test(NAME sd_adlib_data COMMAND test_sd_adlib --data "$ENV{WOLF3D_DATA}")
    foreach(name IN LISTS WOLF_ORIG_TESTS)
      add_test(NAME ${name}_maps COMMAND test_${name} --data "$ENV{WOLF3D_DATA}")
    endforeach()
  endif()
  if(DEFINED ENV{WOLF3D_DATA_WL1})
    wolf_add_orig_runner_tests("_wl1" "$ENV{WOLF3D_DATA_WL1}")
    add_test(NAME smoke_only_wl1 COMMAND wolf3d --data "$ENV{WOLF3D_DATA_WL1}" --smoke-only)
    set_tests_properties(smoke_only_wl1 PROPERTIES
      ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
    add_test(NAME demo_chunks_wl1 COMMAND test_demo_chunks --data "$ENV{WOLF3D_DATA_WL1}")
    add_test(NAME sd_adlib_data_wl1 COMMAND test_sd_adlib --data "$ENV{WOLF3D_DATA_WL1}")
    foreach(name IN LISTS WOLF_ORIG_TESTS)
      add_test(NAME ${name}_maps_wl1 COMMAND test_${name} --data "$ENV{WOLF3D_DATA_WL1}")
    endforeach()
  endif()
endif()
