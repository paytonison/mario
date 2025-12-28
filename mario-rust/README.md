# Jumpman Clone (Rust + Macroquad)

A compact Jumpman-style platformer written in Rust using Macroquad. It uses simple shapes for sprites and a tiny tile map for the level.

## Requirements

- Rust (stable)

## Run

```bash
cargo run
```

## Controls

- Enter: start
- Arrow keys or A/D to move
- Space/Up/W to jump
- R to restart level
- Esc to quit to title

## Notes

- Score: coin = 200, stomp enemy = 100, flagpole = 500, mushroom = 1000.
- Mushroom power-up turns the player blue and grants one extra hit (the hit removes the power-up instead of resetting the level).
- Stomp chestnut guys by landing on them.

## Level Format

The level is an ASCII grid in `assets/levels/level1.txt`:

- `#` = solid tile
- `.` = empty
- `P` = player spawn (exactly one)
- `G` = goal / flagpole (exactly one)
- `E` = enemy spawn
- `C` = coin
- `M` = mushroom power-up

## Audio

The game generates simple procedural sound effects + a looping chiptune track by default (no files required).

To override them, add WAV files under `assets/`:

- `music.wav` (looping background track)

And WAV files under `assets/sfx/`:

- `jump.wav`, `coin.wav`, `stomp.wav`, `powerup.wav`, `hurt.wav`, `win.wav`

## Art

All visuals are placeholder shapes or tiny original pixel sprites generated in code. This project does not include any copyrighted Nintendo / Jumpman assets.
