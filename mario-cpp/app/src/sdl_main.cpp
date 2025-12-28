// Minimal SDL2 app wrapper around the deterministic core simulation.
//
// Build with: -DMARIO_CPP_BUILD_SDL_APP=ON

#include <SDL.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>

#include "mario/core/config.hpp"
#include "mario/core/game_state.hpp"
#include "mario/core/world.hpp"

namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;

namespace {

bool read_file(const fs::path& path, std::string& out) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    return false;
  }
  in.seekg(0, std::ios::end);
  const std::streamoff size = in.tellg();
  if (size < 0) {
    return false;
  }
  out.resize(static_cast<std::size_t>(size));
  in.seekg(0, std::ios::beg);
  in.read(out.data(), size);
  return in.good();
}

int units_to_px(mario::core::units_t u) { return static_cast<int>(u / mario::core::kPosScale); }

SDL_Rect to_screen_rect(mario::core::Rect r, mario::core::Vec2 cam_top_left) {
  const mario::core::units_t sx = r.x - cam_top_left.x;
  const mario::core::units_t sy = r.y - cam_top_left.y;
  return SDL_Rect{
      units_to_px(sx),
      units_to_px(sy),
      units_to_px(r.w),
      units_to_px(r.h),
  };
}

struct KeyState {
  bool left = false;
  bool right = false;
  bool jump = false;
  bool start = false;
  bool restart = false;
  bool quit = false;
};

KeyState read_keys() {
  const Uint8* k = SDL_GetKeyboardState(nullptr);
  KeyState s{};
  s.left = k[SDL_SCANCODE_LEFT] || k[SDL_SCANCODE_A];
  s.right = k[SDL_SCANCODE_RIGHT] || k[SDL_SCANCODE_D];
  s.jump = k[SDL_SCANCODE_SPACE] || k[SDL_SCANCODE_UP] || k[SDL_SCANCODE_W];
  s.start = k[SDL_SCANCODE_RETURN];
  s.restart = k[SDL_SCANCODE_R];
  s.quit = k[SDL_SCANCODE_ESCAPE];
  return s;
}

}  // namespace

