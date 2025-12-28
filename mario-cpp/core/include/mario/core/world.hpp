#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "mario/core/config.hpp"
#include "mario/core/types.hpp"

namespace mario::core {

constexpr std::string_view kFallbackLevel =
    "................................\n"
    "................................\n"
    "................................\n"
    "................................\n"
    ".......C.........C.......C......\n"
    "......#####.....#####...#####...\n"
    "..P....M....E................G..\n"
    "#######...########..######...###\n";

struct World {
  std::vector<Rect> solids;
  std::vector<std::uint8_t> solid_tiles;  // row-major, width*height
  std::vector<Vec2> coins;                // centers
  std::vector<Vec2> mushrooms;            // top-left positions
  std::vector<Vec2> enemy_spawns;         // tile top-left positions
  Vec2 player_spawn;                      // tile top-left
  Vec2 goal_tile;                         // tile top-left
  int width = 0;
  int height = 0;

  static bool from_ascii(std::string_view contents, const Config& config, World& out,
                         std::string& error);

  bool is_solid_tile(int col, int row) const;
  std::optional<units_t> ground_y_for_x(units_t world_x, units_t start_y,
                                       const Config& config) const;

  Rect goal_trigger_rect(const Config& config) const;
};

}  // namespace mario::core

