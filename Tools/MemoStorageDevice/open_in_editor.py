"""UI-only convenience script; does not save assets or change a level."""
import unreal
path='/Game/Meshes/Props/MemoStorageDevice/SM_MemoStorageDevice'
mesh=unreal.load_asset(path)
assert isinstance(mesh,unreal.StaticMesh)
unreal.EditorAssetLibrary.sync_browser_to_objects([path])
editor=unreal.get_editor_subsystem(unreal.AssetEditorSubsystem)
assert editor.open_editor_for_assets([mesh])
unreal.log('MEMO_DEVICE_ASSET_EDITOR_OPENED')
