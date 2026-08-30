import os
import unreal


PROJECT_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SOURCE_PATH = os.path.join(
    PROJECT_DIR,
    "Content",
    "SourceArt",
    "Robot",
    "T_Robot_BodyColorMask.png",
)
DESTINATION_PATH = "/Game/Characters/Robot/Materials"
ASSET_NAME = "T_Robot_BodyColorMask"
ASSET_PATH = f"{DESTINATION_PATH}/{ASSET_NAME}"


if not os.path.isfile(SOURCE_PATH):
    raise RuntimeError(f"Mask source PNG does not exist: {SOURCE_PATH}")

task = unreal.AssetImportTask()
task.filename = SOURCE_PATH
task.destination_path = DESTINATION_PATH
task.destination_name = ASSET_NAME
task.automated = True
task.replace_existing = True
task.replace_existing_settings = True
task.save = False

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
texture = unreal.load_asset(ASSET_PATH)
if texture is None:
    raise RuntimeError(f"Could not import mask texture: {ASSET_PATH}")

texture.set_editor_property("srgb", False)
texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_CHARACTER)
texture.modify()

if not unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False):
    raise RuntimeError(f"Could not save mask texture: {ASSET_PATH}")

unreal.log(f"ROBOT_COLOR_MASK_ASSET={ASSET_PATH}")
unreal.log(f"ROBOT_COLOR_MASK_SRGB={texture.get_editor_property('srgb')}")
unreal.log(
    f"ROBOT_COLOR_MASK_COMPRESSION={texture.get_editor_property('compression_settings')}"
)
