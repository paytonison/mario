#include "mario/core/world.hpp"

#include <algorithm>
#include <cctype>
#include <optional>
#include <string_view>

#include "mario/core/types.hpp"

namespace mario::core {
namespace {

std::string_view trim_end(std::string_view s) {
  while (!s.empty()) {
    const unsigned char ch = static_cast<unsigned char>(s.back());
    if (ch == '\r' || ch == ' ' || ch == '\t' || ch == '\v' || ch == '\f') {
      s.remove_suffix(1);
      continue;
    }
    break;
  }
  return s;
}

}  // namespace

bool World::from_ascii(std::string_view contents, const Config& config, World& out, std::string& error) {
  std::vector<std::string_view> lines;
  lines.reserve(64);

  std::size_t pos = 0;
  while (pos <= contents.size()) {
    const std::size_t next_nl = contents.find('\n', pos);
    const std::size_t end = (next_nl == std::string_view::npos) ? contents.size() : next_nl;
    std::string_view line = contents.substr(pos, end - pos);
    line = trim_end(line);
    if (!line.empty()) {
      lines.push_back(line);
    }

    if (next_nl == std::string_view::npos) {
      break;
    }
    pos = next_nl + 1;
  }

  const int height = static_cast<int>(lines.size());
  int width = 0;
  for (const auto line : lines) {
    width = std::max(width, static_cast<int>(line.size()));
  }

  if (width <= 0 || height <= 0) {
    error = "Level has no tiles";
    return false;
  }

  out = World{};
  out.width = width;
  out.height = height;

  out.solid_tiles.assign(static_cast<std::size_t>(width * height), 0);
  out.solids.clear();
  out.coins.clear();
  out.mushrooms.clear();
  out.enemy_spawns.clear();

  std::vector<Vec2> mushroom_tiles;
  std::optional<Vec2> player_spawn;
  std::optional<Vec2> goal_tile;

  const units_t tile = config.tile_size;

  for (int row = 0; row < height; ++row) {
    const std::string_view line = lines[static_cast<std::size_t>(row)];
    for (int col = 0; col < static_cast<int>(line.size()); ++col) {
      const char ch = line[static_cast<std::size_t>(col)];
      const units_t world_x = static_cast<units_t>(col) * tile;
      const units_t world_y = static_cast<units_t>(row) * tile;
      const Vec2 tile_pos{world_x, world_y};

      switch (ch) {
        case '#': {
          out.solid_tiles[static_cast<std::size_t>(row * width + col)] = 1;
          out.solids.push_back(Rect{tile_pos.x, tile_pos.y, tile, tile});
        } break;
        case 'C':
          out.coins.push_back(Vec2{world_x + tile / 2, world_y + tile / 2});
          break;
        case 'M':
          mushroom_tiles.push_back(tile_pos);
          break;
        case 'E':
          out.enemy_spawns.push_back(tile_pos);
          break;
        case 'P':
          if (player_spawn.has_value()) {
            error = "Multiple player spawns found";
            return false;
          }
          player_spawn = tile_pos;
          break;
        case 'G':
          if (goal_tile.has_value()) {
            error = "Multiple goal tiles found";
            return false;
          }
          goal_tile = tile_pos;
          break;
        case '.':
          break;
        default:
          error = std::string("Unexpected tile '") + ch + "'";
          return false;
      }
    }
  }

  if (!player_spawn.has_value()) {
    error = "Missing player spawn";
    return false;
  }
  if (!goal_tile.has_value()) {
    error = "Missing goal tile";
    return false;
  }

  out.player_spawn = *player_spawn;
  out.goal_tile = *goal_tile;

  out.mushrooms.reserve(mushroom_tiles.size());
  for (const Vec2 tile_pos : mushroom_tiles) {
    const Vec2 size = config.mushroom_size;
    const units_t x = tile_pos.x + (tile - size.x) / 2;
    const units_t sample_x = tile_pos.x + tile / 2;
    const units_t base_y =
        out.ground_y_for_x(sample_x, tile_pos.y, config).value_or(tile_pos.y + tile);
    const units_t y = base_y - size.y;
    out.mushrooms.push_back(Vec2{x, y});
  }

  return true;
}

bool World::is_solid_tile(int col, int row) const {
  if (col < 0 || row < 0) {
    return false;
  }
  if (col >= width || row >= height) {
    return false;
  }
  return solid_tiles[static_cast<std::size_t>(row * width + col)] != 0;
}

std::optional<units_t> World::ground_y_for_x(units_t world_x, units_t start_y,
                                            const Config& config) const {
  const units_t tile = config.tile_size;
  const int col = static_cast<int>(floor_div(world_x, tile));
  const int start_row = std::max<int>(0, static_cast<int>(floor_div(start_y, tile)));

  for (int row = start_row; row < height; ++row) {
    if (is_solid_tile(col, row)) {
      return static_cast<units_t>(row) * tile;
    }
  }
  return std::nullopt;
}

Rect World::goal_trigger_rect(const Config& config) const {
  const units_t tile = config.tile_size;
  const units_t goal_center_x = goal_tile.x + tile / 2;
  const units_t base_y = ground_y_for_x(goal_center_x, goal_tile.y, config).value_or(goal_tile.y + tile);

  const units_t pole_height = tile * 3;
  const units_t pole_w = (tile * 9) / 50;  // tile * 0.18
  const units_t pole_x = goal_center_x - pole_w / 2;
  const units_t pole_y = base_y - pole_height;
  return Rect{pole_x, pole_y, pole_w, pole_height};
}

}  // namespace mario::core

