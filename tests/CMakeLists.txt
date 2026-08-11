include(FetchContent)
include(GoogleTest)

find_package(SQLite3 REQUIRED)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_MakeAvailable(googletest)

add_executable(gameplayfootball_math_tests
  base/math_test.cpp
  onthepitch/ball_physics_test.cpp
)

target_link_libraries(gameplayfootball_math_tests PRIVATE
  gf_ballphysicslib
  gf_mathlib
  GTest::gtest_main
)

gtest_discover_tests(gameplayfootball_math_tests)

# Velocity state machine unit tests (task 2.5)
add_executable(gameplayfootball_velocity_tests
  onthepitch/velocity_state_test.cpp
)

target_include_directories(gameplayfootball_velocity_tests PRIVATE
  ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(gameplayfootball_velocity_tests PRIVATE
  GTest::gtest_main
)

gtest_discover_tests(gameplayfootball_velocity_tests)

# Match simulation integration tests (task 2.6)
add_executable(gameplayfootball_integration_tests
  integration/match_simulation_test.cpp
)

target_include_directories(gameplayfootball_integration_tests PRIVATE
  ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(gameplayfootball_integration_tests PRIVATE
  GTest::gtest_main
)

gtest_discover_tests(gameplayfootball_integration_tests)

add_executable(gameplayfootball_league_integration_tests
  integration/league_bootstrap_test.cpp
  ${PROJECT_SOURCE_DIR}/src/league/leaguecode.cpp
  ${PROJECT_SOURCE_DIR}/src/league/leaguesetup.cpp
  ${PROJECT_SOURCE_DIR}/src/utils/database.cpp
  ${PROJECT_SOURCE_DIR}/src/utils/xmlloader.cpp
  ${PROJECT_SOURCE_DIR}/src/base/properties.cpp
  ${PROJECT_SOURCE_DIR}/src/base/utils.cpp
)

target_include_directories(gameplayfootball_league_integration_tests PRIVATE
  ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(gameplayfootball_league_integration_tests PRIVATE
  gf_mathlib
  GTest::gtest_main
  SQLite::SQLite3
)

gtest_discover_tests(gameplayfootball_league_integration_tests)

# Career data unit tests (Phase 6: tasks 6.13, 6.16, 6.17)
# Also covers the stat-based match simulation (career_database.cpp).
add_executable(gameplayfootball_career_tests
  career/career_modules_test.cpp
  career/careerdata_test.cpp
  career/career_simulation_test.cpp
  career/career_saveload_test.cpp
  career/career_longrun_audit_test.cpp
  ${PROJECT_SOURCE_DIR}/src/data/careerdata.cpp
  ${PROJECT_SOURCE_DIR}/src/data/draftdata.cpp
  ${PROJECT_SOURCE_DIR}/src/data/scoutingdata.cpp
  ${PROJECT_SOURCE_DIR}/src/menu/career/career_board.cpp
  ${PROJECT_SOURCE_DIR}/src/menu/career/career_common.cpp
  ${PROJECT_SOURCE_DIR}/src/menu/career/career_database.cpp
  ${PROJECT_SOURCE_DIR}/src/menu/career/career_finance.cpp
  ${PROJECT_SOURCE_DIR}/src/menu/career/career_persistence.cpp
  ${PROJECT_SOURCE_DIR}/src/menu/career/career_sim.cpp
  ${PROJECT_SOURCE_DIR}/src/menu/career/career_sponsors.cpp
  ${PROJECT_SOURCE_DIR}/src/menu/career/career_staff.cpp
  ${PROJECT_SOURCE_DIR}/src/menu/career/career_training.cpp
  ${PROJECT_SOURCE_DIR}/src/menu/career/career_transfers.cpp
  ${PROJECT_SOURCE_DIR}/src/utils/localization.cpp
  ${PROJECT_SOURCE_DIR}/src/base/log.cpp
)

target_include_directories(gameplayfootball_career_tests PRIVATE
  ${PROJECT_SOURCE_DIR}/src
  ${SQLite3_INCLUDE_DIRS}
)

target_link_libraries(gameplayfootball_career_tests PRIVATE
  GTest::gtest_main
  SQLite::SQLite3
)

gtest_discover_tests(gameplayfootball_career_tests
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)
