"""Inspect actual GPU pixels, without reimplementing the foam shader.

Requires Python, Pillow and NumPy. Optional first argument: capture directory.
"""
import json
from pathlib import Path
import sys

import numpy as np
from PIL import Image
from png_integrity import validate_png

root = Path(__file__).resolve().parents[3]
out = Path(sys.argv[1]) if len(sys.argv) > 1 else root / 'Saved/ShorelineReferenceWork/Review'
report = {'status': 'running'}
checked_pngs = set()

def pixels(name):
    if name not in checked_pngs:
        validate_png(out / name)
        checked_pngs.add(name)
    with Image.open(out / name) as image:
        return np.asarray(image.convert('RGB'), dtype=np.float32)

try:
    capture = json.loads((out / 'capture_validation.json').read_text(encoding='utf-8'))
    assert capture['status'] == 'captured_pending_pixel_validation', capture
    assert capture['saved_assets_unchanged'] and capture['required_captures_exist']
    for entry in capture['captures']:
        validate_png(out / entry['name'])
        checked_pngs.add(entry['name'])
    masks = [pixels(n) for n in ['Mask_0.png', 'Mask_1.png', 'Mask_1_repeat.png', 'Mask_disabled.png']]
    m0, m1, repeat, disabled = [m.mean(axis=2) for m in masks]
    assert m0.shape == (1024, 1024)
    report['mask_peak'] = float(max(m0.max(), m1.max()))
    report['foam_pixels_t0'] = int((m0 > 8).sum())
    report['foam_pixels_t1'] = int((m1 > 8).sum())
    report['disabled_max'] = float(disabled.max())
    report['time_changed_pixels'] = int((np.abs(m0 - m1) > 5).sum())
    report['freeze_repeat_mean_error'] = float(np.abs(m1 - repeat).mean())
    assert report['mask_peak'] > 80 and min(report['foam_pixels_t0'], report['foam_pixels_t1']) > 500
    assert report['disabled_max'] <= 3, 'Foam disable still renders visible pixels'
    assert report['time_changed_pixels'] > 500, 'Foam does not animate'
    assert report['freeze_repeat_mean_error'] < .5, 'Frozen phase is unstable'
    baseline = pixels('Mask_1_breakup_off.png').mean(axis=2)
    with Image.open(root / 'Plugins/ShorelineReference/Resources/Validation/BeforeTailBreakup/Mask_1.png') as image:
        previous_mask = np.asarray(image.convert('RGB'), dtype=np.float32).mean(axis=2)
    report['breakup_off_matches_previous_mean_error'] = float(np.abs(baseline - previous_mask).mean())
    report['breakup_changed_pixels'] = int((baseline - m1 > 5).sum())
    report['tail_removed_pixels'] = int(((baseline > 8) & (m1 <= 8)).sum())
    bright_front = baseline > 240
    assert bright_front.any(), 'No bright front to compare'
    report['bright_front_mean_error'] = float(np.abs(baseline - m1)[bright_front].mean())
    assert report['breakup_off_matches_previous_mean_error'] < .05, 'Strength zero does not restore the original shader'
    assert report['breakup_changed_pixels'] > 500 and report['tail_removed_pixels'] > 100, 'Tail breakup not visible'
    assert report['bright_front_mean_error'] < .5, 'Bright front was eroded'

    # Orthographic camera yaw=0, pitch=-90: screen right is UE +Y, screen up +X.
    v, u = np.mgrid[0:1024, 0:1024]
    x = 4096 - (v + .5) * 8
    source_y = 4096 - (u + .5) * 8  # UE FBX Y is mirrored.
    theta = np.arctan2(source_y, x)
    radius = 1830 + 140*np.sin(3*(theta+.12)) + 95*np.cos(2*(theta-.4)) + 45*np.cos(5*(theta+.3))
    distance = np.hypot(x, source_y) - radius
    # Only geometry, not a copy of the shader: 1:10 beach and its analytic coastline.
    # Two texture pixels of tolerance cover raster and bilinear sampling at the boundary.
    forbidden = (distance < -16) | (distance > 330)
    visible = (m0 > 8) | (m1 > 8)
    report['foam_outside_shallow_water_pixels'] = int((visible & forbidden).sum())
    assert report['foam_outside_shallow_water_pixels'] == 0, 'Mask is not attached to the shallow shoreline'

    for name in ['Wide.png', 'Close_0.png', 'Close_1.png', 'Close_2.png']:
        frame = pixels(name)
        mean = float(frame.mean())
        report[name + '_mean_rgb'] = mean
        assert mean > 25 and (frame.mean(axis=2) > 12).mean() > .8, 'Dark or fallback render: ' + name
    previous = None
    moving_frames = 0
    for index in range(240):
        frame = pixels('Motion_%04d.png' % index)
        assert frame.shape == (800, 1280, 3)
        if previous is not None and np.abs(frame - previous).mean() > .001:
            moving_frames += 1
        previous = frame
    report['motion_frames'] = 240
    report['changed_frame_pairs'] = moving_frames
    assert moving_frames > 220
    log = (out.parent / 'Render.log').read_text(encoding='utf-8', errors='replace')
    assert 'Failed to compile Material' not in log and 'LogPython: Error:' not in log
    report['shader_compile_errors'] = 0
    report['status'] = 'passed'
except Exception as error:
    report.update(status='failed', error=str(error))
    raise
finally:
    report['png_integrity_checked_files'] = len(checked_pngs)
    (out / 'pixel_validation.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps(report, indent=2))
