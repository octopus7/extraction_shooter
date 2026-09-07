"""Read-only bootstrap; retain namespace for delayed full-editor physics audit."""
from pathlib import Path
import json
import traceback
import unreal

_script = Path(__file__).with_name('verify_unreal_map.py')
_namespace = {'__file__': str(_script)}
try:
    exec(compile(_script.read_text(encoding='utf-8'), str(_script), 'exec'), _namespace)
except Exception:
    _error = traceback.format_exc()
    unreal.log_error(_error)
    _out = Path(__file__).resolve().parents[2] / 'TunaSweeper/SourceArt/Environment/LabSupplyProps'
    (_out / 'unreal_map_reload_validation.json').write_text(json.dumps({
        'passed': False, 'phase': 'map_load_or_static_audit', 'error': _error
    }, indent=2), encoding='utf-8')
    unreal.SystemLibrary.quit_editor()
