#pragma once

namespace mario::core {

struct StepInput {
  bool left = false;
  bool right = false;
  bool jump_pressed = false;
  bool jump_released = false;
  bool start_pressed = false;
  bool restart_pressed = false;
  bool quit_pressed = false;

  int move_x() const { return static_cast<int>(right) - static_cast<int>(left); }
};

}  // namespace mario::core

