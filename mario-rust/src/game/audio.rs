use macroquad::audio::{
    load_sound, load_sound_from_bytes, play_sound, stop_sound, PlaySoundParams, Sound,
};

pub struct Sfx {
    jump: Option<Sound>,
    coin: Option<Sound>,
    stomp: Option<Sound>,
    powerup: Option<Sound>,
    hurt: Option<Sound>,
    win: Option<Sound>,
    music: Option<Sound>,
    music_playing: bool,
    volume: f32,
    music_volume: f32,
}

impl Sfx {
    pub async fn new() -> Self {
        Self {
            jump: load_or_generate("sfx/jump.wav", default_jump_sound).await,
            coin: load_or_generate("sfx/coin.wav", default_coin_sound).await,
            stomp: load_or_generate("sfx/stomp.wav", default_stomp_sound).await,
            powerup: load_or_generate("sfx/powerup.wav", default_powerup_sound).await,
            hurt: load_or_generate("sfx/hurt.wav", default_hurt_sound).await,
            win: load_or_generate("sfx/win.wav", default_win_sound).await,
            music: load_or_generate("music.wav", default_music_sound).await,
            music_playing: false,
            volume: 0.45,
            music_volume: 0.22,
        }
    }

    pub fn play_jump(&self) {
        self.play(&self.jump);
    }

    pub fn play_coin(&self) {
        self.play(&self.coin);
    }

    pub fn play_stomp(&self) {
        self.play(&self.stomp);
    }

    pub fn play_powerup(&self) {
        self.play(&self.powerup);
    }

    pub fn play_hurt(&self) {
        self.play(&self.hurt);
    }

    pub fn play_win(&self) {
        self.play(&self.win);
    }

    pub fn start_music(&mut self) {
        if self.music_playing {
            return;
        }

        let Some(sound) = &self.music else {
            return;
        };

        play_sound(
            sound,
            PlaySoundParams {
                looped: true,
                volume: self.music_volume,
            },
        );
        self.music_playing = true;
    }

    pub fn stop_music(&mut self) {
        if !self.music_playing {
            return;
        }

        let Some(sound) = &self.music else {
            self.music_playing = false;
            return;
        };

        stop_sound(sound);
        self.music_playing = false;
    }

    fn play(&self, sound: &Option<Sound>) {
        let Some(sound) = sound else {
            return;
        };

        play_sound(
            sound,
            PlaySoundParams {
                looped: false,
                volume: self.volume,
            },
        );
    }
}

async fn load_or_generate(path: &str, generator: fn() -> Vec<u8>) -> Option<Sound> {
    match load_sound(path).await {
        Ok(sound) => Some(sound),
        Err(_) => load_sound_from_bytes(&generator()).await.ok(),
    }
}

fn default_jump_sound() -> Vec<u8> {
    synth_sine_wav(720.0, 0.12, 0.25)
}

fn default_coin_sound() -> Vec<u8> {
    synth_sine_wav(980.0, 0.08, 0.28)
}

fn default_stomp_sound() -> Vec<u8> {
    synth_sine_wav(220.0, 0.10, 0.35)
}

fn default_powerup_sound() -> Vec<u8> {
    synth_sine_wav(540.0, 0.18, 0.28)
}

fn default_hurt_sound() -> Vec<u8> {
    synth_sine_wav(160.0, 0.16, 0.32)
}

fn default_win_sound() -> Vec<u8> {
    synth_sine_wav(660.0, 0.22, 0.24)
}

fn default_music_sound() -> Vec<u8> {
    synth_chiptune_wav()
}

fn synth_sine_wav(freq_hz: f32, duration_s: f32, amplitude: f32) -> Vec<u8> {
    let sample_rate = 44_100u32;
    let samples = synth_sine_mono_16(sample_rate, freq_hz, duration_s, amplitude);
    wav_pcm_mono_16(sample_rate, &samples)
}

fn synth_sine_mono_16(sample_rate: u32, freq_hz: f32, duration_s: f32, amplitude: f32) -> Vec<i16> {
    let sample_rate_f = sample_rate as f32;
    let len = (duration_s * sample_rate_f).round().max(1.0) as usize;
    let attack = (sample_rate_f * 0.01) as usize;
    let release = (sample_rate_f * 0.02) as usize;
    let amplitude = amplitude.clamp(0.0, 1.0);

    let mut out = Vec::with_capacity(len);
    for i in 0..len {
        let t = i as f32 / sample_rate_f;
        let mut env = 1.0;
        if i < attack {
            env = i as f32 / attack.max(1) as f32;
        } else if i + release > len {
            let remaining = len.saturating_sub(i);
            env = remaining as f32 / release.max(1) as f32;
        }

        let s = (t * freq_hz * std::f32::consts::TAU).sin();
        let sample = (s * env * amplitude * i16::MAX as f32) as i16;
        out.push(sample);
    }
    out
}

