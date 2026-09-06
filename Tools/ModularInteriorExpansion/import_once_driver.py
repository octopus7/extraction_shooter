from pathlib import Path
import unreal,traceback
s=Path(__file__).with_name('import_unreal_once.py')
try:exec(compile(s.read_text(),str(s),'exec'),{'__file__':str(s)})
except Exception:unreal.log_error(traceback.format_exc())
unreal.SystemLibrary.quit_editor()
