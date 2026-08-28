from pathlib import Path

import unreal


REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
FBX_PATH = REPOSITORY_ROOT / "Blender" / "SKM_GazeTestRobot.fbx"
DESTINATION_PATH = "/Game/Characters/Test/GazeRobot"
MESH_PATH = f"{DESTINATION_PATH}/SKM_GazeTestRobot"
BLUEPRINT_PATH = f"{DESTINATION_PATH}/BP_GazeTestRobot"
INTRO_MAP_PATH = "/Game/Maps/IntroMap"
ACTOR_LABEL = "BP_GazeTestRobot_GazeComponentTest"

MATERIAL_SETTINGS = {
    "M_GazeRobot_Body": (unreal.LinearColor(0.12, 0.30, 0.38, 1.0), 0.65, 0.32),
    "M_GazeRobot_Head": (unreal.LinearColor(0.24, 0.52, 0.58, 1.0), 0.50, 0.28),
    "M_GazeRobot_Neck": (unreal.LinearColor(0.08, 0.12, 0.15, 1.0), 0.75, 0.40),
    "M_GazeRobot_Eye": (unreal.LinearColor(0.72, 0.94, 0.92, 1.0), 0.10, 0.18),
    "M_GazeRobot_Pupil": (unreal.LinearColor(0.01, 0.02, 0.025, 1.0), 0.05, 0.22),
}


def ensure_material(name, color, metallic, roughness):
    asset_path = f"{DESTINATION_PATH}/{name}"
    material = unreal.load_asset(asset_path)
    if material:
        return material

    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name,
        DESTINATION_PATH,
        unreal.Material,
        unreal.MaterialFactoryNew(),
    )
    if not material:
        raise RuntimeError(f"Failed to create material: {asset_path}")

    base_color = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionConstant3Vector, -420, -100
    )
    base_color.set_editor_property("constant", color)
    unreal.MaterialEditingLibrary.connect_material_property(
        base_color, "", unreal.MaterialProperty.MP_BASE_COLOR
    )

    metallic_expression = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionConstant, -420, 20
    )
    metallic_expression.set_editor_property("r", metallic)
    unreal.MaterialEditingLibrary.connect_material_property(
        metallic_expression, "", unreal.MaterialProperty.MP_METALLIC
    )

    roughness_expression = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionConstant, -420, 120
    )
    roughness_expression.set_editor_property("r", roughness)
    unreal.MaterialEditingLibrary.connect_material_property(
        roughness_expression, "", unreal.MaterialProperty.MP_ROUGHNESS
    )
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_asset(asset_path, only_if_is_dirty=False)
    return material


def assign_materials(mesh):
    generated_materials = {
        name: ensure_material(name, *settings)
        for name, settings in MATERIAL_SETTINGS.items()
    }
    skeletal_materials = list(mesh.get_editor_property("materials"))
    for skeletal_material in skeletal_materials:
        slot_name = str(skeletal_material.get_editor_property("material_slot_name"))
        material = generated_materials.get(slot_name)
        if material:
            skeletal_material.set_editor_property("material_interface", material)
        unreal.log(f"Gaze robot material slot: {slot_name} -> {material}")
    mesh.set_editor_property("materials", skeletal_materials)
    unreal.EditorAssetLibrary.save_asset(MESH_PATH, only_if_is_dirty=False)


def import_skeletal_mesh():
    if not FBX_PATH.is_file():
        raise RuntimeError(f"Missing gaze robot FBX: {FBX_PATH}")

    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_as_skeletal", True)
    options.set_editor_property("import_animations", False)
    options.set_editor_property("import_materials", True)
    options.set_editor_property("import_textures", False)
    options.set_editor_property("create_physics_asset", False)
    options.set_editor_property("automated_import_should_detect_type", False)
    options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(FBX_PATH))
    task.set_editor_property("destination_path", DESTINATION_PATH)
    task.set_editor_property("destination_name", "SKM_GazeTestRobot")
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", options)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    # The FBX task saves the primary mesh package, while the generated Skeleton
    # and material packages remain dirty. Persist them before another editor
    # process attempts to load the mesh.
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)

    mesh = unreal.load_asset(MESH_PATH)
    if not mesh:
        raise RuntimeError(f"Failed to import skeletal mesh: {MESH_PATH}")
    bounds = mesh.get_bounds()
    if not 0.8 <= bounds.box_extent.z <= 1.0:
        raise RuntimeError(
            f"Unexpected raw gaze robot half-height after import: {bounds.box_extent.z} cm"
        )
    assign_materials(mesh)
    unreal.EditorAssetLibrary.save_asset(MESH_PATH, only_if_is_dirty=False)
    return mesh


