import os
import unreal


SOURCE_ASSET_PATH = "/Game/Characters/Robot/Materials/T_Robot"
PROJECT_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
OUTPUT_DIR = os.path.join(PROJECT_DIR, "Content", "SourceArt", "Robot")
OUTPUT_PATH = os.path.join(OUTPUT_DIR, "T_Robot_Source.png")


os.makedirs(OUTPUT_DIR, exist_ok=True)
texture = unreal.load_asset(SOURCE_ASSET_PATH)
if texture is None:
    raise RuntimeError(f"Could not load texture: {SOURCE_ASSET_PATH}")

task = unreal.AssetExportTask()
task.object = texture
task.filename = OUTPUT_PATH
task.automated = True
task.prompt = False
task.replace_identical = True
task.write_empty_files = False
task.exporter = unreal.TextureExporterPNG()

if not unreal.Exporter.run_asset_export_task(task):
    raise RuntimeError(f"Could not export texture to: {OUTPUT_PATH}")

unreal.log(f"ROBOT_TEXTURE_EXPORT={OUTPUT_PATH}")
