#include "mario/core/enemy.hpp"

#include <algorithm>

#include "mario/core/physics.hpp"
#include "mario/core/types.hpp"

namespace mario::core {

void Enemy::reset(Vec2 spawn_tile, const World& world, const Config& config) {
  size = config.enemy_size;
  const units_t tile = config.tile_size;
  const units_t x = spawn_tile.x + (tile - size.x) / 2;
  const units_t sample_x = spawn_tile.x + tile / 2;
  const units_t base_y = world.ground_y_for_x(sample_x, spawn_tile.y, config).value_or(spawn_tile.y + tile);
  const units_t y = base_y - size.y;

  pos = Vec2{x, y};
  vel = Vec2{0, 0};
  dir = -1;
  alive = true;
  on_ground = false;
}

void Enemy::update(const World& world, const Config& config) {
  if (!alive) {
    return;
  }

  vel.y = std::min(vel.y + config.gravity, config.terminal_velocity);
  vel.x = static_cast<units_t>(dir) * config.enemy_speed;

  const units_t desired_x = vel.x;
  const physics::MoveResult moved = physics::move_with_collisions(pos, size, vel, world.solids);
  const bool hit_wall = (desired_x != 0) && (moved.vel.x == 0);

  pos = moved.pos;
  vel = moved.vel;
  on_ground = moved.on_ground;

  if (hit_wall) {
    dir *= -1;
    vel.x = static_cast<units_t>(dir) * config.enemy_speed;
  } else if (on_ground) {
    const units_t foot_x =
        (dir >= 0) ? (pos.x + size.x + px_to_units(1)) : (pos.x - px_to_units(1));
    const units_t foot_y = pos.y + size.y + px_to_units(1);

    bool has_ground = false;
    if (const auto ground_y = world.ground_y_for_x(foot_x, foot_y, config); ground_y.has_value()) {
      has_ground = (*ground_y <= foot_y);
    }

    if (!has_ground) {
      dir *= -1;
      vel.x = static_cast<units_t>(dir) * config.enemy_speed;
    }
  }

  const units_t world_w = static_cast<units_t>(world.width) * config.tile_size;
  if (pos.x <= 0) {
    pos.x = 0;
    dir = 1;
  } else if (pos.x + size.x >= world_w) {
    pos.x = std::max<units_t>(0, world_w - size.x);
    dir = -1;
  }
}

}  // namespace mario::core
