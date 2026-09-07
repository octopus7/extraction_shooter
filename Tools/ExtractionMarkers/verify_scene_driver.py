"""Load persisted review map, allow component/physics registration, audit, quit."""
from pathlib import Path
import unreal,json,traceback

script=Path(__file__).with_name('verify_scene.py')
output=Path(__file__).resolve().parents[2]/'TunaSweeper/SourceArt/Environment/ExtractionMarkers/unreal_scene_reload_validation.json'
asset_script=Path(__file__).with_name('verify_unreal.py')
asset_output=output.with_name('unreal_reload_validation.json')
scope={'__file__':str(script),'__name__':'extraction_scene_audit'}
_ticks=0
_busy=False
_handle=None


def finish(error=None):
    if _handle is not None:unreal.unregister_slate_post_tick_callback(_handle)
    if error:
        unreal.log_error(error)
        output.write_text(json.dumps({'passed':False,'mode':'fresh editor reload, read-only',
                                     'error':error,'editor_ticks':_ticks},indent=2),encoding='utf-8')
        asset_output.write_text(json.dumps({'passed':False,'mode':'combined fresh editor asset/scene audit',
                                           'error':error,'editor_ticks':_ticks},indent=2),encoding='utf-8')
    unreal.SystemLibrary.quit_editor()


def tick(dt):
    global _ticks,_busy
    if _busy:return
    _ticks+=1
    if _ticks<45:return
    _busy=True
    try:
        scope['validate']()
        # Keep its own __file__ so all source and report paths resolve normally.
        exec(compile(asset_script.read_text(encoding='utf-8'),str(asset_script),'exec'),
             {'__file__':str(asset_script),'__name__':'extraction_asset_audit'})
        for path in (output,asset_output):
            result=json.loads(path.read_text(encoding='utf-8'))
            assert result['passed'],str(path)+' did not pass'
            result['combined_fresh_editor_audit']=True
            result['editor_ticks_before_audit']=_ticks
            path.write_text(json.dumps(result,indent=2),encoding='utf-8')
        finish()
    except Exception:finish(traceback.format_exc())


try:
    exec(compile(script.read_text(encoding='utf-8'),str(script),'exec'),scope)
    asset_output.write_text(json.dumps({'passed':False,'mode':'combined fresh editor asset/scene audit',
                                       'state':'started, awaiting validation'},indent=2),encoding='utf-8')
    for asset in ('/Game/Interaction/ExtractionMarkers/Textures/T_EM_Atlas',
                  '/Game/Environment/ModularInteriorPreview/Textures/T_MI_Atlas',
                  '/Game/Environment/ModularInteriorPreview/Textures/T_MI_DirtMask'):
        assert unreal.load_asset(asset),asset
    scope['prepare']()
    _handle=unreal.register_slate_post_tick_callback(tick)
except Exception:finish(traceback.format_exc())
