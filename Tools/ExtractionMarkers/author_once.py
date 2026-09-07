"""Full-editor one-off driver, removed immediately after the asset commit."""
from pathlib import Path
import unreal,traceback
folder=Path(__file__).resolve().parent
_count=0
def next_tick(dt):
 global _count
 _count+=1
 if _count<80:return
 unreal.unregister_slate_post_tick_callback(_handle)
 try:
  path=folder/'verify_unreal.py';scope={'__file__':str(path)};exec(compile(path.read_text(),str(path),'exec'),scope)
  (scope['OUT']/'unreal_import_validation.json').write_text(scope['json'].dumps(scope['report'],indent=2))
  path=folder/'scene_once.py';exec(compile(path.read_text(),str(path),'exec'),{'__file__':str(path)})
 except Exception:unreal.log_error(traceback.format_exc());unreal.SystemLibrary.quit_editor()
try:
 path=folder/'import_once.py';exec(compile(path.read_text(encoding='utf-8-sig'),str(path),'exec'),{'__file__':str(path)})
 _refs=[unreal.load_asset(p) for p in ('/Game/Environment/ModularInteriorPreview/Textures/T_MI_Atlas','/Game/Environment/ModularInteriorPreview/Textures/T_MI_DirtMask','/Game/Environment/ModularInteriorExpansion/Meshes/SM_MIE_EmergencyLight')]
 _handle=unreal.register_slate_post_tick_callback(next_tick)
except Exception:unreal.log_error(traceback.format_exc());unreal.SystemLibrary.quit_editor()
