import unreal

task = unreal.AssetImportTask()
task.filename = r"D:/github/extraction_shooter/Tools/SourceAssets/SM_Enemy_AlertExclamation.obj"
task.destination_path = "/Game/Characters/Enemy"
task.automated = True
task.replace_existing = True
task.save = True

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

asset_path = "/Game/Characters/Enemy/SM_Enemy_AlertExclamation"
if not unreal.EditorAssetLibrary.does_asset_exist(asset_path):
    raise RuntimeError(f"Failed to import {asset_path}")

unreal.EditorAssetLibrary.save_asset(asset_path, only_if_is_dirty=False)
unreal.log(f"Imported and saved {asset_path}")