def ensure_blueprint():
    blueprint = unreal.load_asset(BLUEPRINT_PATH)
    if blueprint:
        return blueprint

    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.TunaSweeperGazeTestRobotCharacter)
    blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "BP_GazeTestRobot",
        DESTINATION_PATH,
        unreal.Blueprint,
        factory,
    )
    if not blueprint:
        raise RuntimeError(f"Failed to create Blueprint: {BLUEPRINT_PATH}")
    if hasattr(unreal, "BlueprintEditorLibrary"):
        unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    unreal.EditorAssetLibrary.save_asset(BLUEPRINT_PATH, only_if_is_dirty=False)
    return blueprint


def get_component(actor, component_name):
    for component in actor.get_components_by_class(unreal.ActorComponent):
        if component.get_name() == component_name:
            return component
    return None


def clear_existing_placement():
    world = unreal.EditorLoadingAndSavingUtils.load_map(INTRO_MAP_PATH)
    if not world:
        raise RuntimeError(f"Failed to load map for reset: {INTRO_MAP_PATH}")

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for actor in list(actor_subsystem.get_all_level_actors()):
        if actor.get_actor_label() == ACTOR_LABEL:
            actor_subsystem.destroy_actor(actor)
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)

    unreal.log("Cleared prior gaze robot map instance.")


def place_blueprint(blueprint, mesh):
    world = unreal.EditorLoadingAndSavingUtils.load_map(INTRO_MAP_PATH)
    if not world:
        raise RuntimeError(f"Failed to load map: {INTRO_MAP_PATH}")

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    all_actors = actor_subsystem.get_all_level_actors()
    title_actor = next(
        (actor for actor in all_actors if isinstance(actor, unreal.TunaSweeperTitlePresentationActor)),
        None,
    )
    if not title_actor:
        raise RuntimeError("IntroMap has no TunaSweeperTitlePresentationActor")

    for actor in list(all_actors):
        if actor.get_actor_label() == ACTOR_LABEL:
            actor_subsystem.destroy_actor(actor)

    title_origin = title_actor.get_actor_location()
    robot_location = (
        title_origin
        + title_actor.get_actor_forward_vector() * 120.0
        + title_actor.get_actor_right_vector() * 100.0
    )
    title_rotation = title_actor.get_actor_rotation()
    robot_rotation = unreal.Rotator()
    robot_rotation.pitch = 0.0
    robot_rotation.yaw = title_rotation.yaw + 105.0
    robot_rotation.roll = 0.0

    generated_class = blueprint.generated_class()
    unreal.log("Gaze robot placement: spawning Blueprint actor")
    robot = actor_subsystem.spawn_actor_from_class(generated_class, robot_location, robot_rotation)
    if not robot:
        raise RuntimeError("Failed to place BP_GazeTestRobot in IntroMap")
    unreal.log("Gaze robot placement: actor spawned")
    robot.set_actor_label(ACTOR_LABEL)
    unreal.log("Gaze robot placement: actor label assigned")
    robot.set_actor_scale3d(unreal.Vector(0.72, 0.72, 0.72))
    unreal.log("Gaze robot placement: actor scale assigned")

    mesh_component = get_component(robot, "CharacterMesh0")
    if not mesh_component:
        raise RuntimeError("Placed gaze robot has no CharacterMesh0 component")
    unreal.log("Gaze robot placement: assigning skeletal mesh")
    mesh_component.set_skeletal_mesh(mesh)
    unreal.log("Gaze robot placement: skeletal mesh assigned")

    unreal.EditorAssetLibrary.save_asset(BLUEPRINT_PATH, only_if_is_dirty=False)
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log(
        f"Placed {ACTOR_LABEL} at {robot_location}, rotation {robot_rotation}, scale 0.72"
    )


def main():
    clear_existing_placement()
    mesh = import_skeletal_mesh()
    blueprint = ensure_blueprint()
    place_blueprint(blueprint, mesh)
    unreal.log("Gaze test robot import, Blueprint creation, and IntroMap placement completed.")


if __name__ == "__main__":
    main()
