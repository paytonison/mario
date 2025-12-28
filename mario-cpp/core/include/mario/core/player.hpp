#pragma once

#include <cstdint>

#include "mario/core/config.hpp"
#include "mario/core/input.hpp"
#include "mario/core/types.hpp"
#include "mario/core/world.hpp"

namespace mario::core {

struct Player {
  Vec2 pos;
  Vec2 vel;
  bool on_ground = false;
  Vec2 size;

  int facing = 1;
  std::int32_t coyote_timer = 0;
  std::int32_t jump_buffer_timer = 0;
  bool powered = false;
  std::int32_t invuln_timer = 0;

  void reset(Vec2 spawn_tile, const Config& config);
  bool update(const StepInput& input, const World& world, const Config& config);

  Rect rect() const { return rect_at(pos, size); }
  Vec2 center() const { return pos + (size / 2); }

  bool is_invulnerable() const { return invuln_timer > 0; }
};

}  // namespace mario::core

