use macroquad::file::load_string;
use macroquad::prelude::*;

use super::{physics, Config};

const FALLBACK_LEVEL: &str = "\
................................\n\
................................\n\
................................\n\
................................\n\
.......C.........C.......C......\n\
......#####.....#####...#####...\n\
..P....M....E................G..\n\
#######...########..######...###\n";

pub struct World {
    pub solids: Vec<Rect>,
    solid_tiles: Vec<bool>,
    pub coins: Vec<Vec2>,
    pub mushrooms: Vec<Vec2>,
    pub enemy_spawns: Vec<Vec2>,
    pub player_spawn: Vec2,
    pub goal_tile: Vec2,
    pub width: usize,
    pub height: usize,
}

impl World {
    pub async fn load(path: &str, config: &Config) -> Self {
        match load_string(path).await {
            Ok(contents) => match Self::from_ascii(&contents, config) {
                Ok(world) => world,
                Err(error) => {
                    eprintln!("Level parse error: {error}. Using fallback level.");
                    Self::from_ascii(FALLBACK_LEVEL, config).expect("Fallback level is invalid")
                }
            },
            Err(error) => {
                eprintln!("Level load error: {error}. Using fallback level.");
                Self::from_ascii(FALLBACK_LEVEL, config).expect("Fallback level is invalid")
            }
        }
    }

    pub fn from_ascii(contents: &str, config: &Config) -> Result<Self, String> {
        let lines: Vec<&str> = contents
            .lines()
            .map(str::trim_end)
            .filter(|line| !line.is_empty())
            .collect();

        let height = lines.len();
        let width = lines
            .iter()
            .map(|line| line.chars().count())
            .max()
            .unwrap_or(0);

        if width == 0 || height == 0 {
            return Err("Level has no tiles".to_string());
        }

        let tile_size = config.tile_size;
        let mut solid_tiles = vec![false; width * height];
        let mut solids = Vec::new();
        let mut coins = Vec::new();
        let mut mushroom_tiles = Vec::new();
        let mut enemy_spawns = Vec::new();
        let mut player_spawn = None;
        let mut goal_tile = None;

        for (row, line) in lines.iter().enumerate() {
            for (col, ch) in line.chars().enumerate() {
                let world_x = col as f32 * tile_size;
                let world_y = row as f32 * tile_size;
                let tile_pos = vec2(world_x, world_y);

                match ch {
                    '#' => {
                        solid_tiles[row * width + col] = true;
                        solids.push(physics::rect_at(tile_pos, vec2(tile_size, tile_size)));
                    }
                    'C' => coins.push(vec2(world_x + tile_size * 0.5, world_y + tile_size * 0.5)),
                    'M' => mushroom_tiles.push(tile_pos),
                    'E' => enemy_spawns.push(tile_pos),
                    'P' => {
                        if player_spawn.is_some() {
                            return Err("Multiple player spawns found".to_string());
                        }
                        player_spawn = Some(tile_pos);
                    }
                    'G' => {
                        if goal_tile.is_some() {
                            return Err("Multiple goal tiles found".to_string());
                        }
                        goal_tile = Some(tile_pos);
                    }
                    '.' => {}
                    _ => {
                        return Err(format!("Unexpected tile '{ch}'"));
                    }
                }
            }
        }

        let player_spawn = player_spawn.ok_or_else(|| "Missing player spawn".to_string())?;
        let goal_tile = goal_tile.ok_or_else(|| "Missing goal tile".to_string())?;

        let mut world = Self {
            solids,
            solid_tiles,
            coins,
            mushrooms: Vec::new(),
            enemy_spawns,
            player_spawn,
            goal_tile,
            width,
            height,
        };

        world.mushrooms = mushroom_tiles
            .into_iter()
            .map(|tile_pos| {
                let size = config.mushroom_size;
                let tile = config.tile_size;
                let x = tile_pos.x + (tile - size.x) * 0.5;
                let sample_x = tile_pos.x + tile * 0.5;
                let base_y = world
                    .ground_y_for_x(sample_x, tile_pos.y, config)
                    .unwrap_or(tile_pos.y + tile);
                let y = base_y - size.y;
                vec2(x, y)
            })
            .collect();

        Ok(world)
    }

