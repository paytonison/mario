# Jumpman Clone (Pygame)

This repository contains a compact Jumpman-style platformer built with Python and Pygame.

## Features

- Chestnut guys that can be stomped or avoided
- Coins that increase score; every 10 coins grants an extra life
- Mushroom power-up that turns Jumpman green; taking a hit removes it and grants brief invincibility
- Goalpost to complete the level
- Pitfall gaps that cost a life if you fall
- Chiptune-style background music (press `M` to toggle)
- Pixel-art sprites and synthesized sound effects

## Requirements

- Python 3.10+
- Pygame (`pip install -r requirements.txt`)

## Run

```bash
python src/main.py
```

Optional: cap the frame rate (default is 60 FPS):

```bash
python src/main.py --fps 120
```

### Controls

- Arrow keys or WASD to move
- Space to jump
- M to toggle music
- R to restart after win or game over
- Esc to quit
