"""Verify actual iteration pixels; fail if surface detail does not contribute."""
import hashlib
import json
from pathlib import Path
import re
import sys
import numpy as np
from PIL import Image
from png_integrity import validate_png

root = Path(sys.argv[1])
manifest = json.loads((root / 'manifest.json').read_text(encoding='utf-8'))
report = {'status': 'running'}
try:
    assert manifest['status'] == 'captured' and manifest['saved_assets_unchanged']
    archived_log = root / 'render.log'
    if not archived_log.is_file():
        source_log = Path(manifest['log_file'])
        log_bytes = source_log.read_bytes()
        assert b'Log file closed' in log_bytes, 'Wait for the renderer to exit before analysis'
        archived_log.write_bytes(log_bytes)
    log = archived_log.read_text(encoding='utf-8', errors='replace')
    report['render_log_sha256'] = hashlib.sha256(archived_log.read_bytes()).hexdigest()
    assert not any(marker in log for marker in ['Failed to compile Material', 'LogPython: Error:', 'Critical error:']), 'Render process contains an error'
    report['shader_compile_errors'] = 0
    captures = {c['view']: c for c in manifest['captures']}
    required_views = {'wide', 'close', 'close-repeat', 'close-t3', 'close-detail-off',
        'close-caustics-off', 'low', 'surface', 'surface-off', 'mask'}
    if manifest.get('schema_version', 1) >= 2:
        required_views.add('close-glint-off')
    assert required_views.issubset(captures) and len(captures) == len(manifest['captures'])
    for capture in captures.values():
        path = root / capture['file'].replace('\\', '/')
        assert re.fullmatch(r'\d{8}-\d{4}\.png', path.name), path
        assert hashlib.sha256(path.read_bytes()).hexdigest() == capture['sha256']
        validate_png(path)
    def pixels(view):
        with Image.open(root / captures[view]['file'].replace('\\', '/')) as im:
            return np.asarray(im.convert('RGB'), dtype=np.float32)
    close = pixels('close')
    repeat = pixels('close-repeat')
    surface = pixels('surface')
    surface_off = pixels('surface-off')
    report['freeze_repeat_mean_error'] = float(np.abs(close-repeat).mean())
    report['surface_detail_mean_difference'] = float(np.abs(surface-surface_off).mean())
    report['surface_detail_changed_pixels'] = int((np.abs(surface-surface_off).mean(axis=2)>2).sum())
    report['caustics_mean_difference'] = float(np.abs(close-pixels('close-caustics-off')).mean())
    if 'close-glint-off' in captures:
        report['sun_glint_mean_difference'] = float(np.abs(close-pixels('close-glint-off')).mean())
    report['time_changed_pixels'] = int((np.abs(close-pixels('close-t3')).mean(axis=2)>2).sum())
    assert report['freeze_repeat_mean_error'] < 0.5, 'Frozen surface is unstable'
    assert report['surface_detail_mean_difference'] > 0.5 and report['surface_detail_changed_pixels'] > 2000, 'Surface detail is absent'
    assert report['caustics_mean_difference'] > 0.03, 'Shallow light layer is absent'
    if 'close-glint-off' in captures:
        assert report['sun_glint_mean_difference'] > 0.03, 'Sun glint is absent in the review view'
    else:
        report['sun_glint_validation'] = 'not_captured_in_earlier_iteration'
    assert report['time_changed_pixels'] > 500, 'Water does not animate'
    for view in ['wide', 'close', 'low', 'surface']:
        frame = pixels(view)
        assert frame.shape == (800,1280,3)
        assert frame.mean() > 25, 'Dark or fallback frame: '+view
        report[view+'_near_white_fraction'] = float((frame.min(axis=2)>248).mean())
        assert report[view+'_near_white_fraction'] < 0.12, 'Excessively white surface: '+view
    report['status'] = 'passed'
except Exception as error:
    report.update(status='failed', error=str(error))
    raise
finally:
    (root / 'analysis.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps(report, indent=2))
