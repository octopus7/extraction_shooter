from pathlib import Path
import unreal,traceback,json
try:
    p=Path(__file__).with_name('verify_unreal_assets.py');exec(compile(p.read_text(),str(p),'exec'),{'__file__':str(p)})
    p=Path(__file__).with_name('verify_review.py');exec(compile(p.read_text(),str(p),'exec'),{'__file__':str(p)})
except Exception:
    err=traceback.format_exc();unreal.log_error(err)
    out=Path(__file__).resolve().parents[2]/'TunaSweeper/SourceArt/Environment/LootContainerSet/unreal_review_validation.json';out.write_text(json.dumps({'passed':False,'error':err},indent=2))
    unreal.SystemLibrary.quit_editor()
