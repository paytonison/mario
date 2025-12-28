import argparse
import math
from array import array

import pygame

from game import Game, SCREEN_HEIGHT, SCREEN_WIDTH
from sprites import Sprites


def make_beep(freq_hz, duration_s, volume=0.4, sample_rate=44100):
    length = int(sample_rate * duration_s)
    amplitude = int(32767 * max(0.0, min(volume, 1.0)))
    buf = array("h")
    for i in range(length):
        t = i / sample_rate
        value = int(amplitude * math.sin(2 * math.pi * freq_hz * t))
        buf.append(value)
    return pygame.mixer.Sound(buffer=buf.tobytes())


def _midi_to_freq_hz(midi_note):
    return 440.0 * (2.0 ** ((midi_note - 69) / 12))


def make_music_loop(volume=0.18, tempo_bpm=120, sample_rate=44100):
    step_duration = 60.0 / float(tempo_bpm) / 2.0
    melody = [
        72, 76, 79, 76, 72, 76, 79, 84,
        77, 81, 84, 81, 77, 81, 84, 86,
        79, 83, 86, 83, 79, 83, 86, 88,
        72, 76, 79, 84, 79, 76, 72, None,
    ]
    bass = [
        48, 48, 48, 48, 48, 48, 48, 48,
        41, 41, 41, 41, 41, 41, 41, 41,
        43, 43, 43, 43, 43, 43, 43, 43,
        48, 48, 48, 48, 48, 48, 48, 48,
    ]

    fade_len = 256
    fade_curve = [i for i in range(fade_len)]
    max_amp = int(32767 * max(0.0, min(volume, 1.0)) * 0.5)
    phase_mel = 0
    phase_bass = 0
    buf = array("h")

    for melody_note, bass_note in zip(melody, bass):
        seg_len = int(sample_rate * step_duration)
        seg_fade = min(fade_len, max(0, seg_len // 2))

        inc_mel = (
            int(_midi_to_freq_hz(melody_note) * (1 << 32) / sample_rate)
            if melody_note
            else 0
        )
        inc_bass = (
            int(_midi_to_freq_hz(bass_note) * (1 << 32) / sample_rate) if bass_note else 0
        )

        for i in range(seg_len):
            sample = 0
            if inc_bass:
                phase_bass = (phase_bass + inc_bass) & 0xFFFFFFFF
                sample += 1 if phase_bass < 0x80000000 else -1
            if inc_mel:
                phase_mel = (phase_mel + inc_mel) & 0xFFFFFFFF
                sample += 1 if phase_mel < 0x80000000 else -1

            amp = max_amp
            if seg_fade:
                if i < seg_fade:
                    amp = max_amp * fade_curve[i] // seg_fade
                elif i >= seg_len - seg_fade:
                    amp = max_amp * fade_curve[seg_len - i - 1] // seg_fade

            buf.append(int(sample * amp))

    return pygame.mixer.Sound(buffer=buf.tobytes())


def init_audio():
    try:
        if not pygame.mixer.get_init():
            pygame.mixer.init()
    except pygame.error:
        return {}
    try:
        pygame.mixer.set_num_channels(16)
        pygame.mixer.set_reserved(1)
    except pygame.error:
        pass
    return {
        "coin": make_beep(880, 0.08, 0.35),
        "jump": make_beep(660, 0.06, 0.25),
        "stomp": make_beep(220, 0.08, 0.4),
        "hurt": make_beep(120, 0.14, 0.4),
        "powerup": make_beep(1046, 0.12, 0.35),
        "powerdown": make_beep(196, 0.16, 0.35),
        "win": make_beep(990, 0.2, 0.4),
    }


def parse_args():
    parser = argparse.ArgumentParser(description="Jumpman clone (player-controlled).")
    parser.add_argument(
        "--fps",
        type=int,
        default=60,
        help="Frames per second for the game loop.",
    )
    return parser.parse_args()


def read_player_input(jump_pressed):
    keys = pygame.key.get_pressed()
    left = keys[pygame.K_LEFT] or keys[pygame.K_a]
    right = keys[pygame.K_RIGHT] or keys[pygame.K_d]
    if left and right:
        left = right = False
    jump = bool(jump_pressed)
    return left, right, jump


def main():
    args = parse_args()
    pygame.mixer.pre_init(44100, -16, 1, 512)
    pygame.init()
    pygame.display.set_caption("Jumpman Clone")

    screen = pygame.display.set_mode((SCREEN_WIDTH, SCREEN_HEIGHT))
    fonts = {
        "small": pygame.font.Font(None, 24),
        "large": pygame.font.Font(None, 36),
    }
    sprites = Sprites()
    sfx = init_audio()
    music_channel = None
    music_paused = False
    if pygame.mixer.get_init():
        try:
            music_channel = pygame.mixer.Channel(0)
            music_channel.set_volume(0.35)
            music_channel.play(make_music_loop(), loops=-1)
        except pygame.error:
            music_channel = None

    game = Game(sfx=sfx, sprites=sprites)

    clock = pygame.time.Clock()
    running = True
    while running:
        jump_pressed = False
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            elif event.type == pygame.KEYDOWN:
                if event.key == pygame.K_ESCAPE:
                    running = False
                if event.key == pygame.K_r and game.state != "playing":
                    game = Game(sfx=sfx, sprites=sprites)
                if event.key in {pygame.K_SPACE, pygame.K_w, pygame.K_UP}:
                    jump_pressed = True
                if event.key == pygame.K_m and music_channel:
                    if music_paused:
                        music_channel.unpause()
                    else:
                        music_channel.pause()
                    music_paused = not music_paused

        action = read_player_input(jump_pressed)
        game.update(action)

        game.draw(screen, fonts)
        pygame.display.flip()
        clock.tick(args.fps)

    pygame.quit()


if __name__ == "__main__":
    main()
