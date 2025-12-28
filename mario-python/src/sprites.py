import pygame


def _scale2x(surface, scale):
    if scale == 1:
        return surface
    return pygame.transform.scale(surface, (surface.get_width() * scale, surface.get_height() * scale))


def _pixel_sprite(pattern, palette, scale=1):
    if not pattern:
        raise ValueError("pattern is empty")
    width = len(pattern[0])
    if any(len(row) != width for row in pattern):
        raise ValueError("pattern rows must be the same width")

    surface = pygame.Surface((width, len(pattern)), pygame.SRCALPHA)
    for y, row in enumerate(pattern):
        for x, ch in enumerate(row):
            if ch == ".":
                continue
            color = palette.get(ch)
            if color is None:
                continue
            surface.set_at((x, y), color)
    surface = _scale2x(surface, scale)
    try:
        surface = surface.convert_alpha()
    except pygame.error:
        pass
    return surface


def blit_tiled(dest, tile, rect):
    tile_w, tile_h = tile.get_size()
    if tile_w <= 0 or tile_h <= 0:
        return
    for y in range(rect.top, rect.bottom, tile_h):
        for x in range(rect.left, rect.right, tile_w):
            w = min(tile_w, rect.right - x)
            h = min(tile_h, rect.bottom - y)
            if w <= 0 or h <= 0:
                continue
            dest.blit(tile, (x, y), area=pygame.Rect(0, 0, w, h))


