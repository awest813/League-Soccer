cmake_minimum_required(VERSION 3.14)

set(PACKAGE GameplayFootball)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 4)
set(VERSION_PATCH 0)
set(VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMakeModules")

project(GameplayFootball VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH} LANGUAGES C CXX)
include(CTest)
option(GAMEPLAYFOOTBALL_BUILD_GAME "Build the SDL/OpenGL game executable and runtime libraries." ON)
option(GAMEPLAYFOOTBALL_BUILD_DOCS "Generate API documentation with Doxygen (requires doxygen)." OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDE/tooling support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(UNIX)
   # The legacy engine keeps compatibility parameters in virtual interfaces
   # and dormant diagnostic calculations alongside active code. Keep the
   # broad GCC warning sets while excluding only those two known categories.
   set(GAMEPLAYFOOTBALL_LEGACY_UNUSED_WARNINGS
       "-Wno-unused-parameter -Wno-unused-variable")
   set(CMAKE_CXX_FLAGS_DEBUG
       "${CMAKE_CXX_FLAGS_DEBUG} -g -Wall -Wextra ${GAMEPLAYFOOTBALL_LEGACY_UNUSED_WARNINGS}")
   set(CMAKE_CXX_FLAGS_RELEASE
       "${CMAKE_CXX_FLAGS_RELEASE} -Wall -O3 ${GAMEPLAYFOOTBALL_LEGACY_UNUSED_WARNINGS}")
endif()

if(MSVC)
  # SDL2_gfx and the Windows CRT both provide the standard M_PI compatibility
  # macro; their include order can produce C4005 even though the values match.
  # The original engine intentionally mixes float-based world/UI coordinates
  # with integer pixels, frame indices, and container sizes. MSVC reports those
  # established boundaries as C4244/C4267/C4305 throughout the legacy code.
  # Keep the useful warning set enabled while excluding these known, noisy
  # compatibility and conversion categories from Windows builds.
  add_compile_options(/wd4005 /wd4244 /wd4267 /wd4305)
endif()

function(gf_find_boost)
  find_package(Boost CONFIG QUIET)
  if(Boost_FOUND)
    return()
  endif()

  # Modern vcpkg baselines don't always generate an aggregate
  # BoostConfig.cmake merely from installing individual boost-* ports (this
  # project only needs Boost.Signals2, which pulls in Boost.SmartPtr /
  # intrusive_ptr as a dependency). Their toolchain instead specially
  # intercepts find_package(Boost ... COMPONENTS ...) calls (see the vcpkg
  # port's own documented usage: 'find_package(Boost REQUIRED COMPONENTS
  # signals2)') and resolves each requested component against its own
  # vcpkg-generated config package under the hood.
  find_package(Boost QUIET COMPONENTS signals2)
  if(TARGET Boost::signals2)
    if(NOT TARGET Boost::headers)
      add_library(Boost::headers ALIAS Boost::signals2)
    endif()
    set(Boost_FOUND TRUE PARENT_SCOPE)
    return()
  endif()

  # Other vcpkg baselines instead expose a standalone lower-case CMake
  # config package per component (e.g. boost_signals2-config.cmake).
  find_package(boost_signals2 CONFIG QUIET)
  foreach(candidate IN ITEMS Boost::signals2 boost_signals2::boost_signals2)
    if(TARGET ${candidate})
      if(NOT TARGET Boost::headers)
        add_library(Boost::headers ALIAS ${candidate})
      endif()
      set(Boost_FOUND TRUE PARENT_SCOPE)
      return()
    endif()
  endforeach()

  if(POLICY CMP0167)
    cmake_policy(SET CMP0167 OLD)
  endif()
  find_package(Boost QUIET)
  if(Boost_FOUND)
    return()
  endif()

  # Last resort: boost::signals2 and boost::intrusive_ptr are both
  # header-only, so if every find_package(Boost ...) attempt above failed
  # (observed with some vcpkg baselines: CMake's own bundled classic
  # FindBoost.cmake module doesn't recognize 'signals2' as a valid
  # COMPONENTS entry - it has no header-only exception listed for it - and
  # gives up even though the headers are right there on disk), just locate
  # the headers directly. vcpkg's toolchain file adds the triplet's install
  # prefix to CMAKE_PREFIX_PATH, but pass it as an explicit HINTS too (using
  # the VCPKG_INSTALLED_DIR/VCPKG_TARGET_TRIPLET variables the toolchain
  # itself defines) so this doesn't depend on that propagation working.
  message(STATUS "gf_find_boost: fallback search - VCPKG_INSTALLED_DIR=${VCPKG_INSTALLED_DIR} "
                 "VCPKG_TARGET_TRIPLET=${VCPKG_TARGET_TRIPLET} CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}")
  find_path(GF_BOOST_HEADER_DIR
    NAMES boost/signals2.hpp boost/intrusive_ptr.hpp
    HINTS
      "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include"
      "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include"
  )
  message(STATUS "gf_find_boost: header-only fallback search result: ${GF_BOOST_HEADER_DIR}")
  if(GF_BOOST_HEADER_DIR)
    if(NOT TARGET Boost::headers)
      add_library(Boost::headers INTERFACE IMPORTED GLOBAL)
      set_target_properties(Boost::headers PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${GF_BOOST_HEADER_DIR}")
    endif()
    set(Boost_FOUND TRUE PARENT_SCOPE)
    return()
  endif()

  message(FATAL_ERROR
    "Could not find Boost. Install Boost development packages or configure with a vcpkg toolchain.")
endfunction()

function(gf_pick_target out_var)
  foreach(candidate IN LISTS ARGN)
    if(TARGET ${candidate})
      set(${out_var} ${candidate} PARENT_SCOPE)
      return()
    endif()
  endforeach()
  set(${out_var} "" PARENT_SCOPE)
endfunction()

# Prefer each SDL2 package's modern CMake config (as shipped by vcpkg and
# newer distro packages, which provides proper imported targets), but fall
# back to the classic FindXxx.cmake modules in CMakeModules/ (which only set
# plain _LIBRARIES/_INCLUDE_DIRS variables) for older distro packages - e.g.
# Ubuntu 22.04's libsdl2-image-dev/libsdl2-ttf-dev - that don't ship a CMake
# config file at all.
# This is a macro (not a function): the classic Find modules set plain
# variables (e.g. SDL2_IMAGE_LIBRARIES) with a bare set(), no PARENT_SCOPE,
# so calling find_package() from inside a function() would trap those
# variables in the function's local scope and silently drop them (they'd
# never reach the elseif(SDL2_IMAGE_LIBRARIES)/_INCLUDE_DIRS consumers
# below) - a macro executes in the caller's scope instead.
macro(gf_find_sdl2_package name)
  find_package(${name} CONFIG QUIET)
  if(NOT ${name}_FOUND)
    find_package(${name} REQUIRED)
  endif()
endmacro()

# Find required libraries
if(GAMEPLAYFOOTBALL_BUILD_GAME)
  gf_find_boost()
  find_package(OpenGL REQUIRED)
  gf_find_sdl2_package(SDL2)
  gf_find_sdl2_package(SDL2_image)
  gf_find_sdl2_package(SDL2_ttf)
  gf_find_sdl2_package(SDL2_gfx)
  find_package(OpenAL QUIET)
  find_package(SQLite3 REQUIRED)
else()
  gf_find_boost()
endif()

set(GF_BOOST_INCLUDE_DIRS ${Boost_INCLUDE_DIRS})
if(NOT GF_BOOST_INCLUDE_DIRS AND Boost_INCLUDE_DIR)
  set(GF_BOOST_INCLUDE_DIRS ${Boost_INCLUDE_DIR})
endif()
if(NOT GF_BOOST_INCLUDE_DIRS)
  if(TARGET Boost::headers)
    get_target_property(GF_BOOST_INCLUDE_DIRS Boost::headers INTERFACE_INCLUDE_DIRECTORIES)
  elseif(TARGET Boost::boost)
    get_target_property(GF_BOOST_INCLUDE_DIRS Boost::boost INTERFACE_INCLUDE_DIRECTORIES)
  endif()
endif()

if(GAMEPLAYFOOTBALL_BUILD_GAME)
  gf_pick_target(GF_SDL2_TARGET SDL2::SDL2 SDL2::SDL2-static)
  gf_pick_target(GF_SDL2MAIN_TARGET SDL2::SDL2main)
  gf_pick_target(GF_SDL2_IMAGE_TARGET SDL2_image::SDL2_image SDL2_image::SDL2_image-static)
  gf_pick_target(GF_SDL2_TTF_TARGET SDL2_ttf::SDL2_ttf SDL2_ttf::SDL2_ttf-static)
  gf_pick_target(GF_SDL2_GFX_TARGET SDL2_gfx::SDL2_gfx SDL2_gfx::SDL2_gfx-static)
  gf_pick_target(GF_OPENGL_TARGET OpenGL::GL)
  gf_pick_target(GF_OPENAL_TARGET OpenAL::OpenAL)
  # Config packages (vcpkg) often omit the classic *_INCLUDE_DIRS variables that
  # OBJECT libraries still consume via gf_target_includes(). Pull them from the
  # imported target when needed.
  if(GF_SDL2_GFX_TARGET AND NOT SDL2_GFX_INCLUDE_DIRS)
    get_target_property(_gf_sdl2_gfx_inc ${GF_SDL2_GFX_TARGET} INTERFACE_INCLUDE_DIRECTORIES)
    if(_gf_sdl2_gfx_inc)
      set(SDL2_GFX_INCLUDE_DIRS ${_gf_sdl2_gfx_inc})
    endif()
    unset(_gf_sdl2_gfx_inc)
  endif()
endif()

# Source files
include(sources.cmake)

# Helper function: apply common include dirs to an OBJECT library target
function(gf_target_includes tgt)
  target_include_directories(${tgt} PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${OPENGL_INCLUDE_DIR}
    ${SDL2_INCLUDE_DIR}
    ${SDL2_IMAGE_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS}
    ${SDL2_GFX_INCLUDE_DIRS}
    ${GF_BOOST_INCLUDE_DIRS}
    ${SQLite3_INCLUDE_DIRS}
  )
  if(OpenAL_FOUND)
    target_include_directories(${tgt} PRIVATE ${OPENAL_INCLUDE_DIR})
  endif()
endfunction()

add_library(gf_mathlib STATIC
  src/base/log.cpp
  ${BASE_MATH_SOURCES}
  ${BASE_MATH_HEADERS})
target_include_directories(gf_mathlib PUBLIC
  ${PROJECT_SOURCE_DIR}/src
  ${GF_BOOST_INCLUDE_DIRS}
)

add_library(gf_ballphysicslib STATIC
  src/onthepitch/ballphysics.cpp
  src/onthepitch/ballphysics.hpp)
target_link_libraries(gf_ballphysicslib PUBLIC gf_mathlib)
target_include_directories(gf_ballphysicslib PUBLIC
  ${PROJECT_SOURCE_DIR}/src
)

if(GAMEPLAYFOOTBALL_BUILD_GAME)
  if(OpenAL_FOUND)
    add_compile_definitions(GF_USE_OPENAL=1)
  else()
    message(STATUS "OpenAL not found; building with NullAudioRenderer fallback")
  endif()

  # Compile it as multiple OBJECT libraries (note: not compiling physics,
  # as not used by gameplayfootball)

  add_library(baselib OBJECT ${BASE_SOURCES} ${BASE_HEADERS}
          ${BASE_GEOMETRY_HEADERS} ${BASE_MATH_HEADERS})
  gf_target_includes(baselib)

  add_library(systemscommonlib OBJECT ${SYSTEMS_COMMON_SOURCES}
          ${SYSTEMS_COMMON_HEADERS})
  gf_target_includes(systemscommonlib)

  add_library(systemsgraphicslib OBJECT ${SYSTEMS_GRAPHICS_SOURCES}
          ${SYSTEMS_GRAPHICS_HEADERS} ${SYSTEMS_GRAPHICS_OBJECTS_HEADERS}
          ${SYSTEMS_GRAPHICS_RESOURCES_HEADERS} ${SYSTEMS_GRAPHICS_RENDERING_HEADERS})
  gf_target_includes(systemsgraphicslib)

  add_library(systemsaudiolib OBJECT ${SYSTEMS_AUDIO_SOURCES}
          ${SYSTEMS_AUDIO_HEADERS} ${SYSTEMS_AUDIO_OBJECTS_HEADERS}
          ${SYSTEMS_AUDIO_RESOURCES_HEADERS} ${SYSTEMS_AUDIO_RENDERING_HEADERS})
  gf_target_includes(systemsaudiolib)

  add_library(loaderslib OBJECT ${LOADERS_SOURCES} ${LOADERS_HEADERS})
  gf_target_includes(loaderslib)

  add_library(typeslib OBJECT ${TYPES_SOURCES} ${TYPES_HEADERS})
  gf_target_includes(typeslib)

  add_library(frameworklib OBJECT ${FRAMEWORK_SOURCES} ${FRAMEWORK_HEADERS})
  gf_target_includes(frameworklib)

  add_library(scenelib OBJECT ${SCENE_SOURCES} ${SCENE_HEADERS}
          ${SCENE2D_HEADERS} ${SCENE3D_HEADERS} ${SCENE_OBJECTS_HEADERS}
          ${SCENE_RESOURCES_HEADERS})
  gf_target_includes(scenelib)

  add_library(managerslib OBJECT ${MANAGERS_SOURCES} ${MANAGERS_HEADERS})
  gf_target_includes(managerslib)

  add_library(utilslib OBJECT ${UTILS_SOURCES} ${UTILS_HEADERS}
          ${UTILS_EXT_HEADERS})
  gf_target_includes(utilslib)

  add_library(gui2lib OBJECT ${UTILS_GUI2_SOURCES} ${UTILS_GUI2_HEADERS}
          ${UTILS_GUI2_WIDGETS_HEADERS})
  gf_target_includes(gui2lib)

  set(OWN_LIBRARIES $<TARGET_OBJECTS:baselib> $<TARGET_OBJECTS:systemscommonlib>
          $<TARGET_OBJECTS:systemsgraphicslib>
          $<TARGET_OBJECTS:systemsaudiolib> $<TARGET_OBJECTS:loaderslib>
          $<TARGET_OBJECTS:typeslib> $<TARGET_OBJECTS:frameworklib>
          $<TARGET_OBJECTS:scenelib> $<TARGET_OBJECTS:managerslib>
          $<TARGET_OBJECTS:utilslib> $<TARGET_OBJECTS:gui2lib>)

  # Join all created OBJECT libraries into the single blunted2 library.
  add_library(blunted2 ${BLUNTED_CORE_SOURCES} ${BLUNTED_CORE_HEADERS} ${OWN_LIBRARIES})
  gf_target_includes(blunted2)

  add_library(leaguelib ${LEAGUE_SOURCES} ${LEAGUE_HEADERS})
  gf_target_includes(leaguelib)

  add_library(gamelib ${GAME_SOURCES} ${GAME_HEADERS})
  gf_target_includes(gamelib)
  target_link_libraries(gamelib PRIVATE gf_ballphysicslib)

  add_library(hidlib ${HID_SOURCES} ${HID_HEADERS})
  gf_target_includes(hidlib)

  add_library(menulib ${MENU_SOURCES} ${MENU_HEADERS})
  gf_target_includes(menulib)
  target_link_libraries(menulib PRIVATE SQLite::SQLite3)

  add_library(datalib ${DATA_SOURCES} ${DATA_HEADERS})
  gf_target_includes(datalib)

  set(LIBRARIES gamelib hidlib menulib datalib leaguelib blunted2
     SQLite::SQLite3)
  if(GF_SDL2_GFX_TARGET)
    list(APPEND LIBRARIES ${GF_SDL2_GFX_TARGET})
  elseif(SDL2_GFX_LIBRARIES)
    list(APPEND LIBRARIES ${SDL2_GFX_LIBRARIES})
  endif()
  if(GF_SDL2MAIN_TARGET AND NOT WIN32)
    list(APPEND LIBRARIES ${GF_SDL2MAIN_TARGET})
  endif()
  if(GF_SDL2_TARGET)
    list(APPEND LIBRARIES ${GF_SDL2_TARGET})
  elseif(SDL2_LIBRARIES)
    list(APPEND LIBRARIES ${SDL2_LIBRARIES})
  endif()
  if(GF_SDL2_IMAGE_TARGET)
    list(APPEND LIBRARIES ${GF_SDL2_IMAGE_TARGET})
  elseif(SDL2_IMAGE_LIBRARIES)
    list(APPEND LIBRARIES ${SDL2_IMAGE_LIBRARIES})
  endif()
  if(GF_SDL2_TTF_TARGET)
    list(APPEND LIBRARIES ${GF_SDL2_TTF_TARGET})
  elseif(SDL2_TTF_LIBRARIES)
    list(APPEND LIBRARIES ${SDL2_TTF_LIBRARIES})
  endif()
  if(GF_OPENGL_TARGET)
    list(APPEND LIBRARIES ${GF_OPENGL_TARGET})
  elseif(OPENGL_LIBRARIES)
    list(APPEND LIBRARIES ${OPENGL_LIBRARIES})
  endif()
  if(UNIX)
    list(APPEND LIBRARIES dl m)
  endif()
  if(OpenAL_FOUND)
    if(GF_OPENAL_TARGET)
      list(APPEND LIBRARIES ${GF_OPENAL_TARGET})
    elseif(OPENAL_LIBRARY)
      list(APPEND LIBRARIES ${OPENAL_LIBRARY})
    endif()
  endif()

  add_executable(gameplayfootball ${CORE_SOURCES} ${CORE_HEADERS})
  target_link_libraries(gameplayfootball PRIVATE ${LIBRARIES})
  target_include_directories(gameplayfootball PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${OPENGL_INCLUDE_DIR}
    ${SDL2_INCLUDE_DIR}
    ${SDL2_IMAGE_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS}
    ${SDL2_GFX_INCLUDE_DIRS}
    ${GF_BOOST_INCLUDE_DIRS}
    ${SQLite3_INCLUDE_DIRS}
  )
  if(OpenAL_FOUND)
    target_include_directories(gameplayfootball PRIVATE ${OPENAL_INCLUDE_DIR})
  endif()
  add_custom_command(TARGET gameplayfootball POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:gameplayfootball>/data
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${PROJECT_SOURCE_DIR}/data/football.config
      $<TARGET_FILE_DIR:gameplayfootball>/football.config
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${PROJECT_SOURCE_DIR}/data/football.config
      $<TARGET_FILE_DIR:gameplayfootball>/data/football.config
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      ${PROJECT_SOURCE_DIR}/data/databases
      $<TARGET_FILE_DIR:gameplayfootball>/databases
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      ${PROJECT_SOURCE_DIR}/data/locale
      $<TARGET_FILE_DIR:gameplayfootball>/locale
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      ${PROJECT_SOURCE_DIR}/data/media
      $<TARGET_FILE_DIR:gameplayfootball>/media
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      ${PROJECT_SOURCE_DIR}/data/locale
      $<TARGET_FILE_DIR:gameplayfootball>/data/locale
    COMMENT "Copying runtime assets for gameplayfootball"
  )
  # On Windows + modern CMake, copy imported shared-library DLLs next to the
  # executable so local runs and CI packaging find SDL/OpenAL/SQLite without
  # requiring PATH edits. Falls back to scripts/package_windows.ps1 for
  # classic Find-module builds that lack TARGET_RUNTIME_DLLS metadata.
  if(WIN32 AND CMAKE_VERSION VERSION_GREATER_EQUAL "3.21")
    add_custom_command(TARGET gameplayfootball POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_RUNTIME_DLLS:gameplayfootball>
        $<TARGET_FILE_DIR:gameplayfootball>
      COMMAND_EXPAND_LISTS
      COMMENT "Copying runtime DLLs beside gameplayfootball"
    )
  endif()
endif()

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

# ── Doxygen documentation target (task 5.1) ────────────────────────────────
if(GAMEPLAYFOOTBALL_BUILD_DOCS)
  find_package(Doxygen REQUIRED)
  # Generate Doxyfile from template, substituting project version
  configure_file(
    ${PROJECT_SOURCE_DIR}/Doxyfile.in
    ${PROJECT_SOURCE_DIR}/Doxyfile
    @ONLY
  )
  set(DOXYGEN_IN  ${PROJECT_SOURCE_DIR}/Doxyfile)
  set(DOXYGEN_OUT ${PROJECT_SOURCE_DIR}/docs)
  add_custom_target(docs
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_IN}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM
  )
endif()
