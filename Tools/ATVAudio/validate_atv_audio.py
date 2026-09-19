"""Independent file-level QA of PCM format, headroom and loop boundaries."""
from pathlib import Path
import hashlib
import json
import struct
import wave
import numpy as np

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'TunaSweeper/SourceArt/Audio/ATV'


def db(value):
    return float(20 * np.log10(max(float(value), 1e-12)))


def inspect(path, loop):
    with wave.open(str(path), 'rb') as wav:
        channels, width, rate, frames, _, _ = wav.getparams()
        assert (channels, width, rate) == (1, 3, 48000), path.name
        data = np.frombuffer(wav.readframes(frames), dtype=np.uint8).reshape(-1, 3)
    q = data[:,0].astype(np.int32) | data[:,1].astype(np.int32)<<8 | data[:,2].astype(np.int32)<<16
    q[q >= 8388608] -= 16777216
    x = q.astype(np.float64) / 8388608
    assert np.isfinite(x).all() and len(x) == frames
    peak = float(np.max(np.abs(x)))
    assert peak < 10 ** (-2.5/20), f'{path.name}: headroom'
    assert np.max(np.abs(q)) < 8388607, f'{path.name}: clipping'
    # Four-times FFT reconstruction estimates inter-sample peaks.
    up = np.fft.irfft(np.fft.rfft(x), n=4*len(x)) * 4
    true_peak = float(np.max(np.abs(up)))
    assert true_peak < 10 ** (-2/20), f'{path.name}: inter-sample headroom'
    rms = np.sqrt(np.mean(x*x))
    dc = float(abs(np.mean(x)))
    assert dc < .0005, f'{path.name}: DC offset'
    result = dict(file=path.name, duration_seconds=frames/rate, sample_rate=rate,
                  channels=channels, bits_per_sample=width*8, peak_dbfs=db(peak),
                  estimated_true_peak_dbfs_4x=db(true_peak), rms_dbfs=db(rms),
                  dc_offset=dc, clipped_samples=int(np.sum(np.abs(q)>=8388607)),
                  sha256=hashlib.sha256(path.read_bytes()).hexdigest())
    if loop:
        jump = abs(x[0] - x[-1])
        derivative = np.abs(np.diff(x))
        typical_high = np.quantile(derivative, .99)
        assert jump < typical_high, f'{path.name}: boundary impulse'
        block = 4800
        energy = np.sqrt(np.mean(x[:len(x)//block*block].reshape(-1,block)**2,axis=1))
        seam_energy = np.sqrt(np.mean(np.r_[x[-block//2:],x[:block//2]]**2))
        seam_ratio = seam_energy / np.median(energy)
        assert .70 < seam_ratio < 1.3, f'{path.name}: boundary dropout/bump'
        raw = path.read_bytes()
        pos = 12
        found = False
        while pos + 8 <= len(raw):
            tag, size = struct.unpack('<4sI', raw[pos:pos+8])
            if tag == b'smpl':
                fields = struct.unpack('<15I', raw[pos+8:pos+8+60])
                assert fields[7] == 1 and fields[11] == 0 and fields[12] == frames-1
                found = True
            pos += 8 + size + size % 2
        assert found, f'{path.name}: missing embedded loop'
        result.update(loop_boundary_step_dbfs=db(jump), derivative_99th_percentile_dbfs=db(typical_high),
                      seam_rms_ratio_to_median=float(seam_ratio), embedded_loop_verified=True)
    else:
        assert abs(x[0]) < .001 and abs(x[-1]) < .001, f'{path.name}: one-shot fade'
    return result


def main():
    manifest = json.loads((OUT/'audio_manifest.json').read_text(encoding='utf-8'))
    reports = []
    for asset in manifest['assets']:
        report = inspect(OUT/asset['file'], asset['loop'])
        assert report['duration_seconds'] == asset['duration_seconds']
        reports.append(report)
    reports.append(inspect(OUT/manifest['preview'], False))
    report = dict(passed=True, file_count=len(reports),
                  limitations=['Objective signal checks do not replace human listening.',
                               'Four-times reconstructed peak is an estimate, not a certified BS.1770 true-peak measurement.'],
                  files=reports)
    (OUT/'audio_qa.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    for r in reports:
        print(f"{r['file']}: {r['duration_seconds']:.2f}s; peak {r['peak_dbfs']:.2f} dBFS; RMS {r['rms_dbfs']:.2f} dBFS")
    print(f'PASS: {len(reports)} files, PCM24/48kHz/mono, headroom, no clipping, DC, fades, and loop seams.')


if __name__ == '__main__':
    main()
