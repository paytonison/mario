#pragma once

#include <span>

#include "mario/core/types.hpp"

namespace mario::core::physics {

constexpr bool rects_intersect(Rect a, Rect b) {
  return a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h && a.y + a.h > b.y;
}

constexpr units_t approach(units_t value, units_t target, units_t delta) {
  if (value < target) {
    const units_t next = value + delta;
    return next < target ? next : target;
  }
  const units_t next = value - delta;
  return next > target ? next : target;
}

struct MoveResult {
  Vec2 pos;
  Vec2 vel;
  bool on_ground = false;
};

inline MoveResult move_with_collisions(Vec2 pos, Vec2 size, Vec2 vel, std::span<const Rect> solids) {
  MoveResult out{};
  out.pos = pos;
  out.vel = vel;
  out.on_ground = false;

  out.pos.x += out.vel.x;
  Rect rect = rect_at(out.pos, size);
  for (const Rect& solid : solids) {
    if (!rects_intersect(rect, solid)) {
      continue;
    }
    if (out.vel.x > 0) {
      out.pos.x = solid.x - size.x;
    } else if (out.vel.x < 0) {
      out.pos.x = solid.x + solid.w;
    }
    out.vel.x = 0;
    rect.x = out.pos.x;
  }

  out.pos.y += out.vel.y;
  rect.y = out.pos.y;
  for (const Rect& solid : solids) {
    if (!rects_intersect(rect, solid)) {
      continue;
    }
    if (out.vel.y > 0) {
      out.pos.y = solid.y - size.y;
      out.on_ground = true;
    } else if (out.vel.y < 0) {
      out.pos.y = solid.y + solid.h;
    }
    out.vel.y = 0;
    rect.y = out.pos.y;
  }

  return out;
}

}  // namespace mario::core::physics

