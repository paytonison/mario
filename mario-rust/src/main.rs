mod game;

use macroquad::prelude::*;

fn window_conf() -> Conf {
    Conf {
        window_title: "Jumpman Rust".to_string(),
        window_width: 960,
        window_height: 540,
        window_resizable: true,
        ..Default::default()
    }
}

#[macroquad::main(window_conf)]
async fn main() {
    let mut game = game::Game::new().await;

    loop {
        let frame_dt = get_frame_time();
        game.update(frame_dt);
        game.draw();
        next_frame().await;
    }
}
