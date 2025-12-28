#pragma once

#include <cstdint>

namespace mario::core {

constexpr int kTickHz = 60;

// Fixed-point coordinate system:
// - positions are stored in "units" where 1px == kPosScale units.
// - velocities are stored as (px / tick) * kPosScale.
constexpr std::int64_t kPosScale = 3600;

// Fixed-point timer system:
// - timers are stored in "time units" where 1s == kTimeScale units.
// - per-tick dt == (kTimeScale / kTickHz) units.
constexpr std::int32_t kTimeScale = 600;
static_assert(kTimeScale % kTickHz == 0, "kTimeScale must be divisible by kTickHz");
constexpr std::int32_t kDtTimeUnits = kTimeScale / kTickHz;  // 10

}  // namespace mario::core

