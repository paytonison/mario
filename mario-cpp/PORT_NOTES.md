# Mario C++ Port Notes (from `mario-rust/`)

This document captures the **behavioral spec** and a direct mapping from the Rust implementation to the C++ layout in `mario-cpp/`.

## Rust → C++ module mapping

- `mario-rust/src/game/mod.rs`
  - `Config` → `mario-cpp/core/include/mario/core/config.hpp` (`mario::core::Config`)
  - `Game` + `GameState` → `mario-cpp/core/include/mario/core/game_state.hpp` (`mario::core::GameState`, `mario::core::Phase`)
  - fixed-step loop + input capture → C++ uses **tick-based** `StepInput` and `step(GameState&, StepInput)`; app layer runs the loop
- `mario-rust/src/game/physics.rs`
  - `Rect`, `rects_intersect`, `approach`, `move_with_collisions` → `mario-cpp/core/include/mario/core/physics.hpp`
- `mario-rust/src/game/world.rs`
  - ASCII level parsing + collision solids + helper queries → `mario-cpp/core/include/mario/core/world.hpp`
- `mario-rust/src/game/player.rs`
  - player movement + jump buffer/coyote time + power-up + invulnerability timer → `mario-cpp/core/include/mario/core/player.hpp`
- `mario-rust/src/game/enemy.rs`
  - chestnut guy movement + wall/ledge turning → `mario-cpp/core/include/mario/core/enemy.hpp`
- Rendering/audio-only Rust modules:
  - `sprites.rs`, `background.rs`, `audio.rs` → **app layer only** (`mario-cpp/app/`); core emits optional events but has no SDL/audio deps.

## Determinism decisions in the C++ port

- The core sim runs at a fixed tick rate: **60 Hz** (one `step()` == one tick).
- Positions/velocities use deterministic **fixed-point integers**:
  - `POS_SCALE = 3600` units per pixel.
  - Positions are `px * POS_SCALE`.
  - Velocities are `px_per_tick * POS_SCALE`.
- Timers use deterministic integer time units:
  - `TIME_SCALE = 600` units per second (`dt = 10` units per tick).
- Stable end-state hash: hash only deterministic scalar fields + vectors in stable order; no pointers or timestamps.

## Physics + collision rules (copied from Rust)

- Collision is **axis-separated AABB vs. solid tile AABBs**:
  - Move X, resolve overlaps, zero `vel.x` on hit.
  - Move Y, resolve overlaps, zero `vel.y` on hit.
  - `on_ground = true` only when resolving a **downward** collision.
- No slopes; no one-way platforms.
- Intersection test matches Rust’s strict inequalities:
  - `a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h && a.y + a.h > b.y`

## Player behavior (copied from Rust)

- Horizontal movement:
  - `target_speed = move_x * move_speed`
  - accelerate toward target with `move_accel` when input present, else `move_decel`
  - uses `approach(value, target, delta)`
- Jump behavior:
  - Jump buffer (`jump_buffer_time`): pressing jump sets buffer timer.
  - Coyote time (`coyote_time`): while grounded, coyote timer is refreshed.
  - If `jump_buffer_timer > 0` and `coyote_timer > 0` → jump starts.
  - Jump cut: releasing jump while moving upward halves vertical velocity (`jump_cut_multiplier = 0.5`).
  - After movement+collision, if `jump_buffer_timer > 0` and landed → jump immediately (buffer-on-land).
- Gravity:
  - `vel.y += gravity`
  - clamp to `terminal_velocity`
- Power-up + invulnerability:
  - Mushroom sets `powered = true`.
  - If powered and hit by enemy: lose power, get knockback, start invulnerability timer.
  - While invulnerable: side hits with enemies are ignored.

## Enemy behavior (copied from Rust)

- A “goomba-like” chestnut guy:
  - Constant horizontal speed `enemy_speed` in `dir` (starts `-1`).
  - Gravity + terminal velocity like player.
  - Turn around on wall hit (x-collision that zeroes `vel.x`).
  - Turn around at ledges (sample “foot” point one pixel ahead; if ground below is lower/missing, reverse).
  - Clamp within world X bounds; reverse direction at edges.
- Stomp rule:
  - If player intersects enemy and `player.vel.y > 0` and player bottom is above `enemy_rect.y + 6px` → stomp.
  - Stomp kills enemy, bounces player upward (`stomp_bounce`), awards points.

## Level format + representation (copied from Rust)

ASCII grid (`assets/levels/level1.txt`):

- `#` = solid tile
- `.` = empty
- `P` = player spawn (exactly one)
- `G` = goal tile (exactly one)
- `E` = enemy spawn
- `C` = coin
- `M` = mushroom

World parsing rules:

- `#` tiles become solid AABBs of size `tile_size`.
- Coins are stored as **centers** of their tile (for collision they’re treated as a small square).
- Enemies/mushrooms are positioned by:
  - centering in the tile (X)
  - placing their base on the first solid tile below the tile (Y); if none found, pretend ground is at `tile_y + tile_size`.

## Game loop + scoring (copied from Rust)

- Phases: `Title` → `Playing` → `LevelComplete`.
  - `Enter` starts.
  - `R` restarts.
  - `Esc` returns to title.
- Score:
  - coin: `+200`
  - stomp enemy: `+100`
  - goal / flagpole: `+500`
  - mushroom: `+1000`
- Falling off bottom:
  - if `player.pos.y > (world_h + 200px)` → death/reset.

## Constants copied from Rust `Config::default()`

All values below are **Rust source values** (`mario-rust/src/game/mod.rs:Config`):

- `fixed_dt`: `1/60`
- `max_frame_time`: `0.25` (app-only; core is tick-based)
- `tile_size`: `32`
- `player_size`: `22x28`
- `move_speed`: `220`
- `move_accel`: `1600`
- `move_decel`: `2000`
- `gravity`: `1200`
- `terminal_velocity`: `780`
- `jump_speed`: `420`
- `coyote_time`: `0.1`
- `jump_buffer_time`: `0.12`
- `jump_cut_multiplier`: `0.5`
- `stomp_bounce`: `320`
- `enemy_size`: `24x20`
- `enemy_speed`: `65`
- `mushroom_size`: `24x22`
- `hurt_invuln_time`: `0.75`
- `hurt_knockback_x`: `200`
- `hurt_knockback_y`: `260`

