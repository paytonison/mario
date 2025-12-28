use macroquad::prelude::*;

pub(crate) fn rect_at(pos: Vec2, size: Vec2) -> Rect {
    Rect::new(pos.x, pos.y, size.x, size.y)
}

pub(crate) fn rects_intersect(a: Rect, b: Rect) -> bool {
    a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h && a.y + a.h > b.y
}

pub(crate) fn approach(value: f32, target: f32, delta: f32) -> f32 {
    if value < target {
        (value + delta).min(target)
    } else {
        (value - delta).max(target)
    }
}

pub(crate) fn move_with_collisions(
    pos: Vec2,
    size: Vec2,
    vel: Vec2,
    solids: &[Rect],
    dt: f32,
) -> (Vec2, Vec2, bool) {
    let mut pos = pos;
    let mut vel = vel;
    let mut on_ground = false;

    pos.x += vel.x * dt;
    let mut rect = rect_at(pos, size);
    for solid in solids {
        if rects_intersect(rect, *solid) {
            if vel.x > 0.0 {
                pos.x = solid.x - size.x;
            } else if vel.x < 0.0 {
                pos.x = solid.x + solid.w;
            }
            vel.x = 0.0;
            rect.x = pos.x;
        }
    }

    pos.y += vel.y * dt;
    rect.y = pos.y;
    for solid in solids {
        if rects_intersect(rect, *solid) {
            if vel.y > 0.0 {
                pos.y = solid.y - size.y;
                on_ground = true;
            } else if vel.y < 0.0 {
                pos.y = solid.y + solid.h;
            }
            vel.y = 0.0;
            rect.y = pos.y;
        }
    }

    (pos, vel, on_ground)
}
