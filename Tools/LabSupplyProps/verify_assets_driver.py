"""Read-only full-editor asset audit; avoids commandlet-only Niagara startup ensure."""
from pathlib import Path
import unreal,traceback
try:
    p=Path(__file__).with_name('verify_unreal_assets.py')
    exec(compile(p.read_text(encoding='utf-8-sig'),str(p),'exec'),{'__file__':str(p)})
except Exception:unreal.log_error(traceback.format_exc())
unreal.SystemLibrary.quit_editor()
