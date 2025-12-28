#include <catch2/catch_test_macros.hpp>

#include <string>

#include "mario/core/config.hpp"
#include "mario/core/world.hpp"

TEST_CASE("world: rejects missing spawns") {
  const mario::core::Config config{};
  mario::core::World world{};
  std::string error;

  CHECK_FALSE(mario::core::World::from_ascii("..\n..\n", config, world, error));
  CHECK_FALSE(error.empty());
}

TEST_CASE("world: parses fallback level") {
  const mario::core::Config config{};
  mario::core::World world{};
  std::string error;

  REQUIRE(mario::core::World::from_ascii(mario::core::kFallbackLevel, config, world, error));
  REQUIRE(world.width > 0);
  REQUIRE(world.height > 0);
  REQUIRE(world.solids.size() > 0);
  REQUIRE(world.enemy_spawns.size() > 0);
  REQUIRE(world.coins.size() > 0);
  REQUIRE(world.mushrooms.size() > 0);
}

