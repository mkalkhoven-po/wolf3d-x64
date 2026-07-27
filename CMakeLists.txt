cmake_minimum_required(VERSION 3.20)
project(wolf3d-win64 VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Nice defaults on multi-config single-config generators
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

add_executable(wolf3d
  src/main.c
  src/platform/win_sdl/platform_sdl.c
)

target_include_directories(wolf3d PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/win_sdl
)

# Console-subsystem bring-up: keep our own main(); skip WinMain/SDL2main.
# (Link order of SDL2main before SDL2 is flaky with some MinGW+FetchContent combos.)
target_compile_definitions(wolf3d PRIVATE SDL_MAIN_HANDLED)
target_link_libraries(wolf3d PRIVATE SDL2::SDL2)

if(WIN32)
  # Console subsystem for early bring-up (see quit codes / printf);
  # switch to WIN32_EXECUTABLE later in productization if desired.
  set_target_properties(wolf3d PROPERTIES WIN32_EXECUTABLE FALSE)
endif()

# Copy SDL2.dll next to the exe for easy double-click runs (shared build).
if(WIN32 AND TARGET SDL2)
  add_custom_command(TARGET wolf3d POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:SDL2>
      $<TARGET_FILE_DIR:wolf3d>
    COMMENT "Copying SDL2 shared library next to wolf3d"
  )
endif()

install(TARGETS wolf3d RUNTIME DESTINATION bin)
