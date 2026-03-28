cmake_minimum_required(VERSION 3.14)

set(PACKAGE GameplayFootball)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 4)
set(VERSION_PATCH 0)
if(${VERSION_PATCH})
   set(VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})
else(${VERSION_PATCH})
   set(VERSION ${VERSION_MAJOR}.${VERSION_MINOR})
endif(${VERSION_PATCH})

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
   set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -Wall -Wextra")
   set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall -O3")
endif()

# Find required libraries
if(GAMEPLAYFOOTBALL_BUILD_GAME)
  find_package(Boost REQUIRED COMPONENTS system)
  find_package(OpenGL REQUIRED)
  find_package(SDL2 REQUIRED)
  find_package(SDL2_image REQUIRED)
  find_package(SDL2_ttf REQUIRED)
  find_package(SDL2_gfx REQUIRED)
  find_package(OpenAL REQUIRED)
  find_package(SQLite3 REQUIRED)
else()
  find_package(Boost REQUIRED)
endif()

# Source files
include(sources.cmake)

# Helper function: apply common include dirs to an OBJECT library target
function(gf_target_includes tgt)
  target_include_directories(${tgt} PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${OPENGL_INCLUDE_DIR}
    ${SDL2_INCLUDE_DIR}
    ${SDL2_IMAGE_DIRS}
    ${SDL2_TTF_DIRS}
    ${SDL2_GFX_DIRS}
    ${Boost_INCLUDE_DIR}
    ${OPENAL_INCLUDE_DIR}
    ${SQLite3_INCLUDE_DIRS}
  )
endfunction()

add_library(gf_mathlib STATIC
  src/base/log.cpp
  ${BASE_MATH_SOURCES}
  ${BASE_MATH_HEADERS})
target_include_directories(gf_mathlib PUBLIC
  ${PROJECT_SOURCE_DIR}/src
  ${Boost_INCLUDE_DIR}
)

add_library(gf_ballphysicslib STATIC
  src/onthepitch/ballphysics.cpp
  src/onthepitch/ballphysics.hpp)
target_link_libraries(gf_ballphysicslib PUBLIC gf_mathlib)
target_include_directories(gf_ballphysicslib PUBLIC
  ${PROJECT_SOURCE_DIR}/src
)

if(GAMEPLAYFOOTBALL_BUILD_GAME)
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

  add_library(datalib ${DATA_SOURCES} ${DATA_HEADERS})
  gf_target_includes(datalib)

  set(LIBRARIES gamelib hidlib menulib datalib leaguelib blunted2
     Boost::system SQLite::SQLite3
     ${SDL2_IMAGE_LIBRARIES} ${SDL2_TTF_LIBRARIES} ${SDL2_GFX_LIBRARIES}
     ${SDL2_LIBRARIES} ${OPENAL_LIBRARY} dl m ${OPENGL_LIBRARIES})

  add_executable(gameplayfootball WIN32 ${CORE_SOURCES} ${CORE_HEADERS})
  target_link_libraries(gameplayfootball PRIVATE ${LIBRARIES})
  target_include_directories(gameplayfootball PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${OPENGL_INCLUDE_DIR}
    ${SDL2_INCLUDE_DIR}
    ${SDL2_IMAGE_DIRS}
    ${SDL2_TTF_DIRS}
    ${SDL2_GFX_DIRS}
    ${Boost_INCLUDE_DIR}
    ${OPENAL_INCLUDE_DIR}
    ${SQLite3_INCLUDE_DIRS}
  )
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
