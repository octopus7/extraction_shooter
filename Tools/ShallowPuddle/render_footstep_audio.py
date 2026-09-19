"""Offline source-audio synthesis, not an Unreal asset generator. Requires NumPy.

Creates an original short shallow-water footstep from pressure, turbulent spray,
and damped bubble resonances. No recordings or third-party samples are used.
"""
from pathlib import Path
import hashlib
import json
import wave
import numpy as np

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "TunaSweeper/SourceArt/Audio/ShallowPuddle"
RATE = 48000
DURATION = 0.42
TAU = 2 * np.pi


def band_noise(length, rng, low, high):
    # Pad before filtering so the event's tail cannot wrap to its onset.
    size = length + 8192
    freq = np.fft.rfftfreq(size, 1 / RATE)
    response = freq ** 2 / (freq ** 2 + low ** 2)
    response /= np.sqrt(1 + (freq / high) ** 8)
    noise = np.fft.irfft(np.fft.rfft(rng.normal(size=size)) * response, n=size)
    noise = noise[4096:4096 + length]
    return noise / np.sqrt(np.mean(noise * noise))


def render():
    rng = np.random.default_rng(2026091907)
    count = round(DURATION * RATE)
    t = np.arange(count) / RATE
    # Soft sole impact underneath the water, followed by a broad wet slap.
    body = np.sin(TAU * (125 * t - 50 * t * t))
    body *= (1 - np.exp(-t / 0.003)) * np.exp(-t / 0.028)
    splash = band_noise(count, rng, 260, 5300)
    splash *= (1 - np.exp(-t / 0.0018)) * np.exp(-t / 0.045)
    liquid = band_noise(count, rng, 110, 1800)
    liquid *= (1 - np.exp(-t / 0.012)) * np.exp(-t / 0.080)
    x = 0.32 * body + 0.50 * splash + 0.30 * liquid
    # Short, irregular resonances add liquid texture without a sustained note.
    for when in (0.024, 0.041, 0.067, 0.092, 0.128, 0.165, 0.217, 0.276):
        start = round((when + rng.uniform(-0.004, 0.004)) * RATE)
        remaining = count - start
        q = np.arange(remaining) / RATE
        frequency = rng.uniform(650, 2450)
        decay = rng.uniform(0.006, 0.014)
        phase = TAU * frequency * (q + 2.5 * q * q)
        envelope = (1 - np.exp(-q / 0.0009)) * np.exp(-q / decay)
        amplitude = rng.uniform(0.06, 0.14) * np.exp(-when / 0.17)
        bubble = np.sin(phase) + 0.22 * band_noise(remaining, rng, 700, 7500)
        x[start:] += amplitude * bubble * envelope
    # Remove subsonic drift after mixing, then fade both ends before PCM export.
    padded = np.pad(x, (4096, 4096))
    freq = np.fft.rfftfreq(len(padded), 1 / RATE)
    response = freq ** 2 / (freq ** 2 + 45 ** 2)
    x = np.fft.irfft(np.fft.rfft(padded) * response, n=len(padded))[4096:-4096]
    fade_in, fade_out = round(0.002 * RATE), round(0.055 * RATE)
    x[:fade_in] *= np.sin(np.linspace(0, np.pi / 2, fade_in)) ** 2
    x[-fade_out:] *= np.cos(np.linspace(0, np.pi / 2, fade_out)) ** 2
    x *= 10 ** (-4 / 20) / np.max(np.abs(x))
    x[0] = x[-1] = 0
    pcm = np.rint(x * 8388607 + rng.random(count) - rng.random(count)).astype(np.int32)
    pcm[0] = pcm[-1] = 0
    packed = np.column_stack((pcm & 255, (pcm >> 8) & 255, (pcm >> 16) & 255)).astype(np.uint8)
    OUT.mkdir(parents=True, exist_ok=True)
    target = OUT / "SW_ShallowPuddle_Footstep.wav"
    with wave.open(str(target), "wb") as wav:
        wav.setnchannels(1)
        wav.setsampwidth(3)
        wav.setframerate(RATE)
        wav.writeframes(packed.tobytes())
    print(target)
    validate()


def validate():
    target = OUT / "SW_ShallowPuddle_Footstep.wav"
    with wave.open(str(target), "rb") as wav:
        assert (wav.getnchannels(), wav.getsampwidth(), wav.getframerate()) == (1, 3, RATE)
        count = wav.getnframes()
        assert count == round(DURATION * RATE)
        b = np.frombuffer(wav.readframes(count), dtype=np.uint8).reshape(-1, 3)
    pcm = b[:, 0].astype(np.int32) | (b[:, 1].astype(np.int32) << 8) | (b[:, 2].astype(np.int32) << 16)
    pcm[pcm >= 8388608] -= 16777216
    x = pcm.astype(np.float64) / 8388608
    peak = np.max(np.abs(x))
    rms = np.sqrt(np.mean(x * x))
    true_peak = np.max(np.abs(np.fft.irfft(np.fft.rfft(x), n=4 * count) * 4))
    assert peak < 10 ** (-3.9 / 20), "Insufficient sample headroom"
    assert true_peak < 10 ** (-2.5 / 20), "Insufficient reconstructed-peak headroom"
    assert -32 < 20 * np.log10(rms) < -13, "Unexpected silent or over-loud source"
    assert abs(x.mean()) < 0.0005, "DC offset"
    assert x[0] == x[-1] == 0, "Click-free endpoint requirement"
    assert np.max(np.abs(x[-480:])) < 0.001, "Tail does not decay to silence"
    report = dict(passed=True, file=target.name, duration_seconds=DURATION,
                  sample_rate=RATE, channels=1, bits_per_sample=24, looping=False,
                  peak_dbfs=float(20 * np.log10(peak)), rms_dbfs=float(20 * np.log10(rms)),
                  estimated_true_peak_dbfs_4x=float(20 * np.log10(true_peak)),
                  clipped_samples=int(np.sum(np.abs(pcm) >= 8388607)),
                  dc_offset=float(abs(x.mean())), sha256=hashlib.sha256(target.read_bytes()).hexdigest(),
                  provenance="Original deterministic synthesis; no sampled recordings.",
                  validation="Signal/file checks only; not a human listening acceptance test.")
    (OUT / "audio_qa.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    import sys
    validate() if "--verify" in sys.argv else render()