int main(int argc, char** argv) {
  fs::path assets_dir = "assets";
  if (argc >= 3 && std::string_view(argv[1]) == "--assets-dir") {
    assets_dir = fs::path(argv[2]);
  }

  const mario::core::Config config{};
  mario::core::World world{};
  {
    const fs::path level_path = assets_dir / "levels" / "level1.txt";
    std::string contents;
    std::string error;
    if (!read_file(level_path, contents) ||
        !mario::core::World::from_ascii(contents, config, world, error)) {
      std::cerr << "Failed to load level (" << level_path.string() << "): " << error
                << ". Using fallback.\n";
      if (!mario::core::World::from_ascii(mario::core::kFallbackLevel, config, world, error)) {
        std::cerr << "Fallback level parse error: " << error << "\n";
        return 2;
      }
    }
  }

  mario::core::GameState state = mario::core::make_new_game(std::move(world), config);

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
    std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
    return 2;
  }

  SDL_Window* window =
      SDL_CreateWindow("mario-cpp", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 960, 540,
                       SDL_WINDOW_RESIZABLE);
  if (!window) {
    std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
    SDL_Quit();
    return 2;
  }

  SDL_Renderer* renderer =
      SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!renderer) {
    std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << "\n";
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 2;
  }

  bool running = true;
  KeyState prev_keys{};
  KeyState keys = read_keys();

  auto last = Clock::now();
  double accumulator_s = 0.0;
  constexpr double dt_s = 1.0 / 60.0;

  while (running) {
    SDL_Event e;
    while (SDL_PollEvent(&e) != 0) {
      if (e.type == SDL_QUIT) {
        running = false;
      }
    }

    keys = read_keys();

    const auto now = Clock::now();
    const std::chrono::duration<double> frame_dt = now - last;
    last = now;
    accumulator_s += std::min(frame_dt.count(), 0.25);

    while (accumulator_s >= dt_s) {
      mario::core::StepInput input{};
      input.left = keys.left;
      input.right = keys.right;
      input.jump_pressed = keys.jump && !prev_keys.jump;
      input.jump_released = !keys.jump && prev_keys.jump;
      input.start_pressed = keys.start && !prev_keys.start;
      input.restart_pressed = keys.restart && !prev_keys.restart;
      input.quit_pressed = keys.quit && !prev_keys.quit;

      mario::core::step(state, input);

      prev_keys = keys;
      accumulator_s -= dt_s;
    }

    int w_px = 0;
    int h_px = 0;
    SDL_GetRendererOutputSize(renderer, &w_px, &h_px);

    const mario::core::units_t screen_w = static_cast<mario::core::units_t>(w_px) * mario::core::kPosScale;
    const mario::core::units_t screen_h = static_cast<mario::core::units_t>(h_px) * mario::core::kPosScale;
    const mario::core::units_t world_w =
        static_cast<mario::core::units_t>(state.world.width) * state.config.tile_size;
    const mario::core::units_t world_h =
        static_cast<mario::core::units_t>(state.world.height) * state.config.tile_size;

    mario::core::Vec2 focus = state.player.center();
    mario::core::units_t cam_x = focus.x;
    mario::core::units_t cam_y = focus.y;

    if (world_w > screen_w) {
      cam_x = std::clamp(cam_x, screen_w / 2, world_w - screen_w / 2);
    } else {
      cam_x = world_w / 2;
    }
    if (world_h > screen_h) {
      cam_y = std::clamp(cam_y, screen_h / 2, world_h - screen_h / 2);
    } else {
      cam_y = world_h / 2;
    }

    const mario::core::Vec2 cam_top_left{cam_x - screen_w / 2, cam_y - screen_h / 2};

    SDL_SetRenderDrawColor(renderer, 115, 191, 242, 255);
    SDL_RenderClear(renderer);

    // Solids.
    SDL_SetRenderDrawColor(renderer, 64, 140, 64, 255);
    for (const mario::core::Rect& solid : state.world.solids) {
      const SDL_Rect r = to_screen_rect(solid, cam_top_left);
      SDL_RenderFillRect(renderer, &r);
    }

    // Coins.
    SDL_SetRenderDrawColor(renderer, 240, 205, 50, 255);
    const mario::core::units_t coin_radius = state.config.tile_size / 5;
    const mario::core::units_t coin_size = coin_radius * 2;
    for (const mario::core::Vec2 c : state.world.coins) {
      const mario::core::Rect r{c.x - coin_radius, c.y - coin_radius, coin_size, coin_size};
      const SDL_Rect sr = to_screen_rect(r, cam_top_left);
      SDL_RenderFillRect(renderer, &sr);
    }

    // Mushrooms.
    SDL_SetRenderDrawColor(renderer, 217, 38, 140, 255);
    for (const mario::core::Vec2 m : state.world.mushrooms) {
      const mario::core::Rect r{m.x, m.y, state.config.mushroom_size.x, state.config.mushroom_size.y};
      const SDL_Rect sr = to_screen_rect(r, cam_top_left);
      SDL_RenderFillRect(renderer, &sr);
    }

    // Enemies.
    SDL_SetRenderDrawColor(renderer, 140, 90, 60, 255);
    for (const mario::core::Enemy& enemy : state.enemies) {
      if (!enemy.alive) {
        continue;
      }
      const SDL_Rect r = to_screen_rect(enemy.rect(), cam_top_left);
      SDL_RenderFillRect(renderer, &r);
    }

    // Player.
    if (state.player.powered) {
      SDL_SetRenderDrawColor(renderer, 60, 190, 110, 255);
    } else {
      SDL_SetRenderDrawColor(renderer, 200, 40, 45, 255);
    }
    const SDL_Rect pr = to_screen_rect(state.player.rect(), cam_top_left);
    SDL_RenderFillRect(renderer, &pr);

    SDL_RenderPresent(renderer);

    // Simple HUD via window title (no font dependency).
    const std::string title =
        "mario-cpp | score=" + std::to_string(state.score) + " | high=" + std::to_string(state.high_score);
    SDL_SetWindowTitle(window, title.c_str());
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