class Sprites:
    def __init__(self, scale=2):
        self.scale = scale

        self.ground_tile = self._make_ground_tile()
        self.coin_frames = self._make_coin_frames()
        self.goal = self._make_goal_sprite()
        self.mushroom = self._make_mushroom_sprite()

        self.player = {
            "normal": self._make_player_form(outfit_rgb=(220, 50, 60), overalls_rgb=(35, 70, 200)),
            "powered": self._make_player_form(outfit_rgb=(65, 205, 95), overalls_rgb=(30, 150, 70)),
        }

        enemy_right = self._make_enemy_frames()
        self.enemy = [
            {
                "right": enemy_right[0],
                "left": pygame.transform.flip(enemy_right[0], True, False),
            },
            {
                "right": enemy_right[1],
                "left": pygame.transform.flip(enemy_right[1], True, False),
            },
        ]

    def _make_player_form(self, outfit_rgb, overalls_rgb):
        right = self._make_player_frames(outfit_rgb=outfit_rgb, overalls_rgb=overalls_rgb)
        return {
            "idle": {
                "right": right["idle"],
                "left": pygame.transform.flip(right["idle"], True, False),
            },
            "walk": [
                {
                    "right": right["walk"][0],
                    "left": pygame.transform.flip(right["walk"][0], True, False),
                },
                {
                    "right": right["walk"][1],
                    "left": pygame.transform.flip(right["walk"][1], True, False),
                },
            ],
            "jump": {
                "right": right["jump"],
                "left": pygame.transform.flip(right["jump"], True, False),
            },
        }

    def _make_ground_tile(self):
        base = pygame.Surface((20, 20), pygame.SRCALPHA)
        dirt = (115, 70, 35)
        dirt_dark = (92, 56, 28)
        grass = (70, 200, 90)
        grass_dark = (55, 165, 72)
        stone = (150, 145, 140)

        base.fill(dirt)
        pygame.draw.rect(base, grass, pygame.Rect(0, 0, 20, 6))
        for x in range(0, 20, 2):
            base.set_at((x, 5), grass_dark)

        for x, y in [
            (3, 9),
            (7, 12),
            (13, 10),
            (16, 15),
            (10, 17),
        ]:
            base.set_at((x, y), dirt_dark)

        pygame.draw.rect(base, stone, pygame.Rect(14, 11, 3, 3))
        pygame.draw.rect(base, (110, 105, 102), pygame.Rect(15, 12, 1, 1))

        tile = _scale2x(base, self.scale)
        try:
            tile = tile.convert_alpha()
        except pygame.error:
            pass
        return tile

    def _make_coin_frames(self):
        frames = []
        for width in (9, 7, 5, 7):
            base = pygame.Surface((9, 9), pygame.SRCALPHA)
            rect = pygame.Rect(0, 0, width, 9)
            rect.center = (4, 4)
            pygame.draw.ellipse(base, (250, 215, 70), rect)
            pygame.draw.ellipse(base, (140, 110, 20), rect, width=1)
            highlight = pygame.Rect(rect.x + 1, rect.y + 1, max(1, rect.w - 2), max(1, rect.h - 2))
            pygame.draw.ellipse(base, (255, 245, 180), highlight, width=1)
            frame = _scale2x(base, self.scale)
            try:
                frame = frame.convert_alpha()
            except pygame.error:
                pass
            frames.append(frame)
        return frames

    def _make_goal_sprite(self):
        w, h = 20, 140
        surf = pygame.Surface((w, h), pygame.SRCALPHA)
        pole = (245, 245, 245)
        pole_shadow = (195, 195, 195)
        flag = (220, 50, 60)
        flag_shadow = (160, 30, 38)

        pygame.draw.rect(surf, pole_shadow, pygame.Rect(10, 0, 4, h))
        pygame.draw.rect(surf, pole, pygame.Rect(9, 0, 4, h))
        pygame.draw.circle(surf, pole, (11, 4), 4)

        pygame.draw.polygon(
            surf,
            flag_shadow,
            [(11, 10), (11, 38), (2, 32)],
        )
        pygame.draw.polygon(
            surf,
            flag,
            [(10, 10), (10, 38), (1, 32)],
        )
        pygame.draw.rect(surf, (250, 215, 70), pygame.Rect(3, h - 10, 14, 6), border_radius=2)
        try:
            surf = surf.convert_alpha()
        except pygame.error:
            pass
        return surf

    def _make_player_frames(self, outfit_rgb, overalls_rgb):
        palette = {
            "k": (24, 24, 30),
            "r": outfit_rgb,
            "b": overalls_rgb,
            "s": (245, 218, 188),
            "w": (255, 255, 255),
            "y": (250, 215, 70),
            "n": (38, 38, 45),
        }

        idle = [
            "....rrr....",
            "...rrrrr...",
            "...rkkkr...",
            "..ksswssk..",
            "..ksssssk..",
            "..krrrrrk..",
            "..krryrrk..",
            "..kbbbbbk..",
            "..kbbbbbbk.",
            "..kbbbbbbk.",
            "..kb...bk..",
            "..kn...nk..",
            "...nn.nn...",
            "...........",
        ]

        walk1 = [
            "....rrr....",
            "...rrrrr...",
            "...rkkkr...",
            "..ksswssk..",
            "..ksssssk..",
            "..krrrrrk..",
            "..krryrrk..",
            "..kbbbbbk..",
            "..kbbbbbbk.",
            ".kbbb..bbk.",
            "..kb...bk..",
            "..kn...nk..",
            "...nn.nn...",
            "...........",
        ]

        walk2 = [
            "....rrr....",
            "...rrrrr...",
            "...rkkkr...",
            "..ksswssk..",
            "..ksssssk..",
            "..krrrrrk..",
            "..krryrrk..",
            "..kbbbbbk..",
            "..kbbbbbbk.",
            ".kbb..bbbk.",
            "..kb...bk..",
            "..kn...nk..",
            "...nn.nn...",
            "...........",
        ]

        jump = [
            "....rrr....",
            "...rrrrr...",
            "...rkkkr...",
            "..ksswssk..",
            "..ksssssk..",
            "..krrrrrk..",
            "..krryrrk..",
            "..kbbbbbk..",
            "..kbbbbbbk.",
            ".kbbb..bbk.",
            "..kbbbbb k..".replace(" ", ""),
            "...knnnk...",
            "....nnn....",
            "...........",
        ]

        return {
            "idle": _pixel_sprite(idle, palette, scale=self.scale),
            "walk": [
                _pixel_sprite(walk1, palette, scale=self.scale),
                _pixel_sprite(walk2, palette, scale=self.scale),
            ],
            "jump": _pixel_sprite(jump, palette, scale=self.scale),
        }

    def _make_mushroom_sprite(self):
        palette = {
            "k": (24, 24, 30),
            "r": (220, 50, 60),
            "w": (255, 255, 255),
            "s": (245, 218, 188),
            "d": (200, 175, 150),
        }
        pattern = [
            "..kkkkkkk..",
            ".krrrrrrrk.",
            "krrwwwwwrrk",
            "krrwwrwwrrk",
            "krrwwwwwrrk",
            ".krrrrrrrk.",
            "..kdddddk..",
            "..kdd.ddk..",
            "..kdd.ddk..",
            "..kdddddk..",
            "...kkkkk...",
        ]
        surface = _pixel_sprite(pattern, palette, scale=self.scale)
        return surface

    def _make_enemy_frames(self):
        def frame(feet_offset):
            base = pygame.Surface((12, 10), pygame.SRCALPHA)
            body = (160, 100, 48)
            outline = (70, 40, 18)
            eye = (255, 255, 255)
            pupil = (24, 24, 30)
            cheek = (205, 120, 60)

            pygame.draw.ellipse(base, body, pygame.Rect(0, 1, 12, 8))
            pygame.draw.ellipse(base, outline, pygame.Rect(0, 1, 12, 8), width=1)
            pygame.draw.circle(base, eye, (4, 4), 2)
            pygame.draw.circle(base, eye, (8, 4), 2)
            pygame.draw.circle(base, pupil, (4, 4), 1)
            pygame.draw.circle(base, pupil, (8, 4), 1)
            pygame.draw.circle(base, cheek, (2, 6), 1)
            pygame.draw.circle(base, cheek, (10, 6), 1)

            pygame.draw.rect(base, outline, pygame.Rect(3 + feet_offset, 9, 2, 1))
            pygame.draw.rect(base, outline, pygame.Rect(7 - feet_offset, 9, 2, 1))
            return _scale2x(base, self.scale)

        frames = [frame(0), frame(1)]
        for idx, surf in enumerate(frames):
            try:
                frames[idx] = surf.convert_alpha()
            except pygame.error:
                pass
        return frames
