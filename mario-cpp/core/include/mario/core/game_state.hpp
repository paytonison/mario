#pragma once

#include <cstdint>
#include <vector>

#include "mario/core/config.hpp"
#include "mario/core/enemy.hpp"
#include "mario/core/input.hpp"
#include "mario/core/phase.hpp"
#include "mario/core/player.hpp"
#include "mario/core/world.hpp"

namespace mario::core {

struct GameState {
  Phase phase = Phase::Title;
  std::uint64_t tick = 0;

  Config config{};
  World world{};
  Player player{};
  std::vector<Enemy> enemies{};

  std::vector<Vec2> coin_spawns{};
  std::vector<Vec2> mushroom_spawns{};

  std::uint32_t score = 0;
  std::uint32_t high_score = 0;
};

GameState make_new_game(World world, const Config& config = Config{});

void step(GameState& state, const StepInput& input);

std::uint64_t hash_state(const GameState& state);

}  // namespace mario::core

