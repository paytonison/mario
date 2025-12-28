#include "mario/core/game_state.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>

#include "mario/core/constants.hpp"
#include "mario/core/physics.hpp"
#include "mario/core/types.hpp"

namespace mario::core {
namespace {

constexpr std::uint64_t kFnvOffsetBasis = 14695981039346656037ull;
constexpr std::uint64_t kFnvPrime = 1099511628211ull;

void fnv1a_u64_le(std::uint64_t& h, std::uint64_t v) {
  for (int i = 0; i < 8; ++i) {
    h ^= static_cast<std::uint8_t>((v >> (i * 8)) & 0xffu);
    h *= kFnvPrime;
  }
}

void fnv1a_i64_le(std::uint64_t& h, std::int64_t v) {
  fnv1a_u64_le(h, static_cast<std::uint64_t>(v));
}

void fnv1a_u32_le(std::uint64_t& h, std::uint32_t v) { fnv1a_u64_le(h, v); }

void fnv1a_bool(std::uint64_t& h, bool v) { fnv1a_u64_le(h, v ? 1u : 0u); }

std::uint32_t saturating_add_u32(std::uint32_t a, std::uint32_t b) {
  const std::uint64_t sum = static_cast<std::uint64_t>(a) + static_cast<std::uint64_t>(b);
  if (sum > std::numeric_limits<std::uint32_t>::max()) {
    return std::numeric_limits<std::uint32_t>::max();
  }
  return static_cast<std::uint32_t>(sum);
}

void reset_level(GameState& state) {
  state.player.reset(state.world.player_spawn, state.config);
  state.world.coins = state.coin_spawns;
  state.world.mushrooms = state.mushroom_spawns;

  const std::size_t count = std::min(state.enemies.size(), state.world.enemy_spawns.size());
  for (std::size_t i = 0; i < count; ++i) {
    state.enemies[i].reset(state.world.enemy_spawns[i], state.world, state.config);
  }
}

void restart_run(GameState& state) {
  state.score = 0;
  reset_level(state);
}

void player_died(GameState& state) {
  state.score = 0;
  reset_level(state);
}

void add_score(GameState& state, std::uint32_t points) {
  state.score = saturating_add_u32(state.score, points);
  state.high_score = std::max(state.high_score, state.score);
}

std::uint32_t collect_coins(GameState& state) {
  const Rect player_rect = state.player.rect();
  const units_t radius = state.config.tile_size / 5;  // tile * 0.2
  const units_t size = radius * 2;

  std::uint32_t collected = 0;
  std::vector<Vec2> kept;
  kept.reserve(state.world.coins.size());

  for (const Vec2 coin : state.world.coins) {
    const Rect coin_rect{coin.x - radius, coin.y - radius, size, size};
    const bool hit = physics::rects_intersect(player_rect, coin_rect);
    if (hit) {
      collected += 1;
    } else {
      kept.push_back(coin);
    }
  }
  state.world.coins = std::move(kept);

  if (collected > 0) {
    add_score(state, collected * 200);
  }
  return collected;
}

std::uint32_t collect_mushrooms(GameState& state) {
  const Rect player_rect = state.player.rect();
  const Vec2 size = state.config.mushroom_size;

  std::uint32_t collected = 0;
  std::vector<Vec2> kept;
  kept.reserve(state.world.mushrooms.size());

  for (const Vec2 pos : state.world.mushrooms) {
    const Rect mushroom_rect{pos.x, pos.y, size.x, size.y};
    const bool hit = physics::rects_intersect(player_rect, mushroom_rect);
    if (hit) {
      collected += 1;
    } else {
      kept.push_back(pos);
    }
  }
  state.world.mushrooms = std::move(kept);

  if (collected > 0) {
    state.player.powered = true;
    add_score(state, collected * 1000);
  }
  return collected;
}

void handle_player_enemy_collisions(GameState& state) {
  const Rect player_rect = state.player.rect();
  const units_t player_bottom = player_rect.y + player_rect.h;

  std::optional<std::size_t> stomped_index;
  std::optional<int> power_down_dir;
  bool died = false;

  for (std::size_t i = 0; i < state.enemies.size(); ++i) {
    const Enemy& enemy = state.enemies[i];
    if (!enemy.alive) {
      continue;
    }

    const Rect enemy_rect = enemy.rect();
    if (!physics::rects_intersect(player_rect, enemy_rect)) {
      continue;
    }

    const units_t stomp_threshold = enemy_rect.y + px_to_units(6);
    if (state.player.vel.y > 0 && player_bottom <= stomp_threshold) {
      stomped_index = i;
    } else if (state.player.is_invulnerable()) {
      // Ignore side hits while invulnerable.
    } else if (state.player.powered) {
      const units_t player_center_x = player_rect.x + player_rect.w / 2;
      const units_t enemy_center_x = enemy_rect.x + enemy_rect.w / 2;
      const int dir = enemy_center_x < player_center_x ? 1 : -1;
      power_down_dir = dir;
    } else {
      died = true;
    }
    break;
  }

  if (stomped_index.has_value()) {
    state.enemies[*stomped_index].alive = false;
    state.player.vel.y = -state.config.stomp_bounce;
    add_score(state, 100);
  } else if (power_down_dir.has_value()) {
    const int dir = *power_down_dir;
    state.player.powered = false;
    state.player.invuln_timer = std::max<std::int32_t>(0, state.config.hurt_invuln_time);
    state.player.vel.x = static_cast<units_t>(dir) * state.config.hurt_knockback_x;
    state.player.vel.y = -state.config.hurt_knockback_y;
    state.player.pos.x += static_cast<units_t>(dir) * px_to_units(4);
    state.player.on_ground = false;
  } else if (died) {
    player_died(state);
  }
}

void check_goal(GameState& state) {
  const Rect goal_rect = state.world.goal_trigger_rect(state.config);
  if (physics::rects_intersect(state.player.rect(), goal_rect)) {
    add_score(state, 500);
    state.phase = Phase::LevelComplete;
  }
}

void check_fall_off(GameState& state) {
  const units_t fall_limit = static_cast<units_t>(state.world.height) * state.config.tile_size +
                             px_to_units(200);
  if (state.player.pos.y > fall_limit) {
    player_died(state);
  }
}

}  // namespace

GameState make_new_game(World world, const Config& config) {
  GameState state{};
  state.phase = Phase::Title;
  state.tick = 0;
  state.config = config;
  state.world = std::move(world);

  state.player.reset(state.world.player_spawn, state.config);

  state.enemies.clear();
  state.enemies.reserve(state.world.enemy_spawns.size());
  for (const Vec2 spawn : state.world.enemy_spawns) {
    Enemy e{};
    e.reset(spawn, state.world, state.config);
    state.enemies.push_back(e);
  }

  state.coin_spawns = state.world.coins;
  state.mushroom_spawns = state.world.mushrooms;
  state.score = 0;
  state.high_score = 0;
  return state;
}

void step(GameState& state, const StepInput& input) {
  state.tick += 1;

  switch (state.phase) {
    case Phase::Title:
      if (input.start_pressed) {
        state.phase = Phase::Playing;
        restart_run(state);
      }
      break;

    case Phase::Playing: {
      if (input.quit_pressed) {
        state.phase = Phase::Title;
        return;
      }
      if (input.restart_pressed) {
        restart_run(state);
        return;
      }

      state.player.update(input, state.world, state.config);
      for (Enemy& enemy : state.enemies) {
        enemy.update(state.world, state.config);
      }

      collect_coins(state);
      collect_mushrooms(state);
      handle_player_enemy_collisions(state);
      check_goal(state);
      check_fall_off(state);
    } break;

    case Phase::LevelComplete:
      if (input.quit_pressed) {
        state.phase = Phase::Title;
        return;
      }
      if (input.restart_pressed) {
        restart_run(state);
        state.phase = Phase::Playing;
      }
      break;
  }
}

std::uint64_t hash_state(const GameState& state) {
  std::uint64_t h = kFnvOffsetBasis;

  fnv1a_i64_le(h, state.config.tile_size);
  fnv1a_i64_le(h, state.config.player_size.x);
  fnv1a_i64_le(h, state.config.player_size.y);
  fnv1a_i64_le(h, state.config.enemy_size.x);
  fnv1a_i64_le(h, state.config.enemy_size.y);
  fnv1a_i64_le(h, state.config.mushroom_size.x);
  fnv1a_i64_le(h, state.config.mushroom_size.y);

  fnv1a_i64_le(h, state.config.move_speed);
  fnv1a_i64_le(h, state.config.move_accel);
  fnv1a_i64_le(h, state.config.move_decel);
  fnv1a_i64_le(h, state.config.gravity);
  fnv1a_i64_le(h, state.config.terminal_velocity);
  fnv1a_i64_le(h, state.config.jump_speed);
  fnv1a_i64_le(h, state.config.stomp_bounce);
  fnv1a_i64_le(h, state.config.enemy_speed);
  fnv1a_i64_le(h, state.config.hurt_knockback_x);
  fnv1a_i64_le(h, state.config.hurt_knockback_y);
  fnv1a_u64_le(h, static_cast<std::uint64_t>(state.config.coyote_time));
  fnv1a_u64_le(h, static_cast<std::uint64_t>(state.config.jump_buffer_time));
  fnv1a_u64_le(h, static_cast<std::uint64_t>(state.config.hurt_invuln_time));

  fnv1a_u64_le(h, static_cast<std::uint64_t>(state.phase));
  fnv1a_u64_le(h, state.tick);

  fnv1a_u32_le(h, state.score);
  fnv1a_u32_le(h, state.high_score);

  fnv1a_i64_le(h, state.player.pos.x);
  fnv1a_i64_le(h, state.player.pos.y);
  fnv1a_i64_le(h, state.player.vel.x);
  fnv1a_i64_le(h, state.player.vel.y);
  fnv1a_bool(h, state.player.on_ground);
  fnv1a_u64_le(h, static_cast<std::uint64_t>(state.player.facing));
  fnv1a_u64_le(h, static_cast<std::uint64_t>(state.player.coyote_timer));
  fnv1a_u64_le(h, static_cast<std::uint64_t>(state.player.jump_buffer_timer));
  fnv1a_bool(h, state.player.powered);
  fnv1a_u64_le(h, static_cast<std::uint64_t>(state.player.invuln_timer));

  fnv1a_u64_le(h, static_cast<std::uint64_t>(state.world.width));
  fnv1a_u64_le(h, static_cast<std::uint64_t>(state.world.height));
  fnv1a_i64_le(h, state.world.player_spawn.x);
  fnv1a_i64_le(h, state.world.player_spawn.y);
  fnv1a_i64_le(h, state.world.goal_tile.x);
  fnv1a_i64_le(h, state.world.goal_tile.y);

  fnv1a_u64_le(h, static_cast<std::uint64_t>(state.world.coins.size()));
  for (const Vec2 c : state.world.coins) {
    fnv1a_i64_le(h, c.x);
    fnv1a_i64_le(h, c.y);
  }

  fnv1a_u64_le(h, static_cast<std::uint64_t>(state.world.mushrooms.size()));
  for (const Vec2 m : state.world.mushrooms) {
    fnv1a_i64_le(h, m.x);
    fnv1a_i64_le(h, m.y);
  }

  fnv1a_u64_le(h, static_cast<std::uint64_t>(state.world.enemy_spawns.size()));
  for (const Vec2 spawn : state.world.enemy_spawns) {
    fnv1a_i64_le(h, spawn.x);
    fnv1a_i64_le(h, spawn.y);
  }

  fnv1a_u64_le(h, static_cast<std::uint64_t>(state.world.solid_tiles.size()));
  for (const std::uint8_t t : state.world.solid_tiles) {
    fnv1a_u64_le(h, t);
  }

  fnv1a_u64_le(h, static_cast<std::uint64_t>(state.coin_spawns.size()));
  for (const Vec2 c : state.coin_spawns) {
    fnv1a_i64_le(h, c.x);
    fnv1a_i64_le(h, c.y);
  }

  fnv1a_u64_le(h, static_cast<std::uint64_t>(state.mushroom_spawns.size()));
  for (const Vec2 m : state.mushroom_spawns) {
    fnv1a_i64_le(h, m.x);
    fnv1a_i64_le(h, m.y);
  }

  fnv1a_u64_le(h, static_cast<std::uint64_t>(state.enemies.size()));
  for (const Enemy& e : state.enemies) {
    fnv1a_i64_le(h, e.pos.x);
    fnv1a_i64_le(h, e.pos.y);
    fnv1a_i64_le(h, e.vel.x);
    fnv1a_i64_le(h, e.vel.y);
    fnv1a_u64_le(h, static_cast<std::uint64_t>(e.dir));
    fnv1a_bool(h, e.alive);
    fnv1a_bool(h, e.on_ground);
  }

  return h;
}

}  // namespace mario::core
