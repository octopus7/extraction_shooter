"""Deterministic offline gasoline ATV sound design. Requires Python 3.10+ and NumPy.

This is source-audio tooling, not an Unreal Editor asset generator. It neither
launches Unreal nor imports assets. See the generated README for playback notes.
"""
from pathlib import Path
import json
import struct
import wave

import numpy as np

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "TunaSweeper/SourceArt/Audio/ATV"
SR = 48000
TAU = 2 * np.pi


def rms(x):
    return np.sqrt(np.mean(x * x))


def shaped_noise(n, seed, low, high):
    """FFT filtering makes the noise periodic across the whole buffer."""
    rng = np.random.default_rng(seed)
    f = np.fft.rfftfreq(n, 1 / SR)
    response = (f / np.sqrt(f * f + low * low)) ** 2
    response *= 1 / np.sqrt(1 + (f / high) ** 8)
    y = np.fft.irfft(np.fft.rfft(rng.normal(size=n)) * response, n=n)
    return y / rms(y)


def filter_exhaust(x):
    f = np.fft.rfftfreq(len(x), 1 / SR)
    response = 0.28 + 1.1 / (1 + ((f - 112) / 55) ** 2)
    response += 0.8 / (1 + ((f - 255) / 110) ** 2)
    response += 0.32 / (1 + ((f - 570) / 190) ** 2)
    response *= f * f / (f * f + 28 ** 2)
    response *= 1 / np.sqrt(1 + (f / 2600) ** 6)
    return np.fft.irfft(np.fft.rfft(x) * response, n=len(x))


def engine(rpm, seed, load=0.5, periodic=False):
    """Single-cylinder four-stroke: one firing per two crankshaft revolutions.

    Includes uneven firing energy, exhaust formants, intake turbulence, valve
    ticks and twice-firing-frequency crank mechanics. No recorded material.
    """
    n = len(rpm)
    t = np.arange(n) / SR
    duration = n / SR
    cycles = np.cumsum(rpm / (120 * SR))
    if periodic:
        cycles *= round(cycles[-1]) / cycles[-1]
    p = TAU * cycles
    slow = (0.045 * np.sin(TAU * 3 * t / duration + 0.7)
            + 0.022 * np.sin(TAU * 11 * t / duration))
    # A short pressure rise and longer decay creates the exhaust's uneven growl.
    pressure = np.zeros(n)
    for harmonic in range(1, 65):
        pressure += np.cos(harmonic * p - 0.32 * np.log1p(harmonic)) * (
            np.exp(-harmonic / (12 + 12 * load)) / harmonic ** 0.48
        )
    pressure *= 1 + slow + 0.045 * np.sin(0.5 * p)
    exhaust = filter_exhaust(np.tanh(pressure * (0.75 + load * 0.3)))
    exhaust /= rms(exhaust)
    pulse = (0.5 + 0.5 * np.cos(p - 0.4)) ** 6
    intake = shaped_noise(n, seed, 120, 2400) * (0.32 + 0.68 * pulse)
    valves = shaped_noise(n, seed + 1, 1400, 6500)
    valves *= (0.5 + 0.5 * np.cos(p * 2 + 0.9)) ** 20
    crank = (np.sin(p * 2 + 0.35 * np.sin(p))
             + 0.25 * np.sin(p * 6))
    y = exhaust + (0.17 + 0.15 * load) * intake + 0.10 * valves + 0.075 * crank
    y = np.tanh(y * 0.57)
    # Periodic high-pass / low-pass also removes any soft-clip DC offset.
    f = np.fft.rfftfreq(n, 1 / SR)
    response = f * f / (f * f + 24 ** 2) / np.sqrt(1 + (f / 7000) ** 8)
    return np.fft.irfft(np.fft.rfft(y) * response, n=n)


def level(x, target_db):
    x = x.copy()
    x *= 10 ** (target_db / 20) / rms(x)
    ceiling = 10 ** (-3 / 20)
    if np.max(np.abs(x)) > ceiling:
        x *= ceiling / np.max(np.abs(x))
    return x


def fade(x, start=0.012, end=0.060):
    x = x.copy()
    a, b = round(start * SR), round(end * SR)
    if a:
        x[:a] *= np.sin(np.linspace(0, np.pi / 2, a)) ** 2
    if b:
        x[-b:] *= np.cos(np.linspace(0, np.pi / 2, b)) ** 2
    return x


