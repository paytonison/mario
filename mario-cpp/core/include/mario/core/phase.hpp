#pragma once

#include <cstdint>

namespace mario::core {

enum class Phase : std::uint8_t {
  Title = 0,
  Playing = 1,
  LevelComplete = 2,
};

}  // namespace mario::core

