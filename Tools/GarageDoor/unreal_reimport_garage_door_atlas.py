"""Run inside Unreal Editor to reimport only the garage-door base-color atlas."""

from pathlib import Path

import unreal


ROOT = Path(__file__).resolve().parents[2]
SOURCE_PATH = (
    ROOT
    / "TunaSweeper"
    / "SourceArt"
    / "Environment"
    / "BunkerGarageDoor"
    / "Textures"
    / "T_GarageDoor_PartsAtlas_BaseColor.png"
)

task = unreal.AssetImportTask()
task.filename = str(SOURCE_PATH)
task.destination_path = "/Game/Environment/Bunker/GarageDoor/Textures"
task.destination_name = "T_GarageDoor_PartsAtlas_BaseColor"
task.automated = True
task.replace_existing = True
task.replace_existing_settings = False
task.save = True

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
if not task.imported_object_paths:
    raise RuntimeError("Garage-door atlas import returned no asset paths")

unreal.log("GARAGE_ATLAS_IMPORTED=" + ",".join(task.imported_object_paths))
