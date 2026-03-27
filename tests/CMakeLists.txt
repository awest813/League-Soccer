include(FetchContent)
include(GoogleTest)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
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
