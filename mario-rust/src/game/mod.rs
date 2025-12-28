mod audio;
mod background;
mod enemy;
mod physics;
mod player;
mod sprites;
mod world;

use macroquad::file::set_pc_assets_folder;
use macroquad::prelude::*;

use self::audio::Sfx;
use self::enemy::Enemy;
use self::player::Player;
use self::sprites::Sprites;
use self::world::World;

#[derive(Clone, Copy)]
pub struct Config {
    pub fixed_dt: f32,
    pub max_frame_time: f32,
    pub tile_size: f32,
    pub player_size: Vec2,
    pub move_speed: f32,
    pub move_accel: f32,
    pub move_decel: f32,
    pub gravity: f32,
    pub terminal_velocity: f32,
    pub jump_speed: f32,
    pub coyote_time: f32,
    pub jump_buffer_time: f32,
    pub jump_cut_multiplier: f32,
    pub stomp_bounce: f32,
    pub enemy_size: Vec2,
    pub enemy_speed: f32,
    pub mushroom_size: Vec2,
    pub hurt_invuln_time: f32,
    pub hurt_knockback_x: f32,
    pub hurt_knockback_y: f32,
}

impl Default for Config {
    fn default() -> Self {
        Self {
            fixed_dt: 1.0 / 60.0,
            max_frame_time: 0.25,
            tile_size: 32.0,
            player_size: vec2(22.0, 28.0),
            move_speed: 220.0,
            move_accel: 1600.0,
            move_decel: 2000.0,
            gravity: 1200.0,
            terminal_velocity: 780.0,
            jump_speed: 420.0,
            coyote_time: 0.1,
            jump_buffer_time: 0.12,
            jump_cut_multiplier: 0.5,
            stomp_bounce: 320.0,
            enemy_size: vec2(24.0, 20.0),
            enemy_speed: 65.0,
            mushroom_size: vec2(24.0, 22.0),
            hurt_invuln_time: 0.75,
            hurt_knockback_x: 200.0,
            hurt_knockback_y: 260.0,
        }
    }
}

pub struct Game {
    state: GameState,
    accumulator: f32,
    config: Config,
    sfx: Sfx,
    sprites: Sprites,
    world: World,
    player: Player,
    enemies: Vec<Enemy>,
    coin_spawns: Vec<Vec2>,
    mushroom_spawns: Vec<Vec2>,
    score: u32,
    high_score: u32,
    input: InputState,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
enum GameState {
    Title,
    Playing,
    LevelComplete,
}

#[derive(Clone, Copy, Default)]
pub struct InputState {
    pub move_x: f32,
    pub jump_pressed: bool,
    pub jump_released: bool,
    pub start_pressed: bool,
    pub restart_pressed: bool,
    pub quit_pressed: bool,
}

impl Game {
    pub async fn new() -> Self {
        set_pc_assets_folder("assets");
        let config = Config::default();
        let sfx = Sfx::new().await;
        let sprites = Sprites::new();
        let world = World::load("levels/level1.txt", &config).await;
        let player = Player::new(world.player_spawn, &config);
        let enemies = world
            .enemy_spawns
            .iter()
            .copied()
            .map(|spawn| Enemy::new(spawn, &world, &config))
            .collect();
        let coin_spawns = world.coins.clone();
        let mushroom_spawns = world.mushrooms.clone();

        Self {
            state: GameState::Title,
            accumulator: 0.0,
            config,
            sfx,
            sprites,
            world,
            player,
            enemies,
            coin_spawns,
            mushroom_spawns,
            score: 0,
            high_score: 0,
            input: InputState::default(),
        }
    }

    pub fn update(&mut self, frame_dt: f32) {
        self.capture_input();
        self.accumulator += frame_dt.min(self.config.max_frame_time);

        while self.accumulator >= self.config.fixed_dt {
            let input = self.consume_fixed_input();
            self.fixed_update(input);
            self.accumulator -= self.config.fixed_dt;
        }
    }

    pub fn draw(&self) {
        clear_background(Color::new(0.45, 0.75, 0.95, 1.0));

        match self.state {
            GameState::Title => draw_title(),
            GameState::Playing => self.draw_playing(),
            GameState::LevelComplete => self.draw_level_complete(),
        }
    }

