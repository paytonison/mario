#pragma once

#include <cstdint>

#include "mario/core/config.hpp"
#include "mario/core/types.hpp"
#include "mario/core/world.hpp"

namespace mario::core {

struct Enemy {
  Vec2 pos;
  Vec2 vel;
  int dir = -1;
  bool alive = true;
  Vec2 size;
  bool on_ground = false;

  void reset(Vec2 spawn_tile, const World& world, const Config& config);
  void update(const World& world, const Config& config);

  Rect rect() const { return rect_at(pos, size); }
};

}  // namespace mario::core

