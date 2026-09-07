from pathlib import Path
import unreal,traceback
try:
 p=Path(__file__).with_name('scene_once.py');exec(compile(p.read_text(),str(p),'exec'),{'__file__':str(p)})
except Exception:unreal.log_error(traceback.format_exc());unreal.SystemLibrary.quit_editor()