    fn fixed_update(&mut self, input: InputState) {
        match self.state {
            GameState::Title => {
                if input.start_pressed {
                    self.state = GameState::Playing;
                    self.restart_run();
                    self.sfx.start_music();
                }
            }
            GameState::Playing => {
                if input.quit_pressed {
                    self.sfx.stop_music();
                    self.state = GameState::Title;
                    return;
                }

                if input.restart_pressed {
                    self.restart_run();
                    self.sfx.start_music();
                    return;
                }

                let jumped =
                    self.player
                        .update(&input, &self.world, &self.config, self.config.fixed_dt);
                if jumped {
                    self.sfx.play_jump();
                }

                for enemy in &mut self.enemies {
                    enemy.update(&self.world, &self.config, self.config.fixed_dt);
                }

                if self.collect_coins() > 0 {
                    self.sfx.play_coin();
                }
                if self.collect_mushrooms() > 0 {
                    self.sfx.play_powerup();
                }
                self.handle_player_enemy_collisions();
                self.check_goal();
                self.check_fall_off();
            }
            GameState::LevelComplete => {
                if input.quit_pressed {
                    self.sfx.stop_music();
                    self.state = GameState::Title;
                    return;
                }

                if input.restart_pressed {
                    self.restart_run();
                    self.state = GameState::Playing;
                    self.sfx.start_music();
                }
            }
        }
    }

    fn draw_playing(&self) {
        let camera = self
            .world
            .camera_for_focus(self.player.center(), &self.config);
        set_camera(&camera);

        background::draw(&camera, &self.world, &self.config);
        self.world.draw(&self.config);

        for enemy in &self.enemies {
            enemy.draw(&self.sprites);
        }

        let player_size = self.player.size();
        let player_pos = self.player.pos;
        let texture = self.sprites.player(self.player.is_powered());
        let flip_x = self.player.facing_dir() < 0.0;
        let mut tint = WHITE;
        if self.player.is_invulnerable() && (get_time() * 12.0) as i32 % 2 == 0 {
            tint.a = 0.35;
        }
        draw_texture_ex(
            texture,
            player_pos.x,
            player_pos.y,
            tint,
            DrawTextureParams {
                dest_size: Some(player_size),
                flip_x,
                ..Default::default()
            },
        );

        set_default_camera();
        draw_hud(self.high_score, self.score);
    }

    fn draw_level_complete(&self) {
        set_default_camera();
        draw_hud(self.high_score, self.score);
        draw_centered_text("Course Complete! Press R to restart.", 48.0, BLACK);
    }

    fn reset_level(&mut self) {
        self.player.reset(self.world.player_spawn, &self.config);
        self.world.coins = self.coin_spawns.clone();
        self.world.mushrooms = self.mushroom_spawns.clone();
        for (enemy, spawn) in self
            .enemies
            .iter_mut()
            .zip(self.world.enemy_spawns.iter().copied())
        {
            enemy.reset(spawn, &self.world, &self.config);
        }
    }

    fn restart_run(&mut self) {
        self.score = 0;
        self.reset_level();
    }

    fn player_died(&mut self) {
        self.sfx.play_hurt();
        self.score = 0;
        self.reset_level();
    }

    fn add_score(&mut self, points: u32) {
        self.score = self.score.saturating_add(points);
        self.high_score = self.high_score.max(self.score);
    }

    fn collect_coins(&mut self) -> u32 {
        let player_rect = self.player.rect();
        let radius = self.config.tile_size * 0.2;
        let size = radius * 2.0;
        let mut collected = 0u32;

        self.world.coins.retain(|coin| {
            let coin_rect = Rect::new(coin.x - radius, coin.y - radius, size, size);
            let hit = physics::rects_intersect(player_rect, coin_rect);
            if hit {
                collected += 1;
            }
            !hit
        });

        if collected > 0 {
            self.add_score(collected * 200);
        }

        collected
    }

    fn collect_mushrooms(&mut self) -> u32 {
        let player_rect = self.player.rect();
        let size = self.config.mushroom_size;
        let mut collected = 0u32;

        self.world.mushrooms.retain(|pos| {
            let mushroom_rect = Rect::new(pos.x, pos.y, size.x, size.y);
            let hit = physics::rects_intersect(player_rect, mushroom_rect);
            if hit {
                collected += 1;
            }
            !hit
        });

        if collected > 0 {
            self.player.set_powered(true);
            self.add_score(collected * 1000);
        }

        collected
    }

