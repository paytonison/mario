#pragma once

#include <cstdint>

#include "mario/core/constants.hpp"
#include "mario/core/types.hpp"

namespace mario::core {

struct Config {
  // Geometry (units).
  units_t tile_size = 32 * kPosScale;
  Vec2 player_size = Vec2{22 * kPosScale, 28 * kPosScale};
  Vec2 enemy_size = Vec2{24 * kPosScale, 20 * kPosScale};
  Vec2 mushroom_size = Vec2{24 * kPosScale, 22 * kPosScale};

  // Movement (velocities are (px/tick)*kPosScale).
  units_t move_speed = 220 * 60;
  units_t move_accel = 1600;
  units_t move_decel = 2000;

  units_t gravity = 1200;
  units_t terminal_velocity = 780 * 60;
  units_t jump_speed = 420 * 60;

  units_t stomp_bounce = 320 * 60;
  units_t enemy_speed = 65 * 60;

  units_t hurt_knockback_x = 200 * 60;
  units_t hurt_knockback_y = 260 * 60;

  // Timers (time units: 1s == kTimeScale).
  std::int32_t coyote_time = 60;        // 0.1s
  std::int32_t jump_buffer_time = 72;   // 0.12s
  std::int32_t hurt_invuln_time = 450;  // 0.75s
};

}  // namespace mario::core

