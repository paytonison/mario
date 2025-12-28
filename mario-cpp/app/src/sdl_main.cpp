// Minimal SDL2 app wrapper around the deterministic core simulation.
//
// Build with: -DMARIO_CPP_BUILD_SDL_APP=ON

#include <SDL.h>
#include <SDL_image.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "mario/core/config.hpp"
#include "mario/core/game_state.hpp"
#include "mario/core/world.hpp"

namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;

namespace {

struct Args {
  fs::path assets_dir = "assets";
  fs::path jumpman_assets_dir{};
  fs::path level = fs::path("levels") / "level1.txt";
};

void print_usage(std::ostream& os) {
  os << "Usage:\n"
     << "  mario_sdl [--assets-dir DIR] [--jumpman-assets-dir DIR] [--level PATH]\n";
}

bool parse_args(int argc, char** argv, Args& out, bool& wants_help) {
  wants_help = false;
  for (int i = 1; i < argc; ++i) {
    const std::string_view arg = argv[i];
    auto require_value = [&](std::string_view flag) -> std::optional<std::string_view> {
      if (i + 1 >= argc) {
        std::cerr << "Missing value for " << flag << "\n";
        return std::nullopt;
      }
      i += 1;
      return std::string_view{argv[i]};
    };

    if (arg == "--help" || arg == "-h") {
      print_usage(std::cout);
      wants_help = true;
      return false;
    }
    if (arg == "--assets-dir") {
      auto v = require_value(arg);
      if (!v.has_value()) {
        return false;
      }
      out.assets_dir = fs::path{std::string(*v)};
      continue;
    }
    if (arg == "--jumpman-assets-dir") {
      auto v = require_value(arg);
      if (!v.has_value()) {
        return false;
      }
      out.jumpman_assets_dir = fs::path{std::string(*v)};
      continue;
    }
    if (arg == "--level") {
      auto v = require_value(arg);
      if (!v.has_value()) {
        return false;
      }
      out.level = fs::path{std::string(*v)};
      continue;
    }

    std::cerr << "Unknown argument: " << arg << "\n";
    return false;
  }

  return true;
}

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

int world_to_screen_x(mario::core::units_t world_x, mario::core::Vec2 cam_top_left) {
  return units_to_px(world_x - cam_top_left.x);
}

int world_to_screen_y(mario::core::units_t world_y, mario::core::Vec2 cam_top_left) {
  return units_to_px(world_y - cam_top_left.y);
}

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

struct Texture {
  SDL_Texture* handle = nullptr;
  int w = 0;
  int h = 0;

  Texture() = default;
  Texture(const Texture&) = delete;
  Texture& operator=(const Texture&) = delete;

  Texture(Texture&& other) noexcept { *this = std::move(other); }

  Texture& operator=(Texture&& other) noexcept {
    if (this == &other) {
      return *this;
    }
    reset();
    handle = other.handle;
    w = other.w;
    h = other.h;
    other.handle = nullptr;
    other.w = 0;
    other.h = 0;
    return *this;
  }

  ~Texture() { reset(); }

  void reset() {
    if (handle) {
      SDL_DestroyTexture(handle);
      handle = nullptr;
    }
    w = 0;
    h = 0;
  }
};

std::optional<Texture> load_png_texture(SDL_Renderer* renderer, const fs::path& path) {
  SDL_Surface* surface = IMG_Load(path.string().c_str());
  if (!surface) {
    std::cerr << "IMG_Load failed (" << path.string() << "): " << IMG_GetError() << "\n";
    return std::nullopt;
  }

  SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surface);
  const int w = surface->w;
  const int h = surface->h;
  SDL_FreeSurface(surface);

  if (!tex) {
    std::cerr << "SDL_CreateTextureFromSurface failed (" << path.string()
              << "): " << SDL_GetError() << "\n";
    return std::nullopt;
  }
  SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);

  Texture out{};
  out.handle = tex;
  out.w = w;
  out.h = h;
  return out;
}