    pub fn draw(&self, config: &Config) {
        let tile = config.tile_size;

        for solid in &self.solids {
            draw_rectangle(
                solid.x,
                solid.y,
                solid.w,
                solid.h,
                Color::new(0.25, 0.55, 0.25, 1.0),
            );
        }

        for coin in &self.coins {
            draw_circle(coin.x, coin.y, tile * 0.2, Color::new(0.95, 0.8, 0.2, 1.0));
        }

        self.draw_mushrooms(config);
        self.draw_goal_post(config);
    }

    pub fn goal_trigger_rect(&self, config: &Config) -> Rect {
        let tile = config.tile_size;
        let goal_center_x = self.goal_tile.x + tile * 0.5;
        let base_y = self
            .ground_y_for_x(goal_center_x, self.goal_tile.y, config)
            .unwrap_or(self.goal_tile.y + tile);

        let pole_height = tile * 3.0;
        let pole_w = tile * 0.18;
        let pole_x = goal_center_x - pole_w * 0.5;
        let pole_y = base_y - pole_height;

        Rect::new(pole_x, pole_y, pole_w, pole_height)
    }

    fn draw_mushrooms(&self, config: &Config) {
        let size = config.mushroom_size;
        for pos in &self.mushrooms {
            let stem_w = size.x * 0.35;
            let stem_h = size.y * 0.45;
            let stem_x = pos.x + (size.x - stem_w) * 0.5;
            let stem_y = pos.y + size.y - stem_h;
            draw_rectangle(
                stem_x,
                stem_y,
                stem_w,
                stem_h,
                Color::new(0.95, 0.9, 0.75, 1.0),
            );

            let cap_h = size.y * 0.6;
            draw_rectangle(
                pos.x,
                pos.y,
                size.x,
                cap_h,
                Color::new(0.85, 0.15, 0.55, 1.0),
            );
            draw_rectangle(
                pos.x + size.x * 0.15,
                pos.y + cap_h * 0.25,
                size.x * 0.2,
                cap_h * 0.35,
                WHITE,
            );
        }
    }

    pub fn is_solid_tile(&self, col: i32, row: i32) -> bool {
        if col < 0 || row < 0 {
            return false;
        }
        let col = col as usize;
        let row = row as usize;
        if col >= self.width || row >= self.height {
            return false;
        }
        self.solid_tiles[row * self.width + col]
    }

    pub fn ground_y_for_x(&self, world_x: f32, start_y: f32, config: &Config) -> Option<f32> {
        let tile = config.tile_size;
        let col = (world_x / tile).floor() as i32;
        let start_row = (start_y / tile).floor().max(0.0) as i32;
        for row in start_row..(self.height as i32) {
            if self.is_solid_tile(col, row) {
                return Some(row as f32 * tile);
            }
        }
        None
    }

    pub fn camera_for_focus(&self, focus: Vec2, config: &Config) -> Camera2D {
        let world_w = self.width as f32 * config.tile_size;
        let world_h = self.height as f32 * config.tile_size;
        let screen_w = screen_width();
        let screen_h = screen_height();

        let mut cam_x = focus.x;
        let mut cam_y = focus.y;

        if world_w > screen_w {
            cam_x = cam_x.clamp(screen_w * 0.5, world_w - screen_w * 0.5);
        } else {
            cam_x = world_w * 0.5;
        }

        if world_h > screen_h {
            cam_y = cam_y.clamp(screen_h * 0.5, world_h - screen_h * 0.5);
        } else {
            cam_y = world_h * 0.5;
        }

        Camera2D {
            target: vec2(cam_x, cam_y),
            zoom: vec2(2.0 / screen_w, 2.0 / screen_h),
            ..Default::default()
        }
    }

    fn draw_goal_post(&self, config: &Config) {
        let tile = config.tile_size;
        let goal_center_x = self.goal_tile.x + tile * 0.5;
        let base_y = self
            .ground_y_for_x(goal_center_x, self.goal_tile.y, config)
            .unwrap_or(self.goal_tile.y + tile);

        let pole_height = tile * 3.0;
        let pole_w = tile * 0.12;
        let pole_x = self.goal_tile.x + tile * 0.5 - pole_w * 0.5;
        let pole_y = base_y - pole_height;

        draw_rectangle(pole_x, pole_y, pole_w, pole_height, GRAY);
        draw_rectangle(
            pole_x + pole_w,
            pole_y + tile * 0.3,
            tile * 0.55,
            tile * 0.35,
            RED,
        );
        draw_rectangle(
            self.goal_tile.x + tile * 0.4,
            base_y - tile * 0.12,
            tile * 0.2,
            tile * 0.12,
            DARKBROWN,
        );
    }
}
