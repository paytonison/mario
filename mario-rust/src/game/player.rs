use macroquad::prelude::*;

use super::{physics, world::World, Config, InputState};

pub struct Player {
    pub pos: Vec2,
    pub vel: Vec2,
    pub on_ground: bool,
    size: Vec2,
    facing: f32,
    coyote_timer: f32,
    jump_buffer_timer: f32,
    powered: bool,
    invuln_timer: f32,
}

impl Player {
    pub fn new(spawn: Vec2, config: &Config) -> Self {
        let size = config.player_size;
        let pos = spawn + vec2((config.tile_size - size.x) * 0.5, config.tile_size - size.y);

        Self {
            pos,
            vel: Vec2::ZERO,
            on_ground: false,
            size,
            facing: 1.0,
            coyote_timer: 0.0,
            jump_buffer_timer: 0.0,
            powered: false,
            invuln_timer: 0.0,
        }
    }

    pub fn reset(&mut self, spawn: Vec2, config: &Config) {
        let size = config.player_size;
        self.pos = spawn + vec2((config.tile_size - size.x) * 0.5, config.tile_size - size.y);
        self.vel = Vec2::ZERO;
        self.on_ground = false;
        self.facing = 1.0;
        self.coyote_timer = 0.0;
        self.jump_buffer_timer = 0.0;
        self.powered = false;
        self.invuln_timer = 0.0;
        self.size = size;
    }

    pub fn update(&mut self, input: &InputState, world: &World, config: &Config, dt: f32) -> bool {
        self.invuln_timer = (self.invuln_timer - dt).max(0.0);
        let mut jumped = false;
        if input.jump_pressed {
            self.jump_buffer_timer = config.jump_buffer_time;
        } else {
            self.jump_buffer_timer = (self.jump_buffer_timer - dt).max(0.0);
        }

        if input.jump_released && self.vel.y < 0.0 {
            self.vel.y *= config.jump_cut_multiplier;
        }

        if self.on_ground {
            self.coyote_timer = config.coyote_time;
        } else {
            self.coyote_timer = (self.coyote_timer - dt).max(0.0);
        }

        if input.move_x.abs() > f32::EPSILON {
            self.facing = input.move_x.signum();
        }

        let target_speed = input.move_x * config.move_speed;
        let accel = if input.move_x.abs() > f32::EPSILON {
            config.move_accel
        } else {
            config.move_decel
        };
        self.vel.x = physics::approach(self.vel.x, target_speed, accel * dt);

        if self.jump_buffer_timer > 0.0 && self.coyote_timer > 0.0 {
            self.vel.y = -config.jump_speed;
            self.on_ground = false;
            self.coyote_timer = 0.0;
            self.jump_buffer_timer = 0.0;
            jumped = true;
        }

        self.vel.y = (self.vel.y + config.gravity * dt).min(config.terminal_velocity);

        let (pos, vel, on_ground) =
            physics::move_with_collisions(self.pos, self.size, self.vel, &world.solids, dt);

        self.pos = pos;
        self.vel = vel;
        self.on_ground = on_ground;

        if self.jump_buffer_timer > 0.0 && self.on_ground {
            self.vel.y = -config.jump_speed;
            self.on_ground = false;
            self.coyote_timer = 0.0;
            self.jump_buffer_timer = 0.0;
            jumped = true;
        }

        jumped
    }

    pub fn size(&self) -> Vec2 {
        self.size
    }

    pub fn center(&self) -> Vec2 {
        self.pos + self.size * 0.5
    }

    pub fn rect(&self) -> Rect {
        physics::rect_at(self.pos, self.size)
    }

    pub fn facing_dir(&self) -> f32 {
        self.facing
    }

    pub fn is_powered(&self) -> bool {
        self.powered
    }

    pub fn set_powered(&mut self, powered: bool) {
        self.powered = powered;
    }

    pub fn is_invulnerable(&self) -> bool {
        self.invuln_timer > 0.0
    }

    pub fn start_invulnerability(&mut self, duration: f32) {
        self.invuln_timer = duration.max(0.0);
    }
}