struct SpriteSheet {
  Texture texture{};
  int cols = 0;
  int rows = 0;
  int cell_w = 0;
  int cell_h = 0;

  SDL_Rect src_rect(int frame) const {
    const int clamped = std::clamp(frame, 0, cols * rows - 1);
    const int col = clamped % cols;
    const int row = clamped / cols;
    return SDL_Rect{col * cell_w, row * cell_h, cell_w, cell_h};
  }
};

int anim_frame(std::uint64_t tick, int frames, int ticks_per_frame) {
  if (frames <= 0 || ticks_per_frame <= 0) {
    return 0;
  }
  return static_cast<int>((tick / static_cast<std::uint64_t>(ticks_per_frame)) %
                          static_cast<std::uint64_t>(frames));
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
  Args args{};
  bool wants_help = false;
  if (!parse_args(argc, argv, args, wants_help)) {
    return wants_help ? 0 : 1;
  }

  const mario::core::Config config{};
  mario::core::World world{};
  {
    const fs::path level_path = args.assets_dir / args.level;
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

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");  // nearest neighbor scaling

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

  const int img_flags = IMG_INIT_PNG;
  if ((IMG_Init(img_flags) & img_flags) != img_flags) {
    std::cerr << "IMG_Init failed: " << IMG_GetError() << "\n";
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 2;
  }

  const fs::path jumpman_assets_dir =
      args.jumpman_assets_dir.empty()
          ? (args.assets_dir.parent_path() / "jumpman_assets").lexically_normal()
          : args.jumpman_assets_dir;

  std::optional<SpriteSheet> player_sheet;
  std::optional<SpriteSheet> enemy_sheet;
  std::optional<SpriteSheet> icons_sheet;
  std::optional<SpriteSheet> tiles_sheet;
  {
    if (auto tex = load_png_texture(renderer, jumpman_assets_dir / "sprocket_character_32x32.png");
        tex.has_value()) {
      player_sheet = SpriteSheet{std::move(*tex), 4, 3, 32, 32};
    }
    if (auto tex = load_png_texture(renderer, jumpman_assets_dir / "chestnut_guy_32x32.png");
        tex.has_value()) {
      enemy_sheet = SpriteSheet{std::move(*tex), 4, 2, 32, 32};
    }
    if (auto tex = load_png_texture(renderer, jumpman_assets_dir / "icons_ui_16x16.png");
        tex.has_value()) {
      icons_sheet = SpriteSheet{std::move(*tex), 8, 2, 16, 16};
    }
    if (auto tex = load_png_texture(renderer, jumpman_assets_dir / "tileset_16x16.png");
        tex.has_value()) {
      tiles_sheet = SpriteSheet{std::move(*tex), 8, 4, 16, 16};
    }

    if (!player_sheet.has_value() || !enemy_sheet.has_value() || !icons_sheet.has_value() ||
        !tiles_sheet.has_value()) {
      std::cerr << "Warning: one or more sprite sheets failed to load from "
                << jumpman_assets_dir.string() << "; falling back to debug rectangles.\n";
    }
  }

  bool running = true;
  KeyState prev_keys{};
  KeyState keys = read_keys();
  std::vector<bool> enemy_was_alive(state.enemies.size(), true);
  std::vector<std::uint64_t> enemy_death_tick(state.enemies.size(), 0);

  {
    mario::core::StepInput autostart{};
    autostart.start_pressed = true;
    mario::core::step(state, autostart);
  }

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

    if (enemy_death_tick.size() != state.enemies.size()) {
      enemy_was_alive.assign(state.enemies.size(), true);
      enemy_death_tick.assign(state.enemies.size(), 0);
    }
    for (std::size_t i = 0; i < state.enemies.size(); ++i) {
      const bool alive = state.enemies[i].alive;
      if (enemy_was_alive[i] && !alive) {
        enemy_death_tick[i] = state.tick;
      }
      enemy_was_alive[i] = alive;
    }

    int w_px = 0;
    int h_px = 0;
    SDL_GetRendererOutputSize(renderer, &w_px, &h_px);

    const mario::core::units_t screen_w =
        static_cast<mario::core::units_t>(w_px) * mario::core::kPosScale;
    const mario::core::units_t screen_h =
        static_cast<mario::core::units_t>(h_px) * mario::core::kPosScale;
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

    const bool has_sprites = player_sheet.has_value() && enemy_sheet.has_value() &&
                             icons_sheet.has_value() && tiles_sheet.has_value();

    if (has_sprites) {
      const mario::core::units_t tile = state.config.tile_size;
      const int tile_px = units_to_px(tile);
      const int grass_top_a = 0;   // row0 col0
      const int grass_top_b = 1;   // row0 col1
      const int dirt_mid_a = 8;    // row1 col0
      const int dirt_mid_b = 9;    // row1 col1

      // Solids (tilemap).
      for (int row = 0; row < state.world.height; ++row) {
        for (int col = 0; col < state.world.width; ++col) {
          if (!state.world.is_solid_tile(col, row)) {
            continue;
          }

          const bool solid_above = state.world.is_solid_tile(col, row - 1);
          const bool variant = ((col + row) % 2) == 0;
          const int tile_frame = solid_above ? (variant ? dirt_mid_a : dirt_mid_b)
                                             : (variant ? grass_top_a : grass_top_b);

          const mario::core::units_t world_x = static_cast<mario::core::units_t>(col) * tile;
          const mario::core::units_t world_y = static_cast<mario::core::units_t>(row) * tile;
          SDL_Rect dst{
              world_to_screen_x(world_x, cam_top_left),
              world_to_screen_y(world_y, cam_top_left),
              tile_px,
              tile_px,
          };
          const SDL_Rect src = tiles_sheet->src_rect(tile_frame);
          SDL_RenderCopy(renderer, tiles_sheet->texture.handle, &src, &dst);
        }
      }

      // Goal flag + pole.
      {
        const mario::core::Rect goal_rect = state.world.goal_trigger_rect(state.config);
        const SDL_Rect pole = to_screen_rect(goal_rect, cam_top_left);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(renderer, &pole);

        const int flag_frame = 11;  // icons row2 col3
        const int flag_px = tile_px;
        SDL_Rect flag_dst{
            pole.x - flag_px / 2,
            pole.y,
            flag_px,
            flag_px,
        };
        const SDL_Rect flag_src = icons_sheet->src_rect(flag_frame);
        SDL_RenderCopy(renderer, icons_sheet->texture.handle, &flag_src, &flag_dst);
      }

      // Coins.
      const int coin_frame = anim_frame(state.tick, 4, 10);  // 6 fps
      constexpr int coin_px = 16;
      for (const mario::core::Vec2 c : state.world.coins) {
        const int cx = world_to_screen_x(c.x, cam_top_left);
        const int cy = world_to_screen_y(c.y, cam_top_left);
        SDL_Rect dst{cx - coin_px / 2, cy - coin_px / 2, coin_px, coin_px};
        const SDL_Rect src = icons_sheet->src_rect(coin_frame);
        SDL_RenderCopy(renderer, icons_sheet->texture.handle, &src, &dst);
      }

      // Mushrooms (power-ups).
      const int power_frame = 4 + anim_frame(state.tick, 4, 10);
      for (const mario::core::Vec2 m : state.world.mushrooms) {
        const int mx = world_to_screen_x(m.x, cam_top_left);
        const int my = world_to_screen_y(m.y, cam_top_left);
        SDL_Rect dst{mx, my, units_to_px(state.config.mushroom_size.x),
                     units_to_px(state.config.mushroom_size.y)};
        const SDL_Rect src = icons_sheet->src_rect(power_frame);
        SDL_RenderCopy(renderer, icons_sheet->texture.handle, &src, &dst);
      }

      // Enemies.
      for (std::size_t i = 0; i < state.enemies.size(); ++i) {
        const mario::core::Enemy& enemy = state.enemies[i];
        int frame = 0;
        bool draw = false;

        if (enemy.alive) {
          frame = anim_frame(state.tick + i * 3, 4, 12);  // walk0..walk3
          draw = true;
        } else if (enemy_death_tick[i] != 0 && state.tick - enemy_death_tick[i] <= 24) {
          frame = 4 + anim_frame(state.tick + i * 3, 2, 6);  // squish0..squish1
          draw = true;
        }

        if (draw) {
          const mario::core::Rect er = enemy.rect();
          constexpr int sprite_px = 32;
          const int ex = world_to_screen_x(er.x + (er.w - mario::core::px_to_units(sprite_px)) / 2,
                                           cam_top_left);
          const int ey =
              world_to_screen_y(er.y + er.h - mario::core::px_to_units(sprite_px), cam_top_left);
          SDL_Rect dst{ex, ey, sprite_px, sprite_px};
          const SDL_Rect src = enemy_sheet->src_rect(frame);
          const SDL_RendererFlip flip =
              (enemy.dir < 0) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
          SDL_RenderCopyEx(renderer, enemy_sheet->texture.handle, &src, &dst, 0.0, nullptr, flip);
        }
      }

      // Player.
      const bool blink =
          state.player.is_invulnerable() && ((state.tick / 4) % 2u == 0u);
      if (!blink) {
        int frame = 0;
        if (state.phase == mario::core::Phase::LevelComplete) {
          frame = 9;  // celebrate
        } else if (state.player.is_invulnerable()) {
          frame = 8;  // hurt
        } else if (!state.player.on_ground) {
          frame = (state.player.vel.y < 0) ? 6 : 7;  // jump/fall
        } else if (state.player.vel.x != 0) {
          frame = 2 + anim_frame(state.tick, 4, 6);  // run0..run3
        } else {
          const int idle = anim_frame(state.tick, 2, 20);
          frame = state.player.powered ? (10 + idle) : idle;  // idle or idle_alt
        }

        const mario::core::Rect pr = state.player.rect();
        constexpr int sprite_px = 32;
        const int px = world_to_screen_x(pr.x + (pr.w - mario::core::px_to_units(sprite_px)) / 2,
                                         cam_top_left);
        const int py =
            world_to_screen_y(pr.y + pr.h - mario::core::px_to_units(sprite_px), cam_top_left);
        SDL_Rect dst{px, py, sprite_px, sprite_px};
        const SDL_Rect src = player_sheet->src_rect(frame);
        const SDL_RendererFlip flip =
            (state.player.facing < 0) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
        SDL_RenderCopyEx(renderer, player_sheet->texture.handle, &src, &dst, 0.0, nullptr, flip);
      }
    } else {
      // Fallback debug shapes.

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
        const mario::core::Rect r{m.x, m.y, state.config.mushroom_size.x,
                                  state.config.mushroom_size.y};
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
    }

    SDL_RenderPresent(renderer);

    // Simple HUD via window title (no font dependency).
    std::string title;
    switch (state.phase) {
      case mario::core::Phase::Title:
        title = "mario-cpp | Press Enter to start";
        break;
      case mario::core::Phase::Playing:
        title = "mario-cpp | score=" + std::to_string(state.score) +
                " | high=" + std::to_string(state.high_score) + " | Esc=title";
        break;
      case mario::core::Phase::LevelComplete:
        title = "mario-cpp | Level complete! R=restart Esc=title | score=" + std::to_string(state.score) +
                " | high=" + std::to_string(state.high_score);
        break;
    }
    SDL_SetWindowTitle(window, title.c_str());
  }

  // Destroy textures before destroying the renderer.
  player_sheet.reset();
  enemy_sheet.reset();
  icons_sheet.reset();
  tiles_sheet.reset();

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  IMG_Quit();
  SDL_Quit();
  return 0;
}
