#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "mario/core/input.hpp"

namespace mario::core {

// Replay format: JSONL (one JSON object per line).
//
// Header line (optional but written by default):
//   {"version":1,"level":"levels/level1.txt"}
//
// Per-tick input lines:
//   {"l":0,"r":1,"jp":0,"jr":0,"start":0,"restart":0,"quit":0}
struct Replay {
  std::uint32_t version = 1;
  std::string level;
  std::vector<StepInput> inputs;
};

std::string replay_to_jsonl(const Replay& replay);
bool replay_from_jsonl(std::string_view jsonl, Replay& out, std::string& error);

}  // namespace mario::core