fn synth_chiptune_wav() -> Vec<u8> {
    let sample_rate = 44_100u32;
    let bpm = 140.0;
    let step_s = 60.0 / bpm / 4.0;
    let steps = 64usize;
    let duration_s = step_s * steps as f32;
    let total_samples = (duration_s * sample_rate as f32).round() as usize;

    let melody: [i32; 64] = [
        69, 0, 72, 0, 76, 0, 72, 0, 69, 0, 67, 0, 64, 0, 67, 0, 72, 0, 76, 0, 79, 0, 76, 0, 72, 0,
        71, 0, 67, 0, 69, 0, 76, 0, 79, 0, 83, 0, 79, 0, 76, 0, 74, 0, 71, 0, 74, 0, 72, 0, 76, 0,
        79, 0, 76, 0, 72, 0, 71, 0, 67, 0, 69, 0,
    ];
    let bass: [i32; 64] = [
        45, 0, 45, 0, 48, 0, 45, 0, 43, 0, 43, 0, 40, 0, 43, 0, 45, 0, 45, 0, 48, 0, 45, 0, 43, 0,
        43, 0, 40, 0, 43, 0, 48, 0, 48, 0, 52, 0, 48, 0, 47, 0, 47, 0, 43, 0, 47, 0, 45, 0, 45, 0,
        48, 0, 45, 0, 43, 0, 43, 0, 40, 0, 43, 0,
    ];

    let drum: [u8; 64] = [
        1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 2, 0, 0, 1, 0, 1, 0, 0, 0, 2, 0, 1, 0, 1, 0, 2, 0, 0, 0,
        1, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 2, 0, 0, 1, 0, 1, 0, 0, 0, 2, 0, 1, 0, 1, 0, 2, 0,
        0, 0, 1, 0,
    ];

    let mut rng = 0x1234_5678u32;
    let sample_rate_f = sample_rate as f32;
    let mut out = Vec::with_capacity(total_samples);

    for i in 0..total_samples {
        let t = i as f32 / sample_rate_f;
        let step = ((t / step_s).floor() as usize) % steps;
        let step_pos = (t - (step as f32 * step_s)) / step_s;

        let fade_s = 0.05;
        let global_env = if t < fade_s {
            t / fade_s
        } else if duration_s - t < fade_s {
            (duration_s - t) / fade_s
        } else {
            1.0
        };

        let note_env = if step_pos < 0.08 {
            step_pos / 0.08
        } else if step_pos > 0.85 {
            (1.0 - step_pos) / 0.15
        } else {
            1.0
        };

        let mut sample = 0.0;

        let mel = melody[step];
        if mel != 0 {
            let f = midi_to_freq(mel);
            sample += square_wave(t, f) * 0.18 * note_env;
        }

        let b = bass[step];
        if b != 0 {
            let f = midi_to_freq(b);
            sample += square_wave(t, f) * 0.16 * note_env;
        }

        match drum[step] {
            1 => {
                let env = (1.0 - step_pos).powf(4.0);
                let local_t = step_pos * step_s;
                let kick_f = 60.0 + 90.0 * (1.0 - step_pos);
                sample += (local_t * kick_f * std::f32::consts::TAU).sin() * 0.25 * env;
            }
            2 => {
                let env = (1.0 - step_pos).powf(2.5);
                sample += noise(&mut rng) * 0.16 * env;
            }
            _ => {}
        }

        sample *= global_env;
        sample = sample.clamp(-1.0, 1.0);
        out.push((sample * i16::MAX as f32) as i16);
    }

    wav_pcm_mono_16(sample_rate, &out)
}

fn midi_to_freq(midi_note: i32) -> f32 {
    440.0 * 2.0_f32.powf((midi_note as f32 - 69.0) / 12.0)
}

fn square_wave(t: f32, freq_hz: f32) -> f32 {
    if (t * freq_hz).fract() < 0.5 {
        1.0
    } else {
        -1.0
    }
}

fn noise(rng: &mut u32) -> f32 {
    *rng ^= *rng << 13;
    *rng ^= *rng >> 17;
    *rng ^= *rng << 5;
    (*rng as f32 / u32::MAX as f32) * 2.0 - 1.0
}

fn wav_pcm_mono_16(sample_rate: u32, samples: &[i16]) -> Vec<u8> {
    let num_channels: u16 = 1;
    let bits_per_sample: u16 = 16;
    let block_align = num_channels * (bits_per_sample / 8);
    let byte_rate = sample_rate * block_align as u32;
    let data_size = (samples.len() as u32) * block_align as u32;
    let chunk_size = 36 + data_size;

    let mut out = Vec::with_capacity((44 + data_size) as usize);

    out.extend_from_slice(b"RIFF");
    out.extend_from_slice(&chunk_size.to_le_bytes());
    out.extend_from_slice(b"WAVE");

    out.extend_from_slice(b"fmt ");
    out.extend_from_slice(&16u32.to_le_bytes());
    out.extend_from_slice(&1u16.to_le_bytes());
    out.extend_from_slice(&num_channels.to_le_bytes());
    out.extend_from_slice(&sample_rate.to_le_bytes());
    out.extend_from_slice(&byte_rate.to_le_bytes());
    out.extend_from_slice(&block_align.to_le_bytes());
    out.extend_from_slice(&bits_per_sample.to_le_bytes());

    out.extend_from_slice(b"data");
    out.extend_from_slice(&data_size.to_le_bytes());

    for s in samples {
        out.extend_from_slice(&s.to_le_bytes());
    }

    out
}
