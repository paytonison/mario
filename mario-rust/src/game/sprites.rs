use macroquad::prelude::*;

pub struct Sprites {
    player_base: Texture2D,
    player_powered: Texture2D,
    chestnut_guy: Texture2D,
}

impl Sprites {
    pub fn new() -> Self {
        let player_base = player_texture(
            Color::new(0.78, 0.14, 0.16, 1.0),
            Color::new(0.16, 0.28, 0.78, 1.0),
        );
        let player_powered = player_texture(
            Color::new(0.18, 0.62, 0.35, 1.0),
            Color::new(0.2, 0.6, 0.86, 1.0),
        );
        let chestnut_guy = chestnut_guy_texture();

        Self {
            player_base,
            player_powered,
            chestnut_guy,
        }
    }

    pub fn player(&self, powered: bool) -> &Texture2D {
        if powered {
            &self.player_powered
        } else {
            &self.player_base
        }
    }

    pub fn chestnut_guy(&self) -> &Texture2D {
        &self.chestnut_guy
    }
}

fn player_texture(shirt: Color, overalls: Color) -> Texture2D {
    // 11x14 pixels, scaled 2x to match the default 22x28 player hitbox.
    let pixels: [&str; 14] = [
        "...RRRRR...",
        "..RRRRRRR..",
        "..RRRRRRR..",
        "...SSSSS...",
        "..SSSSSSS..",
        "..SSKKKSS..",
        "...RRRRR...",
        "..RRBBBBR..",
        "..RBBBBBR..",
        "..BBBBBBB..",
        "..BBYYBB...",
        "...BBBBB...",
        "..KK..KK...",
        "..KK..KK...",
    ];

    let skin = Color::new(0.98, 0.82, 0.68, 1.0);
    let dark = Color::new(0.12, 0.08, 0.07, 1.0);
    let button = Color::new(0.98, 0.88, 0.2, 1.0);

    texture_from_pixels(pixels.as_slice(), |ch| match ch {
        '.' => None,
        'R' => Some(shirt),
        'B' => Some(overalls),
        'S' => Some(skin),
        'K' => Some(dark),
        'Y' => Some(button),
        _ => None,
    })
}

fn chestnut_guy_texture() -> Texture2D {
    // 12x10 pixels, scaled 2x to match the default 24x20 enemy hitbox.
    let pixels: [&str; 10] = [
        "...BBBBBB...",
        "..BBBBBBBB..",
        ".BBBBDDDBBB.",
        ".BBBWKKWBBB.",
        ".BBBWKKWBBB.",
        ".BBBBDDDDBB.",
        "..BBBDDDDB..",
        "...DD..DD...",
        "..DD....DD..",
        "...DD..DD...",
    ];

    let brown = Color::new(0.55, 0.35, 0.2, 1.0);
    let dark_brown = Color::new(0.38, 0.22, 0.12, 1.0);

    texture_from_pixels(pixels.as_slice(), |ch| match ch {
        '.' => None,
        'B' => Some(brown),
        'D' => Some(dark_brown),
        'W' => Some(WHITE),
        'K' => Some(BLACK),
        _ => None,
    })
}

fn texture_from_pixels<F>(rows: &[&str], mut color_for: F) -> Texture2D
where
    F: FnMut(char) -> Option<Color>,
{
    let height = rows.len();
    let width = rows.first().map(|row| row.chars().count()).unwrap_or(0);

    let mut bytes = Vec::with_capacity(width * height * 4);
    for row in rows {
        assert_eq!(
            row.chars().count(),
            width,
            "Sprite rows must have a consistent width"
        );

        for ch in row.chars() {
            let color = color_for(ch).unwrap_or(Color::new(0.0, 0.0, 0.0, 0.0));
            bytes.push((color.r * 255.0) as u8);
            bytes.push((color.g * 255.0) as u8);
            bytes.push((color.b * 255.0) as u8);
            bytes.push((color.a * 255.0) as u8);
        }
    }

    let image = Image {
        bytes,
        width: width as u16,
        height: height as u16,
    };
    let texture = Texture2D::from_image(&image);
    texture.set_filter(FilterMode::Nearest);
    texture
}
