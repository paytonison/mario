#include "mario/core/player.hpp"

#include <algorithm>

#include "mario/core/constants.hpp"
#include "mario/core/physics.hpp"
#include "mario/core/types.hpp"

namespace mario::core {

void Player::reset(Vec2 spawn_tile, const Config& config) {
  size = config.player_size;
  pos = Vec2{
      spawn_tile.x + (config.tile_size - size.x) / 2,
      spawn_tile.y + (config.tile_size - size.y),
  };

  vel = Vec2{0, 0};
  on_ground = false;
  facing = 1;
  coyote_timer = 0;
  jump_buffer_timer = 0;
  powered = false;
  invuln_timer = 0;
}

bool Player::update(const StepInput& input, const World& world, const Config& config) {
  invuln_timer = std::max<std::int32_t>(0, invuln_timer - kDtTimeUnits);

  bool jumped = false;

  if (input.jump_pressed) {
    jump_buffer_timer = config.jump_buffer_time;
  } else {
    jump_buffer_timer = std::max<std::int32_t>(0, jump_buffer_timer - kDtTimeUnits);
  }

  if (input.jump_released && vel.y < 0) {
    vel.y /= 2;  // jump_cut_multiplier = 0.5
  }

  if (on_ground) {
    coyote_timer = config.coyote_time;
  } else {
    coyote_timer = std::max<std::int32_t>(0, coyote_timer - kDtTimeUnits);
  }

  const int move_x = input.move_x();
  if (move_x != 0) {
    facing = move_x < 0 ? -1 : 1;
  }

  const units_t target_speed = static_cast<units_t>(move_x) * config.move_speed;
  const units_t accel = (move_x != 0) ? config.move_accel : config.move_decel;
  vel.x = physics::approach(vel.x, target_speed, accel);

  if (jump_buffer_timer > 0 && coyote_timer > 0) {
    vel.y = -config.jump_speed;
    on_ground = false;
    coyote_timer = 0;
    jump_buffer_timer = 0;
    jumped = true;
  }

  vel.y = std::min(vel.y + config.gravity, config.terminal_velocity);

  const physics::MoveResult moved = physics::move_with_collisions(pos, size, vel, world.solids);
  pos = moved.pos;
  vel = moved.vel;
  on_ground = moved.on_ground;

  if (jump_buffer_timer > 0 && on_ground) {
    vel.y = -config.jump_speed;
    on_ground = false;
    coyote_timer = 0;
    jump_buffer_timer = 0;
    jumped = true;
  }

  return jumped;
}

}  // namespace mario::core

