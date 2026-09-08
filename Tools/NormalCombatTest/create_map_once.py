"""One-off UE 5.7 authoring of the saved normal combat laboratory.

Commit this generator with its assets, then remove it in the next commit.
This script is never used by gameplay or editor startup.
"""
from pathlib import Path
import json
import unreal

ROOT = Path(__file__).resolve().parents[2]
MAP = "/Game/Maps/NormalCombatTestMap"
DEST = "/Game/Environment/NormalCombatTest/Materials"
REPORT = ROOT / "TunaSweeper/Saved/Automation/NormalCombatTest/map_generation.json"
V = unreal.Vector
MATERIALS = {}
MARKERS = [(1080, -850, 100), (1170, 0, 100), (900, 850, 100)]
# Explicitly authored solids; broad center and perimeter routes stay connected.
COVERS = [
    ("LowWest", (-450, -660, 55), (150, 460, 110), "Cover"),
    ("SolidEast", (250, 650, 95), (190, 400, 190), "Metal"),
    ("SolidWest", (420, -380, 85), (230, 270, 170), "Metal"),
    ("LowRear", (-850, 690, 50), (250, 260, 100), "Cover"),
]


def spawn(cls, label, pos, *, pitch=0, yaw=0, roll=0):
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(cls, V(*pos), unreal.Rotator(pitch=pitch, yaw=yaw, roll=roll))
    assert actor, label
    actor.set_actor_label(label)
    actor.set_folder_path("CombatLab/" + label.split("_")[0])
    return actor


