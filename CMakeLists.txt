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

  # Needs a user-supplied pack; skipped when WOLF3D_DATA is not set at configure time.
  if(DEFINED ENV{WOLF3D_DATA})
    add_test(NAME smoke_only COMMAND wolf3d --data "$ENV{WOLF3D_DATA}" --smoke-only)
    set_tests_properties(smoke_only PROPERTIES
      ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
  endif()
endif()
