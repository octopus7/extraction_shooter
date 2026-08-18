"""Reimport the three garage-door frame meshes and two deployable rail meshes."""

from pathlib import Path

import unreal


ROOT = Path(__file__).resolve().parents[2]
SOURCE_DIR = (
    ROOT
    / "TunaSweeper"
    / "SourceArt"
    / "Environment"
    / "BunkerGarageDoor"
    / "Models"
)
FRAME_AND_RAIL_MESH_NAMES = (
    "SM_GarageDoor_FrameTop",
    "SM_GarageDoor_FrameLeft",
    "SM_GarageDoor_FrameRight",
    "SM_GarageDoor_CanopyRailLeft",
    "SM_GarageDoor_CanopyRailRight",
)

tasks = []
for mesh_name in FRAME_AND_RAIL_MESH_NAMES:
    task = unreal.AssetImportTask()
    task.filename = str(SOURCE_DIR / f"{mesh_name}.obj")
    task.destination_path = "/Game/Environment/Bunker/GarageDoor/Meshes"
    task.destination_name = mesh_name
    task.automated = True
    task.replace_existing = True
    task.replace_existing_settings = False
    task.save = True
    tasks.append(task)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for task in tasks:
    if not task.imported_object_paths:
        raise RuntimeError(f"Frame/rail mesh import returned no asset path: {task.filename}")
    unreal.log("GARAGE_FRAME_RAIL_IMPORTED=" + ",".join(task.imported_object_paths))
