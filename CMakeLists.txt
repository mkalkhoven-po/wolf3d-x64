cmake_minimum_required(VERSION 3.20)
project(wolf3d-win64 VERSION 0.8.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

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
  src/id/wl_data.c
  src/id/gamepal.c
  src/id/wl_sprite.c
)

set(WOLF_GAME_SOURCES
  src/game/wl_menu_simple.c
  src/game/wl_play.c
  src/game/wl_draw.c
  src/game/wl_game.c
  src/game/wl_status.c
  src/game/wl_weapon.c
)

set(WOLF_PLATFORM_SOURCES
  src/platform/win_sdl/platform_sdl.c
  src/platform/win_sdl/wolf_userdata.c
  src/platform/win_sdl/wolf_present.c
  src/platform/win_sdl/wolf_winutil.c
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
  src/orig/orig_act2_todo.c
  src/orig/orig_agent.c
  src/orig/orig_game.c
  src/orig/orig_hooks.c
  src/orig/orig_play.c
  src/orig/orig_state.c
  src/orig/orig_states.c
)

add_executable(wolf3d
  src/main.c
  ${WOLF_ID_SOURCES}
  ${WOLF_GAME_SOURCES}
  ${WOLF_PLATFORM_SOURCES}
)

target_include_directories(wolf3d PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/src/id
  ${CMAKE_CURRENT_SOURCE_DIR}/src/game
  ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/win_sdl
)

target_compile_definitions(wolf3d PRIVATE SDL_MAIN_HANDLED)
target_link_libraries(wolf3d PRIVATE SDL2::SDL2)

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
  set_target_properties(wolf3d PROPERTIES WIN32_EXECUTABLE FALSE)
endif()

if(WIN32 AND TARGET SDL2)
  add_custom_command(TARGET wolf3d POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:SDL2>
      $<TARGET_FILE_DIR:wolf3d>
    COMMENT "Copying SDL2 shared library next to wolf3d"
  )
endif()

install(TARGETS wolf3d RUNTIME DESTINATION bin)

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

  add_executable(test_orig_level
    tests/test_orig_level.c
    ${WOLF_ORIG_SOURCES}
    src/id/id_ca.c
    src/id/id_us.c
    src/id/wl_data.c
    src/platform/win_sdl/wolf_winutil.c
  )
  target_include_directories(test_orig_level PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/id
    ${CMAKE_CURRENT_SOURCE_DIR}/src/orig
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/win_sdl
  )
  if(UNIX OR MINGW)
    target_link_libraries(test_orig_level PRIVATE m)
  endif()
  wolf_apply_hardening(test_orig_level)
  add_test(NAME orig_level COMMAND test_orig_level)

  add_executable(test_orig_ai
    tests/test_orig_ai.c
    ${WOLF_ORIG_SOURCES}
    src/id/id_ca.c
    src/id/id_us.c
    src/id/wl_data.c
    src/platform/win_sdl/wolf_winutil.c
  )
  target_include_directories(test_orig_ai PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/id
    ${CMAKE_CURRENT_SOURCE_DIR}/src/orig
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/win_sdl
  )
  if(UNIX OR MINGW)
    target_link_libraries(test_orig_ai PRIVATE m)
  endif()
  wolf_apply_hardening(test_orig_ai)
  add_test(NAME orig_ai COMMAND test_orig_ai)

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

  # Needs a user-supplied pack; skipped when WOLF3D_DATA is not set at configure time.
  if(DEFINED ENV{WOLF3D_DATA})
    add_test(NAME smoke_only COMMAND wolf3d --data "$ENV{WOLF3D_DATA}" --smoke-only)
    set_tests_properties(smoke_only PROPERTIES
      ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
    add_test(NAME demo_chunks COMMAND test_demo_chunks --data "$ENV{WOLF3D_DATA}")
    add_test(NAME orig_level_maps COMMAND test_orig_level --data "$ENV{WOLF3D_DATA}")
    add_test(NAME orig_ai_maps COMMAND test_orig_ai --data "$ENV{WOLF3D_DATA}")
  endif()
  if(DEFINED ENV{WOLF3D_DATA_WL1})
    add_test(NAME smoke_only_wl1 COMMAND wolf3d --data "$ENV{WOLF3D_DATA_WL1}" --smoke-only)
    set_tests_properties(smoke_only_wl1 PROPERTIES
      ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
    add_test(NAME demo_chunks_wl1 COMMAND test_demo_chunks --data "$ENV{WOLF3D_DATA_WL1}")
    add_test(NAME orig_level_maps_wl1 COMMAND test_orig_level --data "$ENV{WOLF3D_DATA_WL1}")
    add_test(NAME orig_ai_maps_wl1 COMMAND test_orig_ai --data "$ENV{WOLF3D_DATA_WL1}")
  endif()
endif()