def finish_one_shot(x, start=0.012, end=0.060):
    # Changing RPM/envelopes can create subsonic drift. Remove it after all
    # mixing, with zero padding so FFT filtering cannot wrap an event's tail
    # back to its beginning. Mean subtraction alone makes silent tails nonzero.
    padded = np.pad(x, (SR, SR))
    f = np.fft.rfftfreq(len(padded), 1 / SR)
    response = f*f / (f*f + 30**2)
    clean = np.fft.irfft(np.fft.rfft(padded) * response, n=len(padded))[SR:-SR]
    return fade(clean, start, end)


def event(n, when, duration, seed, tone=140, low=90, high=2200):
    count = round(duration * SR)
    t = np.arange(count) / SR
    y = 0.55 * shaped_noise(count, seed, low, high) + np.sin(TAU * tone * t)
    y *= (1 - np.exp(-t / 0.0015)) * np.exp(-t / (duration / 7))
    y = fade(y, 0.002, 0.025)
    out = np.zeros(n)
    start = round(when * SR)
    stop = min(n, start + count)
    out[start:stop] += y[:stop-start]
    return out


def write_wav(path, x, loop=False):
    """PCM24 with triangular dither; loops include a standard RIFF smpl chunk."""
    rng = np.random.default_rng(20260919)
    q = np.rint(x * 8388607 + rng.random(len(x)) - rng.random(len(x))).astype(np.int32)
    # Silence remains exact at one-shot ends.
    q[x == 0] = 0
    data = np.column_stack((q & 255, (q >> 8) & 255, (q >> 16) & 255)).astype(np.uint8).tobytes()
    fmt = struct.pack('<HHIIHH', 1, 1, SR, SR * 3, 3, 24)
    chunks = b'fmt ' + struct.pack('<I', len(fmt)) + fmt
    chunks += b'data' + struct.pack('<I', len(data)) + data
    if len(data) % 2:
        chunks += b'\x00'
    if loop:
        # Inclusive sample-loop end, infinite forward playback.
        smpl = struct.pack('<9I', 0, 0, round(1e9 / SR), 60, 0, 0, 0, 1, 0)
        smpl += struct.pack('<6I', 0, 0, 0, len(x) - 1, 0, 0)
        chunks += b'smpl' + struct.pack('<I', len(smpl)) + smpl
    path.write_bytes(b'RIFF' + struct.pack('<I', len(chunks) + 4) + b'WAVE' + chunks)


def crossfade(a, b, seconds):
    n = round(seconds * SR)
    w = np.linspace(0, 1, n)
    # Equal-power fades for independently phased engine states.
    return np.concatenate((a[:-n], a[-n:] * np.cos(w*np.pi/2)
                           + b[:n] * np.sin(w*np.pi/2), b[n:]))


