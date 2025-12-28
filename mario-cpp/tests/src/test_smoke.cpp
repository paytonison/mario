#include <catch2/catch_test_macros.hpp>

#include <string>

#include "mario/core/config.hpp"
#include "mario/core/game_state.hpp"
#include "mario/core/world.hpp"

TEST_CASE("core: deterministic fixed-tick simulation") {
  const mario::core::Config config{};
  std::string error;
  mario::core::World world{};
  REQUIRE(mario::core::World::from_ascii(mario::core::kFallbackLevel, config, world, error));

  mario::core::GameState a = mario::core::make_new_game(world, config);
  mario::core::GameState b = mario::core::make_new_game(world, config);

  mario::core::StepInput input{};
  for (int i = 0; i < 120; ++i) {
    mario::core::step(a, input);
    mario::core::step(b, input);
  }

  REQUIRE(a.tick == 120);
  REQUIRE(b.tick == 120);
  REQUIRE(mario::core::hash_state(a) == mario::core::hash_state(b));
}
