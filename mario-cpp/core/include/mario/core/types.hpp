#pragma once

#include <cstdint>

#include "mario/core/constants.hpp"

namespace mario::core {

using units_t = std::int64_t;

struct Vec2 {
  units_t x = 0;
  units_t y = 0;

  constexpr Vec2() = default;
  constexpr Vec2(units_t x_in, units_t y_in) : x(x_in), y(y_in) {}
};

struct Rect {
  units_t x = 0;
  units_t y = 0;
  units_t w = 0;
  units_t h = 0;

  constexpr Rect() = default;
  constexpr Rect(units_t x_in, units_t y_in, units_t w_in, units_t h_in)
      : x(x_in), y(y_in), w(w_in), h(h_in) {}
};

constexpr Vec2 operator+(Vec2 a, Vec2 b) { return Vec2{a.x + b.x, a.y + b.y}; }
constexpr Vec2 operator-(Vec2 a, Vec2 b) { return Vec2{a.x - b.x, a.y - b.y}; }

constexpr Vec2 operator*(Vec2 a, units_t s) { return Vec2{a.x * s, a.y * s}; }
constexpr Vec2 operator/(Vec2 a, units_t s) { return Vec2{a.x / s, a.y / s}; }

constexpr Rect rect_at(Vec2 pos, Vec2 size) { return Rect{pos.x, pos.y, size.x, size.y}; }

constexpr units_t px_to_units(units_t px) { return px * kPosScale; }

constexpr units_t floor_div(units_t a, units_t b) {
  // b must be positive.
  if (a >= 0) {
    return a / b;
  }
  return -(((-a) + (b - 1)) / b);
}

constexpr int signum(units_t v) {
  if (v > 0) {
    return 1;
  }
  if (v < 0) {
    return -1;
  }
  return 0;
}

}  // namespace mario::core

