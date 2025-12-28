include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.5.4
)
FetchContent_MakeAvailable(Catch2)

add_executable(mario_tests
  src/test_smoke.cpp
  src/test_replay.cpp
  src/test_world.cpp
)

target_link_libraries(mario_tests PRIVATE mario::core Catch2::Catch2WithMain)
target_compile_features(mario_tests PRIVATE cxx_std_20)

get_filename_component(MARIO_CPP_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(MARIO_CPP_ROOT_DIR_ESCAPED "${MARIO_CPP_ROOT_DIR}")
string(REPLACE "\\" "\\\\" MARIO_CPP_ROOT_DIR_ESCAPED "${MARIO_CPP_ROOT_DIR_ESCAPED}")

configure_file(
  "${CMAKE_CURRENT_LIST_DIR}/src/test_paths.hpp.in"
  "${CMAKE_CURRENT_BINARY_DIR}/generated/test_paths.hpp"
  @ONLY
)

target_include_directories(mario_tests PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")

include(CTest)
include(Catch)
catch_discover_tests(mario_tests)
