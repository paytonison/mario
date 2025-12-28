import math

import pygame

from sprites import blit_tiled


SCREEN_WIDTH = 960
SCREEN_HEIGHT = 540


class Game:
    def __init__(self, sfx=None, sprites=None):
        self.sfx = sfx or {}
        self.sprites = sprites
        self.state = "playing"

        self.score = 0
        self.coins = 0
        self.lives = 3
        self.powered_up = False
        self.invincible_s = 0.0

        self._config = {
            "gravity": 2400.0,
            "terminal_velocity": 2200.0,
            "accel": 5200.0,
            "friction": 6400.0,
            "max_speed": 520.0,
            "jump_speed": 920.0,
            "stomp_bounce": 720.0,
            "enemy_speed": 140.0,
        }

        self.tile_size = 40
        self.world_width = 4800
        self.camera_x = 0.0

        self.solids = []
        ground_y = SCREEN_HEIGHT - 80
        ground_h = 120
        ground_segments = [
            (0, 1400),
            (1600, 2450),
            (2700, 3550),
            (3800, self.world_width),
        ]
        for start_x, end_x in ground_segments:
            self.solids.append(
                pygame.Rect(int(start_x), int(ground_y), int(end_x - start_x), int(ground_h))
            )

        self.solids.extend(
            [
                pygame.Rect(520, 340, 240, 28),
                pygame.Rect(980, 280, 200, 28),
                pygame.Rect(1880, 320, 240, 28),
                pygame.Rect(2220, 260, 200, 28),
                pygame.Rect(3080, 320, 240, 28),
                pygame.Rect(4120, 300, 240, 28),
            ]
        )

        self._coin_rects = []
        for x, y in [
            (560, 300),
            (620, 300),
            (680, 300),
            (1020, 240),
            (1080, 240),
            (1940, 280),
            (2000, 280),
            (2280, 220),
            (3140, 280),
            (3200, 280),
            (4180, 260),
            (4240, 260),
        ]:
            self._coin_rects.append(pygame.Rect(int(x), int(y), 18, 18))

        self._mushroom_rects = []
        for x, y in [
            (640, 318),
            (1980, 298),
        ]:
            self._mushroom_rects.append(pygame.Rect(int(x), int(y), 22, 22))

        self.goal = pygame.Rect(self.world_width - 120, ground_y - 140, 20, 140)

        self.spawn = pygame.Vector2(120.0, float(ground_y - 28))
        self.player = {
            "x": float(self.spawn.x),
            "y": float(self.spawn.y),
            "w": 22.0,
            "h": 28.0,
            "vx": 0.0,
            "vy": 0.0,
            "on_ground": False,
        }
        self._player_dir = 1

        self.chestnut_guys = []
        for x in [760, 1760, 2920, 4060]:
            self.chestnut_guys.append(
                {
                    "x": float(x),
                    "y": float(ground_y - 20),
                    "w": 24.0,
                    "h": 20.0,
                    "vx": -self._config["enemy_speed"],
                    "vy": 0.0,
                    "dir": -1.0,
                    "alive": True,
                }
            )

        self._last_time_s = pygame.time.get_ticks() / 1000.0

    def update(self, action):
        now = pygame.time.get_ticks() / 1000.0
        dt = max(0.0, min(1 / 30, now - self._last_time_s))
        self._last_time_s = now

        if self.state != "playing":
            self._update_camera()
            return

        if self.invincible_s > 0.0:
            self.invincible_s = max(0.0, self.invincible_s - dt)

        left, right, jump = action
        self._update_player(left, right, jump, dt)
        self._collect_coins()
        self._collect_mushrooms()
        self._update_enemies(dt)
        self._check_goal()
        self._check_fall_off_world()
        self._update_camera()

    def draw(self, screen, fonts):
        screen.fill((120, 190, 255))

        camera_x = int(self.camera_x)
        ticks = pygame.time.get_ticks()

        if self.sprites:
            for rect in self.solids:
                draw_rect = pygame.Rect(rect.x - camera_x, rect.y, rect.w, rect.h)
                blit_tiled(screen, self.sprites.ground_tile, draw_rect)
        else:
            for rect in self.solids:
                pygame.draw.rect(
                    screen,
                    (105, 70, 30),
                    pygame.Rect(rect.x - camera_x, rect.y, rect.w, rect.h),
                )

        if self.sprites:
            coin_frame = self.sprites.coin_frames[(ticks // 120) % len(self.sprites.coin_frames)]
            for rect in self._coin_rects:
                screen.blit(coin_frame, (rect.x - camera_x, rect.y))
        else:
            for rect in self._coin_rects:
                pygame.draw.ellipse(
                    screen,
                    (250, 215, 70),
                    pygame.Rect(rect.x - camera_x, rect.y, rect.w, rect.h),
                )

        if self._mushroom_rects:
            if self.sprites:
                for rect in self._mushroom_rects:
                    bob = int(math.sin((ticks + rect.x * 7) / 220.0) * 2)
                    screen.blit(self.sprites.mushroom, (rect.x - camera_x, rect.y + bob))
            else:
                for rect in self._mushroom_rects:
                    pygame.draw.rect(
                        screen,
                        (220, 50, 60),
                        pygame.Rect(rect.x - camera_x, rect.y, rect.w, rect.h),
                        border_radius=6,
                    )

        if self.sprites:
            screen.blit(self.sprites.goal, (self.goal.x - camera_x, self.goal.y))
        else:
            pygame.draw.rect(
                screen,
                (255, 255, 255),
                pygame.Rect(self.goal.x - camera_x, self.goal.y, self.goal.w, self.goal.h),
            )

        for enemy in self.chestnut_guys:
            if not enemy["alive"]:
                continue
            rect = pygame.Rect(int(enemy["x"]), int(enemy["y"]), int(enemy["w"]), int(enemy["h"]))
            rect.x -= camera_x
            if self.sprites:
                facing = "left" if enemy["dir"] < 0 else "right"
                frame = self.sprites.enemy[(ticks // 180) % len(self.sprites.enemy)][facing]
                screen.blit(frame, rect.topleft)
            else:
                pygame.draw.rect(screen, (150, 95, 45), rect, border_radius=6)
                eye = pygame.Rect(rect.x + 6, rect.y + 6, 4, 4)
                pygame.draw.rect(screen, (255, 255, 255), eye, border_radius=2)
                eye.x += 10
                pygame.draw.rect(screen, (255, 255, 255), eye, border_radius=2)

        player_rect = self._player_rect()
        player_draw = pygame.Rect(
            player_rect.x - camera_x, player_rect.y, player_rect.w, player_rect.h
        )
        if self.sprites:
            if self.invincible_s > 0.0 and (ticks // 90) % 2 == 0:
                pass
            else:
                form = "powered" if self.powered_up else "normal"
                facing = "left" if self._player_dir < 0 else "right"
                if not self.player["on_ground"]:
                    sprite = self.sprites.player[form]["jump"][facing]
                elif abs(self.player["vx"]) > 60:
                    sprite = self.sprites.player[form]["walk"][(ticks // 120) % 2][facing]
                else:
                    sprite = self.sprites.player[form]["idle"][facing]
                screen.blit(sprite, player_draw.topleft)
        else:
            top_color = (65, 205, 95) if self.powered_up else (220, 50, 60)
            if self.invincible_s > 0.0 and (ticks // 90) % 2 == 0:
                pass
            else:
                pygame.draw.rect(screen, top_color, player_draw, border_radius=6)
                pygame.draw.rect(
                    screen,
                    (35, 70, 200),
                    pygame.Rect(player_draw.x, player_draw.y + 14, player_draw.w, 14),
                    border_radius=6,
                )

        status = []
        if self.powered_up:
            status.append("POWER")
        if self.invincible_s > 0.0:
            status.append("INV")
        suffix = f"   {' '.join(status)}" if status else ""
        hud = f"Score {self.score}   Coins {self.coins}   Lives {self.lives}{suffix}"
        text = fonts["small"].render(hud, True, (0, 0, 0))
        screen.blit(text, (14, 12))

        if self.state == "win":
            msg = fonts["large"].render("YOU WIN! Press R to restart.", True, (10, 10, 10))
            screen.blit(msg, (SCREEN_WIDTH // 2 - msg.get_width() // 2, 120))
        elif self.state == "gameover":
            msg = fonts["large"].render("GAME OVER. Press R to restart.", True, (10, 10, 10))
            screen.blit(msg, (SCREEN_WIDTH // 2 - msg.get_width() // 2, 120))

    def _player_rect(self):
        return pygame.Rect(
            int(self.player["x"]),
            int(self.player["y"]),
            int(self.player["w"]),
            int(self.player["h"]),
        )

    def _play_sfx(self, name):
        sound = self.sfx.get(name)
        if sound:
            try:
                sound.play()
            except pygame.error:
                pass

    def _update_player(self, left, right, jump, dt):
        prev_rect = self._player_rect()

        accel = self._config["accel"]
        friction = self._config["friction"]
        max_speed = self._config["max_speed"]

        if left and not right:
            self._player_dir = -1
            self.player["vx"] -= accel * dt
        elif right and not left:
            self._player_dir = 1
            self.player["vx"] += accel * dt
        else:
            if self.player["vx"] > 0.0:
                self.player["vx"] = max(0.0, self.player["vx"] - friction * dt)
            elif self.player["vx"] < 0.0:
                self.player["vx"] = min(0.0, self.player["vx"] + friction * dt)

        self.player["vx"] = max(-max_speed, min(max_speed, self.player["vx"]))

        if jump and self.player["on_ground"]:
            self.player["vy"] = -self._config["jump_speed"]
            self.player["on_ground"] = False
            self._play_sfx("jump")

        self.player["vy"] = min(
            self._config["terminal_velocity"],
            self.player["vy"] + self._config["gravity"] * dt,
        )

        self.player["x"] += self.player["vx"] * dt
        rect = self._player_rect()
        rect = self._resolve_solids_axis(rect, axis="x")
        self.player["x"] = float(rect.x)

        self.player["y"] += self.player["vy"] * dt
        rect = self._player_rect()
        self.player["on_ground"] = False
        rect = self._resolve_solids_axis(rect, axis="y")
        self.player["y"] = float(rect.y)

        if rect.y != int(self.player["y"]):
            self.player["y"] = float(rect.y)

        if rect.bottom == prev_rect.bottom and rect.y == prev_rect.y and not self.player["on_ground"]:
            pass

        if rect.bottom >= prev_rect.bottom and self.player["vy"] >= 0.0:
            if self._is_on_ground(rect):
                self.player["on_ground"] = True
                self.player["vy"] = 0.0

        self.player["x"] = max(0.0, min(float(self.world_width - rect.w), self.player["x"]))

        self._check_enemy_collisions(prev_rect)

    def _resolve_solids_axis(self, rect, axis):
        for solid in self.solids:
            if not rect.colliderect(solid):
                continue
            if axis == "x":
                if rect.centerx > solid.centerx:
                    rect.left = solid.right
                else:
                    rect.right = solid.left
                self.player["vx"] = 0.0
            else:
                if rect.centery > solid.centery:
                    rect.top = solid.bottom
                    self.player["vy"] = 0.0
                else:
                    rect.bottom = solid.top
                    self.player["vy"] = 0.0
                    self.player["on_ground"] = True
        return rect

    def _is_on_ground(self, rect):
        probe = pygame.Rect(rect.x, rect.bottom + 1, rect.w, 2)
        return any(probe.colliderect(solid) for solid in self.solids)

    def _collect_coins(self):
        player_rect = self._player_rect()
        remaining = []
        for coin in self._coin_rects:
            if player_rect.colliderect(coin):
                self.score += 200
                self.coins += 1
                self._play_sfx("coin")
                if self.coins % 10 == 0:
                    self.lives += 1
            else:
                remaining.append(coin)
        self._coin_rects = remaining

    def _collect_mushrooms(self):
        player_rect = self._player_rect()
        remaining = []
        for mushroom in self._mushroom_rects:
            if player_rect.colliderect(mushroom):
                if not self.powered_up:
                    self.powered_up = True
                    self.score += 1000
                    self._play_sfx("powerup")
                else:
                    self.score += 250
                    self._play_sfx("coin")
            else:
                remaining.append(mushroom)
        self._mushroom_rects = remaining

    def _update_enemies(self, dt):
        for enemy in self.chestnut_guys:
            if not enemy["alive"]:
                continue

            enemy["vy"] = min(
                self._config["terminal_velocity"],
                enemy["vy"] + self._config["gravity"] * dt,
            )
            enemy["vx"] = self._config["enemy_speed"] * enemy["dir"]

            enemy["x"] += enemy["vx"] * dt
            rect = pygame.Rect(int(enemy["x"]), int(enemy["y"]), int(enemy["w"]), int(enemy["h"]))
            hit_wall = False
            for solid in self.solids:
                if rect.colliderect(solid):
                    hit_wall = True
                    if rect.centerx > solid.centerx:
                        rect.left = solid.right
                    else:
                        rect.right = solid.left
                    enemy["vx"] = 0.0
            enemy["x"] = float(rect.x)

            enemy["y"] += enemy["vy"] * dt
            rect = pygame.Rect(int(enemy["x"]), int(enemy["y"]), int(enemy["w"]), int(enemy["h"]))
            on_ground = False
            for solid in self.solids:
                if rect.colliderect(solid):
                    if rect.centery > solid.centery:
                        rect.top = solid.bottom
                        enemy["vy"] = 0.0
                    else:
                        rect.bottom = solid.top
                        enemy["vy"] = 0.0
                        on_ground = True
            enemy["y"] = float(rect.y)

            if hit_wall:
                enemy["dir"] *= -1.0

            if on_ground:
                foot_x = rect.right + 2 if enemy["dir"] > 0 else rect.left - 2
                foot_probe = pygame.Rect(int(foot_x), rect.bottom + 2, 2, 6)
                if not any(foot_probe.colliderect(solid) for solid in self.solids):
                    enemy["dir"] *= -1.0

            if enemy["y"] > SCREEN_HEIGHT + 400:
                enemy["alive"] = False

    def _check_enemy_collisions(self, prev_player_rect):
        player_rect = self._player_rect()
        for enemy in self.chestnut_guys:
            if not enemy["alive"]:
                continue
            enemy_rect = pygame.Rect(
                int(enemy["x"]), int(enemy["y"]), int(enemy["w"]), int(enemy["h"])
            )
            if not player_rect.colliderect(enemy_rect):
                continue

            stomp = self.player["vy"] > 0.0 and prev_player_rect.bottom <= enemy_rect.top + 6
            if stomp:
                enemy["alive"] = False
                self.player["vy"] = -self._config["stomp_bounce"]
                self.score += 100
                self._play_sfx("stomp")
            else:
                if self.invincible_s > 0.0:
                    continue
                if self.powered_up:
                    self._power_down(enemy_rect)
                    return
                self._play_sfx("hurt")
                self._lose_life_and_respawn()
                return

    def _power_down(self, enemy_rect):
        self.powered_up = False
        self.invincible_s = 1.25
        self._play_sfx("powerdown")

        player_rect = self._player_rect()
        if player_rect.centerx < enemy_rect.centerx:
            knock_dir = -1.0
            self.player["x"] = float(enemy_rect.left - player_rect.w - 2)
        else:
            knock_dir = 1.0
            self.player["x"] = float(enemy_rect.right + 2)

        self.player["vx"] = knock_dir * self._config["max_speed"] * 0.9
        self.player["vy"] = -self._config["jump_speed"] * 0.55
        self.player["on_ground"] = False
        self.player["x"] = max(0.0, min(float(self.world_width - player_rect.w), self.player["x"]))

    def _check_goal(self):
        if self.state != "playing":
            return
        if self._player_rect().colliderect(self.goal):
            self.state = "win"
            self.score += 500
            self._play_sfx("win")

    def _check_fall_off_world(self):
        if self.state != "playing":
            return
        if self.player["y"] > SCREEN_HEIGHT + 400:
            self._lose_life_and_respawn()

    def _lose_life_and_respawn(self):
        self.lives -= 1
        if self.lives <= 0:
            self.state = "gameover"
            return
        self.powered_up = False
        self.invincible_s = 0.0
        self.player["x"] = float(self.spawn.x)
        self.player["y"] = float(self.spawn.y)
        self.player["vx"] = 0.0
        self.player["vy"] = 0.0
        self.player["on_ground"] = False

    def _update_camera(self):
        target = self.player["x"] + self.player["w"] * 0.5 - SCREEN_WIDTH * 0.5
        self.camera_x = max(0.0, min(float(self.world_width - SCREEN_WIDTH), target))
