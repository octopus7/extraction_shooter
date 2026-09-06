"""Console bootstrap keeps the editor alive for the post-load physics verification callback."""
from pathlib import Path
import unreal,traceback
_script=Path(__file__).with_name('build_preview_map.py')
try:exec(compile(_script.read_text(encoding='utf-8'),str(_script),'exec'),{'__file__':str(_script)})
except Exception:
    unreal.log_error(traceback.format_exc());unreal.SystemLibrary.quit_editor()