def material(name, color, emission=0.0):
    path = f"{DEST}/M_NCT_{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        mat = unreal.load_asset(path)
    else:
        mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(f"M_NCT_{name}", DEST, unreal.Material, unreal.MaterialFactoryNew())
    lib = unreal.MaterialEditingLibrary
    lib.delete_all_material_expressions(mat)
    rgb = lib.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -400, -100)
    rgb.set_editor_property("constant", unreal.LinearColor(*color, 1.0))
    lib.connect_material_property(rgb, "", unreal.MaterialProperty.MP_BASE_COLOR)
    rough = lib.create_material_expression(mat, unreal.MaterialExpressionConstant, -200, 120)
    rough.set_editor_property("r", 0.72)
    lib.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    if emission:
        glow = lib.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -400, 260)
        glow.set_editor_property("constant", unreal.LinearColor(*(c * emission for c in color), 1.0))
        lib.connect_material_property(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    lib.recompile_material(mat)
    assert unreal.EditorAssetLibrary.save_loaded_asset(mat, False)
    MATERIALS[name] = mat


def box(label, pos, size, mat="Floor", collision=True, yaw=0):
    actor = spawn(unreal.StaticMeshActor, label, pos, yaw=yaw)
    comp = actor.static_mesh_component
    comp.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cube"))
    comp.set_material(0, MATERIALS[mat])
    comp.set_mobility(unreal.ComponentMobility.STATIC)
    comp.set_collision_profile_name("BlockAll" if collision else "NoCollision")
    comp.set_collision_enabled(unreal.CollisionEnabled.QUERY_AND_PHYSICS if collision else unreal.CollisionEnabled.NO_COLLISION)
    comp.set_editor_property("can_ever_affect_navigation", collision)
    comp.set_editor_property("generate_overlap_events", False)
    comp.set_editor_property("cast_shadow", collision)
    actor.set_actor_scale3d(V(*(d / 100 for d in size)))
    actor.set_editor_property("tags", ["CombatLab_Blocker" if collision else "CombatLab_Decoration"])
    return actor


def text(label, message, pos, size=42, color=(225, 235, 240)):
    actor = spawn(unreal.TextRenderActor, label, pos, pitch=90, yaw=180)
    comp = actor.get_component_by_class(unreal.TextRenderComponent)
    comp.set_text(message)
    comp.set_world_size(size)
    comp.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
    comp.set_vertical_alignment(unreal.VerticalTextAligment.EVRTA_TEXT_CENTER)
    comp.set_text_render_color(unreal.Color(*color, 255))
    comp.set_editor_property("cast_shadow", False)


def build():
    assert not unreal.EditorAssetLibrary.does_asset_exist(MAP), "Refuse to overwrite an existing map"
    game_mode = unreal.load_class(None, "/Script/TunaSweeper.TunaSweeperCombatLabGameMode")
    assert game_mode, "Compile CombatLabGameMode before authoring map"
    world = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    world.get_world_settings().set_editor_property("default_game_mode", game_mode)
    world.get_world_settings().set_editor_property("kill_z", -1200.0)
    for spec in [
        ("Floor", (0.23, 0.28, 0.31)), ("Grid", (0.32, 0.37, 0.39)),
        ("Wall", (0.075, 0.105, 0.135)), ("Metal", (0.13, 0.18, 0.22)),
        ("Cover", (0.29, 0.34, 0.31)), ("Trim", (0.48, 0.53, 0.55)),
        ("Cyan", (0.025, 0.56, 0.70), 0.45), ("Amber", (0.93, 0.43, 0.08), 0.40),
    ]:
        material(*spec)
    box("Arena_Floor", (0, 0, -30), (3200, 2600, 60))
    for name, pos, size in [
        ("North", (1625, 0, 100), (50, 2700, 200)), ("South", (-1625, 0, 100), (50, 2700, 200)),
        ("West", (0, -1325, 100), (3200, 50, 200)), ("East", (0, 1325, 100), (3200, 50, 200)),
    ]:
        box("Perimeter_" + name, pos, size, "Wall")
    for sign in (-1, 1):
        box("Edge_X_" + str(sign), (sign * 1550, 0, 2), (8, 2480, 3), "Trim", False)
        box("Edge_Y_" + str(sign), (0, sign * 1250, 2), (3100, 8, 3), "Trim", False)
        box("Flank_Guide_" + str(sign), (0, sign * 1090, 2), (2360, 5, 2), "Cyan", False)
        text("Label_Flank_" + str(sign), "FLANK", (80, sign * 1140, 4), 34, (105, 203, 218))
    for index in range(-3, 4):
        box("Grid_X_" + str(index), (index * 400, 0, 1), (2, 2440, 1), "Grid", False)
    for index in range(-2, 3):
        box("Grid_Y_" + str(index), (0, index * 400, 1), (3040, 2, 1), "Grid", False)
    for name, pos, size, mat in COVERS:
        actor = box("Cover_" + name, pos, size, mat)
        actor.set_editor_property("tags", ["CombatLab_Blocker", "CombatLab_Cover"])
        x, y, z = pos
        sx, sy, sz = size
        box("CoverTrim_" + name, (x, y, z + sz / 2 + 2), (sx - 8, sy - 8, 4), "Trim", False)
        # Safety strip is paint, never a second collision hull or nav obstacle.
        for strip in range(3):
            box(f"CoverStripe_{name}_{strip}", (x, y - sy * 0.3 + strip * sy * 0.3, z + sz / 2 + 5),
                (sx - 16, 22, 2), "Amber", False)
    for index, pos in enumerate(MARKERS, 1):
        marker = spawn(unreal.TargetPoint, "CombatLabEnemy_" + str(index), pos, yaw=180)
        marker.set_editor_property("tags", ["CombatLabEnemy", "CombatLabEnemy_" + str(index)])
        x, y, _ = pos
        for sign in (-1, 1):
            box(f"SpawnPad_{index}_{sign}", (x + sign * 92, y, 2), (8, 170, 3), "Amber", False)
        text("Label_Enemy_" + str(index), f"0{index}", (x - 175, y, 4), 45, (235, 179, 103))
    spawn(unreal.PlayerStart, "CombatLabPlayerStart", (-1120, 0, 100))
    box("Player_ReadyLine", (-1260, 0, 2), (10, 750, 3), "Cyan", False)
    text("Label_Title", "COMBAT LAB  /  3 HOSTILES", (1440, 0, 4), 50)
    text("Label_Player", "PLAYER", (-1430, 0, 4), 48, (105, 203, 218))
    nav = spawn(unreal.NavMeshBoundsVolume, "Navigation_Bounds", (0, 0, 150))
    _, extent = nav.get_actor_bounds(False)
    assert min(extent.x, extent.y, extent.z) > 0
    nav.set_actor_scale3d(V(1700 / extent.x, 1400 / extent.y, 400 / extent.z))
    navs = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.RecastNavMesh)
    if not navs:
        navs = [spawn(unreal.RecastNavMesh, "Navigation_Recast", (0, 0, 0))]
    for recast in navs:
        recast.set_editor_property("runtime_generation", unreal.RuntimeGenerationType.DYNAMIC)
        recast.set_editor_property("force_rebuild_on_load", True)
    for name, pitch, yaw, intensity, priority in [
        ("Lighting_Key", -62, -28, 4.0, 1), ("Lighting_Fill", -38, 140, 1.6, 0),
    ]:
        light = spawn(unreal.DirectionalLight, name, (0, 0, 3000), pitch=pitch, yaw=yaw)
        comp = light.get_component_by_class(unreal.DirectionalLightComponent)
        comp.set_mobility(unreal.ComponentMobility.MOVABLE)
        comp.set_intensity(intensity)
        comp.set_forward_shading_priority(priority)
        comp.set_editor_property("light_source_angle", 8.0)
        if priority == 0:
            comp.set_editor_property("cast_shadows", False)
    post = spawn(unreal.PostProcessVolume, "Lighting_Exposure", (0, 0, 0))
    post.set_editor_property("unbound", True)
    settings = post.get_editor_property("settings")
    for name, value in [
        ("override_auto_exposure_method", True), ("auto_exposure_method", unreal.AutoExposureMethod.AEM_MANUAL),
        ("override_auto_exposure_bias", True), ("auto_exposure_bias", 1.0),
        ("override_auto_exposure_apply_physical_camera_exposure", True), ("auto_exposure_apply_physical_camera_exposure", False),
    ]:
        settings.set_editor_property(name, value)
    post.set_editor_property("settings", settings)
    assert unreal.EditorLoadingAndSavingUtils.save_map(world, MAP)
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps({"passed": True, "map": MAP, "markers": MARKERS, "covers": COVERS}, indent=2), encoding="utf-8")
    unreal.log("NORMAL_COMBAT_TEST_MAP_GENERATED")


if __name__ == "__main__":
    try:
        build()
    finally:
        unreal.SystemLibrary.quit_editor()
