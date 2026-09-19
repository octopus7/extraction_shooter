"""One-off audio import and binding. Remove immediately after the asset commit."""
from pathlib import Path
import unreal

ROOT = Path(__file__).resolve().parents[2]
DEST = "/Game/Environment/ShallowPuddle"
source = ROOT / "TunaSweeper/SourceArt/Audio/ShallowPuddle/SW_ShallowPuddle_Footstep.wav"
assert source.is_file()
task = unreal.AssetImportTask()
task.set_editor_property("filename", str(source))
task.set_editor_property("destination_path", DEST + "/Audio")
task.set_editor_property("destination_name", "SW_ShallowPuddle_Footstep")
task.set_editor_property("automated", True)
task.set_editor_property("replace_existing", True)
task.set_editor_property("save", True)
task.set_editor_property("factory", unreal.SoundFactory())
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
sound = unreal.load_asset(DEST + "/Audio/SW_ShallowPuddle_Footstep")
assert isinstance(sound, unreal.SoundWave)
sound.set_editor_property("looping", False)
sound.set_editor_property("compression_quality", 80)
assert unreal.EditorAssetLibrary.save_loaded_asset(sound, False)
blueprint = unreal.load_asset(DEST + "/BP_ShallowPuddle")
defaults = unreal.get_default_object(blueprint.generated_class())
defaults.set_editor_property("water_footstep_sound", sound)
unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
assert unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False)
map_path = DEST + "/Maps/L_ShallowPuddle_Review"
world = unreal.EditorLoadingAndSavingUtils.load_map(map_path)
puddles = unreal.GameplayStatics.get_all_actors_of_class(world, blueprint.generated_class())
assert len(puddles) == 3
for puddle in puddles:
    puddle.set_editor_property("water_footstep_sound", sound)
assert unreal.EditorLoadingAndSavingUtils.save_map(world, map_path)
unreal.log("PUDDLE_AUDIO_IMPORTED_AND_BOUND " + sound.get_path_name())
