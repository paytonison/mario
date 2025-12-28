add_library(mario_core STATIC
  src/enemy.cpp
  src/game_state.cpp
  src/player.cpp
  src/replay.cpp
  src/world.cpp
)

target_include_directories(mario_core
  PUBLIC
    "${CMAKE_CURRENT_LIST_DIR}/include"
)

target_compile_features(mario_core PUBLIC cxx_std_20)

add_library(mario::core ALIAS mario_core)
