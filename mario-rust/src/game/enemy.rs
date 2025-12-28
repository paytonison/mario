use macroquad::prelude::*;

use super::{physics, sprites::Sprites, world::World, Config};

#[derive(Clone)]
pub struct Enemy {
    pub pos: Vec2,
    pub vel: Vec2,
    dir: f32,
    pub alive: bool,
    size: Vec2,
    on_ground: bool,
}

impl Enemy {
    pub fn new(tile_pos: Vec2, world: &World, config: &Config) -> Self {
        let size = config.enemy_size;
        let tile = config.tile_size;
        let x = tile_pos.x + (tile - size.x) * 0.5;
        let sample_x = tile_pos.x + tile * 0.5;
        let base_y = world
            .ground_y_for_x(sample_x, tile_pos.y, config)
            .unwrap_or(tile_pos.y + tile);
        let y = base_y - size.y;

        Self {
            pos: vec2(x, y),
            vel: Vec2::ZERO,
            dir: -1.0,
            alive: true,
            size,
            on_ground: false,
        }
    }

    pub fn reset(&mut self, tile_pos: Vec2, world: &World, config: &Config) {
        *self = Self::new(tile_pos, world, config);
    }

    pub fn update(&mut self, world: &World, config: &Config, dt: f32) {
        if !self.alive {
            return;
        }

        self.vel.y = (self.vel.y + config.gravity * dt).min(config.terminal_velocity);
        self.vel.x = config.enemy_speed * self.dir;

        let desired_x = self.vel.x;
        let (pos, vel, on_ground) =
            physics::move_with_collisions(self.pos, self.size, self.vel, &world.solids, dt);

        let hit_wall = desired_x.abs() > f32::EPSILON && vel.x.abs() <= f32::EPSILON;
        self.pos = pos;
        self.vel = vel;
        self.on_ground = on_ground;

        if hit_wall {
            self.dir *= -1.0;
            self.vel.x = config.enemy_speed * self.dir;
        } else if self.on_ground {
            let foot_x = if self.dir >= 0.0 {
                self.pos.x + self.size.x + 1.0
            } else {
                self.pos.x - 1.0
            };
            let foot_y = self.pos.y + self.size.y + 1.0;
            if world
                .ground_y_for_x(foot_x, foot_y, config)
                .map(|ground_y| ground_y <= foot_y)
                != Some(true)
            {
                self.dir *= -1.0;
                self.vel.x = config.enemy_speed * self.dir;
            }
        }

        let world_w = world.width as f32 * config.tile_size;
        if self.pos.x <= 0.0 {
            self.pos.x = 0.0;
            self.dir = 1.0;
        } else if self.pos.x + self.size.x >= world_w {
            self.pos.x = (world_w - self.size.x).max(0.0);
            self.dir = -1.0;
        }
    }

    pub fn rect(&self) -> Rect {
        physics::rect_at(self.pos, self.size)
    }

    pub fn draw(&self, sprites: &Sprites) {
        if !self.alive {
            return;
        }

        draw_texture_ex(
            sprites.chestnut_guy(),
            self.pos.x,
            self.pos.y,
            WHITE,
            DrawTextureParams {
                dest_size: Some(self.size),
                flip_x: self.vel.x < 0.0,
                ..Default::default()
            },
        );
    }
}