def render():
    OUT.mkdir(parents=True, exist_ok=True)
    assets = []
    signals = {}

    def save(name, x, usage, loop=False, rpm=None, **extra):
        write_wav(OUT / (name + '.wav'), x, loop)
        signals[name] = x
        assets.append(dict(file=name + '.wav', duration_seconds=len(x)/SR,
                           sample_rate=SR, channels=1, bits_per_sample=24,
                           loop=loop, loop_start_sample=0 if loop else None,
                           loop_end_sample_inclusive=len(x)-1 if loop else None,
                           nominal_rpm=rpm, intended_playback=usage, **extra))

    for name, rpm, db, load, seed in [
        ('SW_ATV_Idle_Loop', 1560, -21, 0.15, 41),
        ('SW_ATV_Drive_Loop', 4200, -19, 0.62, 51),
        ('SW_ATV_Boost_Loop', 6840, -17, 0.97, 61),
    ]:
        t = np.arange(SR * 8) / SR
        # All modulation has integer cycles across the eight-second loop.
        varying = rpm * (1 + 0.007*np.sin(TAU*3*t/8) + 0.003*np.sin(TAU*13*t/8))
        x = level(engine(varying, seed, load, periodic=True), db)
        # Rotate the periodic signal to its gentlest zero crossing. This keeps
        # direct first playback quiet without a loop-boundary fade or dropout.
        crossings = np.flatnonzero(x[:-1] * x[1:] < 0) + 1
        cost = np.maximum(np.abs(x[crossings]), np.abs(x[crossings-1]))
        x = np.roll(x, -int(crossings[np.argmin(cost)]))
        save(name, x, 'Continuous engine state; crossfade with adjacent engine loops over 0.25-0.40 seconds.',
             loop=True, rpm=rpm, transition_crossfade_seconds=0.3)

    t = np.arange(round(SR * 3.8)) / SR
    n = len(t)
    rpm = np.interp(t, [0, .22, .93, 1.12, 1.40, 2.15, 3.8],
                    [180, 280, 300, 1100, 2200, 1560, 1560])
    body = engine(rpm, 71, .35)
    amplitude = np.interp(t, [0, .15, .80, .91, 1.16, 1.45, 2.1, 3.8],
                          [0, .10, .13, .12, .85, 1, .68, .68])
    starter_phase = TAU * np.cumsum(np.interp(t, [0,.9,1.2,3.8], [70,95,110,110])) / SR
    starter = (np.sin(starter_phase) + .25*np.sin(3*starter_phase))
    starter += .22 * shaped_noise(n, 72, 350, 2600)
    starter *= np.interp(t, [0,.08,.20,.90,1.15,1.22,3.8], [0,0,.24,.28,.15,0,0])
    start = body * amplitude + starter
    start += .18 * event(n, .045, .12, 73, 420, 500, 5500)
    start += .25 * event(n, 1.03, .18, 74, 95)
    start = level(start, -20)
    # Tail explicitly becomes the same idle recording used by the runtime loop.
    tail_n = round(.8 * SR)
    w = np.linspace(0,1,tail_n)
    idle = signals['SW_ATV_Idle_Loop']
    start[-tail_n:] = start[-tail_n:] * np.cos(w*np.pi/2) + idle[:tail_n] * np.sin(w*np.pi/2)
    start = finish_one_shot(start, .01, .06)
    save('SW_ATV_Mount_Start', start,
         'Mounting triggers starter crank, ignition catch, rev settle. Crossfade to Idle at 3.50 seconds for 0.30 seconds.',
         transition_to='SW_ATV_Idle_Loop.wav', transition_start_seconds=3.5,
         transition_crossfade_seconds=.3)

    t = np.arange(round(SR * 2.4)) / SR
    n = len(t)
    rpm = np.interp(t, [0,.28,.48,1.15,1.65,2.4], [1560,1560,1350,630,90,0])
    stop = engine(rpm, 81, .15)
    stop *= np.interp(t, [0,.18,.35,.65,1.1,1.55,1.78,2.4], [.62,.62,.60,.5,.32,.12,0,0])
    stop += .09 * event(n, .31, .10, 82, 460, 700, 4600)
    stop += .12 * event(n, 1.58, .30, 83, 155, 100, 1300)
    stop = finish_one_shot(level(stop, -23), .03, .14)
    save('SW_ATV_Dismount_Stop', stop,
         'Use when dismounting also switches the engine off; fade out the active loop over 0.18 seconds while playing this shutdown once.',
         transition_crossfade_seconds=.18)

    n = round(SR * .95)
    dismount = .28*event(n, .08, .38, 91, 120, 120, 3000)
    dismount += .20*event(n, .32, .36, 92, 75, 60, 1500)
    dismount += .13*event(n, .50, .32, 93, 180, 350, 4200)
    dismount = finish_one_shot(level(dismount, -25), .012, .08)
    save('SW_ATV_Dismount_Mechanical', dismount,
         'Optional engine-on dismount: footrest/seat suspension movement only. Keep Idle running; do not play Dismount_Stop.',
         optional=True)

    preview = start
    for name, seconds, blend in [
        ('SW_ATV_Idle_Loop', 4.0, .3), ('SW_ATV_Drive_Loop', 5.0, .4),
        ('SW_ATV_Boost_Loop', 5.0, .4), ('SW_ATV_Drive_Loop', 2.0, .4),
        ('SW_ATV_Idle_Loop', 2.0, .4), ('SW_ATV_Dismount_Stop', 2.4, .18),
    ]:
        preview = crossfade(preview, signals[name][:round(seconds*SR)], blend)
    write_wav(OUT / 'ATV_Audition_Sequence.wav', preview)
    manifest = dict(version=1, origin='Original deterministic procedural synthesis; not field recordings or generated by a sound model.',
                    engine_design='Single-cylinder four-stroke gasoline ATV',
                    generator='Tools/ATVAudio/render_atv_audio.py',
                    audible_review='Not performed: no audio listening/understanding tool available to the agent. Human audition required.',
                    assets=assets, preview='ATV_Audition_Sequence.wav')
    (OUT / 'audio_manifest.json').write_text(json.dumps(manifest, indent=2)+'\n', encoding='utf-8')
    print(f'Rendered {len(assets)} assets and audition sequence to {OUT}')


if __name__ == '__main__':
    render()