    fn handle_player_enemy_collisions(&mut self) {
        let player_rect = self.player.rect();
        let player_bottom = player_rect.y + player_rect.h;
        let mut stomped_index = None;
        let mut power_down_dir = None;
        let mut died = false;

        for (idx, enemy) in self.enemies.iter().enumerate() {
            if !enemy.alive {
                continue;
            }

            let enemy_rect = enemy.rect();
            if !physics::rects_intersect(player_rect, enemy_rect) {
                continue;
            }

            let stomp_threshold = enemy_rect.y + 6.0;
            if self.player.vel.y > 0.0 && player_bottom <= stomp_threshold {
                stomped_index = Some(idx);
            } else if self.player.is_invulnerable() {
                // Ignore side hits while invulnerable.
            } else if self.player.is_powered() {
                let player_center_x = player_rect.x + player_rect.w * 0.5;
                let enemy_center_x = enemy_rect.x + enemy_rect.w * 0.5;
                let dir = if enemy_center_x < player_center_x {
                    1.0
                } else {
                    -1.0
                };
                power_down_dir = Some(dir);
            } else {
                died = true;
            }
            break;
        }

        if let Some(idx) = stomped_index {
            if let Some(enemy) = self.enemies.get_mut(idx) {
                enemy.alive = false;
            }
            self.player.vel.y = -self.config.stomp_bounce;
            self.add_score(100);
            self.sfx.play_stomp();
        } else if let Some(dir) = power_down_dir {
            self.player.set_powered(false);
            self.player
                .start_invulnerability(self.config.hurt_invuln_time);
            self.player.vel.x = dir * self.config.hurt_knockback_x;
            self.player.vel.y = -self.config.hurt_knockback_y;
            self.player.pos.x += dir * 4.0;
            self.player.on_ground = false;
            self.sfx.play_hurt();
        } else if died {
            self.player_died();
        }
    }

    fn check_goal(&mut self) {
        let goal_rect = self.world.goal_trigger_rect(&self.config);
        if physics::rects_intersect(self.player.rect(), goal_rect) {
            self.add_score(500);
            self.sfx.stop_music();
            self.sfx.play_win();
            self.state = GameState::LevelComplete;
        }
    }

    fn check_fall_off(&mut self) {
        let fall_limit = self.world.height as f32 * self.config.tile_size + 200.0;
        if self.player.pos.y > fall_limit {
            self.player_died();
        }
    }

    fn capture_input(&mut self) {
        self.input.move_x = read_move_x();
        self.input.jump_pressed |= read_jump_pressed();
        self.input.jump_released |= read_jump_released();
        self.input.start_pressed |= is_key_pressed(KeyCode::Enter);
        self.input.restart_pressed |= is_key_pressed(KeyCode::R);
        self.input.quit_pressed |= is_key_pressed(KeyCode::Escape);
    }

    fn consume_fixed_input(&mut self) -> InputState {
        let snapshot = self.input;
        self.input.jump_pressed = false;
        self.input.jump_released = false;
        self.input.start_pressed = false;
        self.input.restart_pressed = false;
        self.input.quit_pressed = false;
        snapshot
    }
}

fn draw_title() {
    let title = "Rusty Platformer";
    let subtitle = "Press Enter to Start";

    let title_size = 56;
    let subtitle_size = 28;
    let title_dim = measure_text(title, None, title_size, 1.0);
    let subtitle_dim = measure_text(subtitle, None, subtitle_size, 1.0);

    let center_x = screen_width() * 0.5;
    let center_y = screen_height() * 0.5;

    draw_text(
        title,
        center_x - title_dim.width * 0.5,
        center_y - 20.0,
        title_size as f32,
        BLACK,
    );

    draw_text(
        subtitle,
        center_x - subtitle_dim.width * 0.5,
        center_y + 30.0,
        subtitle_size as f32,
        DARKGRAY,
    );
}

fn draw_hud(high_score: u32, score: u32) {
    let size = 26.0;
    draw_text(
        &format!("High Score: {high_score}"),
        16.0,
        30.0,
        size,
        BLACK,
    );
    draw_text(&format!("Score: {score}"), 16.0, 58.0, size, BLACK);
}

fn draw_centered_text(text: &str, font_size: f32, color: Color) {
    let dims = measure_text(text, None, font_size as u16, 1.0);
    draw_text(
        text,
        (screen_width() - dims.width) * 0.5,
        screen_height() * 0.5,
        font_size,
        color,
    );
}

fn read_move_x() -> f32 {
    let mut move_x = 0.0;
    if is_key_down(KeyCode::Left) || is_key_down(KeyCode::A) {
        move_x -= 1.0;
    }
    if is_key_down(KeyCode::Right) || is_key_down(KeyCode::D) {
        move_x += 1.0;
    }
    move_x
}

fn read_jump_pressed() -> bool {
    is_key_pressed(KeyCode::Space) || is_key_pressed(KeyCode::Up) || is_key_pressed(KeyCode::W)
}

fn read_jump_released() -> bool {
    is_key_released(KeyCode::Space) || is_key_released(KeyCode::Up) || is_key_released(KeyCode::W)
}
