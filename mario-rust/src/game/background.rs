use macroquad::prelude::*;

use super::{world::World, Config};

pub fn draw(camera: &Camera2D, world: &World, config: &Config) {
    let screen_w = screen_width();
    let cam_left = camera.target.x - screen_w * 0.5;
    let world_w = world.width as f32 * config.tile_size;
    let world_h = world.height as f32 * config.tile_size;

    draw_hills(cam_left, world_w, world_h, config);
    draw_clouds(cam_left, world_w);
}

fn draw_hills(cam_left: f32, world_w: f32, world_h: f32, config: &Config) {
    let horizon_y = world_h - config.tile_size * 1.25;

    let far_parallax = 0.25;
    let far_offset = cam_left * (1.0 - far_parallax);
    let far_color = Color::new(0.35, 0.68, 0.84, 1.0);
    for (x, radius) in [
        (world_w * 0.18, 190.0),
        (world_w * 0.52, 230.0),
        (world_w * 0.84, 200.0),
    ] {
        draw_circle(x + far_offset, horizon_y + 60.0, radius, far_color);
    }

    let near_parallax = 0.55;
    let near_offset = cam_left * (1.0 - near_parallax);
    let near_color = Color::new(0.28, 0.62, 0.34, 1.0);
    for (x, radius) in [
        (world_w * 0.05, 140.0),
        (world_w * 0.36, 160.0),
        (world_w * 0.66, 150.0),
        (world_w * 0.95, 130.0),
    ] {
        draw_circle(x + near_offset, horizon_y + 80.0, radius, near_color);
    }
}

fn draw_clouds(cam_left: f32, world_w: f32) {
    let parallax = 0.15;
    let offset = cam_left * (1.0 - parallax);
    let color = Color::new(1.0, 1.0, 1.0, 0.9);

    for (x, y, scale) in [
        (world_w * 0.12, 90.0, 1.0),
        (world_w * 0.38, 140.0, 1.2),
        (world_w * 0.64, 80.0, 0.9),
        (world_w * 0.86, 120.0, 1.1),
    ] {
        let cx = x + offset;
        let r = 18.0 * scale;
        draw_circle(cx - r * 0.8, y, r, color);
        draw_circle(cx, y - r * 0.35, r * 1.1, color);
        draw_circle(cx + r * 0.85, y, r * 0.95, color);
    }
}
