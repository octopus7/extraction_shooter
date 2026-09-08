"""One-off correction of the new test map's light priority and floor lettering.

Commit with the corrected map, then remove alongside create_map_once.py.
"""
import unreal

try:
    path = "/Game/Maps/BossCombatTestMap"
    world = unreal.EditorLoadingAndSavingUtils.load_map(path)
    lights = {actor.get_actor_label(): actor for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.DirectionalLight)}
    assert set(lights) == {"Lighting_Key", "Lighting_Fill"}
    for name, priority in (("Lighting_Key", 1), ("Lighting_Fill", 0)):
        component = lights[name].get_component_by_class(unreal.DirectionalLightComponent)
        component.set_forward_shading_priority(priority)
        assert component.get_editor_property("forward_shading_priority") == priority
    for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.TextRenderActor):
        actor.set_actor_rotation(unreal.Rotator(pitch=90, yaw=180, roll=0), False)
    assert unreal.EditorLoadingAndSavingUtils.save_map(world, path)
    unreal.log("BOSS_COMBAT_TEST_LIGHTING_AND_TEXT_REPAIRED")
finally:
    unreal.SystemLibrary.quit_editor()
