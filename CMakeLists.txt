cmake_minimum_required(VERSION 3.20)
project(wolf3d-win64 VERSION 0.8.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

option(WOLF_FETCH_SDL2 "Download SDL2 via CMake FetchContent" ON)

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
)

set(WOLF_GAME_SOURCES
  src/game/wl_menu_simple.c
  src/game/wl_play.c
  src/game/wl_draw.c
  src/game/wl_game.c
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
