"""One-off authoring of the persistent boss combat test map (UE 5.7).

Commit the saved map/materials with this generator, then remove this file in
the immediately following commit. Gameplay never runs this generator.
"""
from pathlib import Path
import json
import unreal

ROOT = Path(__file__).resolve().parents[2]
MAP = "/Game/Maps/BossCombatTestMap"
DEST = "/Game/Environment/BossCombatTest/Materials"
REPORT = ROOT / "TunaSweeper/Saved/Automation/BossCombatTest/map_generation.json"
V = unreal.Vector
R = unreal.Rotator
ASSETS = unreal.AssetToolsHelpers.get_asset_tools()
LIB = unreal.MaterialEditingLibrary
WORLD = None
MATERIALS = {}

ARENAS = [
    {"id": "charge", "origin": (10000, -10000), "title": "01  CHARGE",
     "class": "TunaSweeperChargeTeachingMiniboss", "half": 1800,
     "lesson": "READ THE LANE  /  DODGE TO THE SIDE", "accent": "Amber"},
    {"id": "robots", "origin": (10000, 0), "title": "02  ROLLING ROBOTS",
     "class": "TunaSweeperRobotTeachingMiniboss", "half": 1800,
     "lesson": "SHOOT ROBOTS WHILE THEY ROLL", "accent": "Cyan"},
    {"id": "main", "origin": (10000, 10000), "title": "03  MAIN BOSS",
     "class": "TunaSweeperPatternEnemyCharacter", "half": 2200,
     "lesson": "MISSILES  /  CHARGE  /  ROLLING ROBOTS", "accent": "Red"},
]


def spawn(cls, label, pos, rotation=None):
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(cls, V(*pos), rotation or R())
    assert actor, ("Actor creation failed", label)
    actor.set_actor_label(label)
    actor.set_folder_path("BossCombatTest/" + label.split("_")[0])
    return actor


def material(name, color, emission=0.0, roughness=0.72, metallic=0.0):
    path = f"{DEST}/M_BCT_{name}"
    mat = unreal.load_asset(path)
    if mat is None:
        mat = ASSETS.create_asset(f"M_BCT_{name}", DEST, unreal.Material, unreal.MaterialFactoryNew())
    assert isinstance(mat, unreal.Material), path
    LIB.delete_all_material_expressions(mat)
    rgb = LIB.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -420, -120)
    rgb.set_editor_property("constant", unreal.LinearColor(*color, 1.0))
    LIB.connect_material_property(rgb, "", unreal.MaterialProperty.MP_BASE_COLOR)
    for value, prop, y in ((roughness, unreal.MaterialProperty.MP_ROUGHNESS, 40),
                           (metallic, unreal.MaterialProperty.MP_METALLIC, 180)):
        node = LIB.create_material_expression(mat, unreal.MaterialExpressionConstant, -220, y)
        node.set_editor_property("r", value)
        LIB.connect_material_property(node, "", prop)
    if emission:
        glow = LIB.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -420, 320)
        glow.set_editor_property("constant", unreal.LinearColor(*(v * emission for v in color), 1.0))
        LIB.connect_material_property(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    LIB.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat, False)
    MATERIALS[name] = mat


def mesh(label, pos, dimensions, mat="Floor", collision=True, shape="Cube", yaw=0):
    actor = spawn(unreal.StaticMeshActor, label, pos, R(0, yaw, 0))
    comp = actor.static_mesh_component
    comp.set_static_mesh(unreal.load_asset(f"/Engine/BasicShapes/{shape}"))
    comp.set_material(0, MATERIALS[mat])
    comp.set_mobility(unreal.ComponentMobility.STATIC)
    actor.set_actor_scale3d(V(*(d / 100.0 for d in dimensions)))
    comp.set_collision_profile_name("BlockAll" if collision else "NoCollision")
    comp.set_collision_enabled(unreal.CollisionEnabled.QUERY_AND_PHYSICS if collision else unreal.CollisionEnabled.NO_COLLISION)
    comp.set_editor_property("generate_overlap_events", False)
    comp.set_editor_property("can_ever_affect_navigation", collision)
    comp.set_editor_property("cast_shadow", collision)
    actor.set_editor_property("tags", ["BCT_Blocker" if collision else "BCT_Decoration"])
    return actor


def text(label, message, pos, size=55, color=(200, 222, 230)):
    # Text faces +Z; its top points +X (north), matching the player camera.
    actor = spawn(unreal.TextRenderActor, label, pos, R(pitch=90, yaw=180, roll=0))
    comp = actor.get_component_by_class(unreal.TextRenderComponent)
    comp.set_text(message)
    comp.set_world_size(size)
    comp.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
    comp.set_vertical_alignment(unreal.VerticalTextAligment.EVRTA_TEXT_CENTER)
    comp.set_text_render_color(unreal.Color(*color, 255))
    comp.set_editor_property("cast_shadow", False)
    return actor


def stripes(prefix, x, y, width, material_name="Amber"):
    for i in range(int(width / 110)):
        mesh(f"{prefix}_Stripe_{i:02}", (x, y - width / 2 + 55 + i * 110, 2),
             (42, 70, 3), material_name, False, yaw=25)


def outline(prefix, center, size, mat="Cyan", width=10):
    x, y = center
    sx, sy = size
    mesh(prefix + "_North", (x + sx / 2, y, 2), (width, sy, 3), mat, False)
    mesh(prefix + "_South", (x - sx / 2, y, 2), (width, sy, 3), mat, False)
    mesh(prefix + "_East", (x, y + sy / 2, 2), (sx, width, 3), mat, False)
    mesh(prefix + "_West", (x, y - sy / 2, 2), (sx, width, 3), mat, False)


def perimeter(prefix, center, size, entry_width=0):
    x, y = center
    sx, sy = size
    height = 180
    mesh(prefix + "_WallNorth", (x + sx / 2 + 25, y, height / 2), (50, sy + 100, height), "Wall")
    mesh(prefix + "_WallWest", (x, y - sy / 2 - 25, height / 2), (sx, 50, height), "Wall")
    mesh(prefix + "_WallEast", (x, y + sy / 2 + 25, height / 2), (sx, 50, height), "Wall")
    if entry_width:
        part = (sy - entry_width) / 2
        for side in (-1, 1):
            mesh(f"{prefix}_WallSouth_{side}", (x - sx / 2 - 25, y + side * (entry_width / 2 + part / 2), height / 2),
                 (50, part, height), "Wall")
    else:
        mesh(prefix + "_WallSouth", (x - sx / 2 - 25, y, height / 2), (50, sy + 100, height), "Wall")


def portal(label, point_id, target_id, pos, display, yaw=0, accent="Cyan"):
    cls = unreal.load_class(None, "/Script/TunaSweeper.TunaSweeperWarpPointActor")
    actor = spawn(cls, label, pos, R(0, yaw, 0))
    # Use the project's existing interaction and fade; authored landing pads
    # provide clearance because the shared warp does not sweep for obstacles.
    actor.configure_warp_point_defaults(
        point_id, target_id, display, None,
        unreal.load_asset("/Game/Interaction/M_WarpPointEnergy"),
        unreal.load_asset("/Engine/BasicShapes/Sphere"),
        V(0.7, 0.7, 1.4), V(0, 0, 45), V(250, 0, 0), True)
    actor.get_interactable_component().set_editor_property("marker_visible_distance", 600.0)
    x, y, _ = pos
    mesh(label + "_Plinth", (x, y, 8), (235, 235, 16), "Metal", False, "Cylinder")
    mesh(label + "_Pad", (x, y, 17), (205, 205, 3), accent, False, "Cylinder")
    for sign in (-1, 1):
        mesh(f"{label}_Pylon_{sign}", (x, y + sign * 145, 75), (45, 45, 150), "Metal")
        mesh(f"{label}_Lamp_{sign}", (x, y + sign * 145, 153), (48, 48, 7), accent, False)
    return actor


def navigation(label, center, extent):
    nav = spawn(unreal.NavMeshBoundsVolume, label, center)
    _, default_extent = nav.get_actor_bounds(False)
    assert min(default_extent.x, default_extent.y, default_extent.z) > 0, "Nav volume factory did not build a brush"
    nav.set_actor_scale3d(V(extent[0] / default_extent.x, extent[1] / default_extent.y, extent[2] / default_extent.z))
    return nav


def build_hub():
    mesh("Hub_Floor", (0, 0, -30), (2200, 2800, 60))
    perimeter("Hub", (0, 0), (2200, 2800))
    outline("Hub_Edge", (0, 0), (2100, 2700), "Cyan")
    text("Hub_Title", "BOSS COMBAT LAB", (820, 0, 4), 85)
    text("Hub_Instructions", "WASD MOVE   /   F PORTAL   /   SPACE DODGE", (-560, 0, 4), 48)
    text("Hub_SafeNotice", "PORTALS LEAD TO SAFE STAGING AREAS", (-740, 0, 4), 44, (90, 210, 222))
    text("Hub_EntryNotice", "WALK ACROSS THE ORANGE LINE TO START", (-890, 0, 4), 42, (244, 170, 78))
    for index, arena in enumerate(ARENAS):
        y = (index - 1) * 720
        portal("Hub_Portal_" + arena["id"], "bct_hub_" + arena["id"], "bct_stage_" + arena["id"],
               (400, y, 96), arena["title"], 180, arena["accent"])
        text("Hub_Label_" + arena["id"], arena["title"], (170, y, 4), 42)
        text("Hub_Use_" + arena["id"], "F  /  WARP", (-10, y, 4), 37, (90, 210, 222))
    spawn(unreal.PlayerStart, "Hub_PlayerStart", (-230, 0, 100))
    navigation("Hub_Navigation", (0, 0, 150), (1200, 1500, 400))


def build_arena(spec):
    ident, (ox, oy), half = spec["id"], spec["origin"], spec["half"]
    prefix = ident.capitalize()
    center_x = ox + 2200 + half
    mesh(prefix + "_StageFloor", (ox, oy, -30), (1800, 2200, 60), "SafeFloor")
    # Stage north wall has a 700 cm opening; rotate the perimeter to keep its opening north.
    mesh(prefix + "_StageSouthWall", (ox - 925, oy, 90), (50, 2300, 180), "Wall")
    for side in (-1, 1):
        mesh(f"{prefix}_StageSideWall_{side}", (ox, oy + side * 1125, 90), (1800, 50, 180), "Wall")
        mesh(f"{prefix}_StageNorthWall_{side}", (ox + 925, oy + side * 725, 90), (50, 750, 180), "Wall")
    outline(prefix + "_SafeBorder", (ox, oy), (1720, 2120))
    portal(prefix + "_ReturnPortal", "bct_stage_" + ident, "bct_hub_" + ident,
           (ox, oy, 96), "RETURN TO HUB", accent="Cyan")
    text(prefix + "_ReturnLabel", "F  /  RETURN TO HUB", (ox - 290, oy, 4), 48, (90, 210, 222))
    text(prefix + "_StageTitle", spec["title"], (ox + 580, oy, 4), 65)
    text(prefix + "_StageSafety", "SAFE STAGING", (ox - 570, oy, 4), 65, (90, 210, 222))
    text(prefix + "_StageHint", "TAKE YOUR TIME. PREPARE HERE.", (ox - 760, oy, 4), 38)
    # A 13 m runway keeps the destination well outside the combat trigger.
    mesh(prefix + "_ApproachFloor", (ox + 1550, oy, -30), (1300, 700, 60), "SafeFloor")
    for side in (-1, 1):
        mesh(f"{prefix}_ApproachWall_{side}", (ox + 1550, oy + side * 375, 90), (1300, 50, 180), "Wall")
        mesh(f"{prefix}_ApproachGuide_{side}", (ox + 1490, oy + side * 320, 2), (1100, 8, 3), "Cyan", False)
    for i, x in enumerate((1100, 1450, 1800)):
        # Chevron arrows physically point +X.
        for side in (-1, 1):
            mesh(f"{prefix}_Arrow_{i}_{side}", (ox + x, oy + side * 45, 3), (14, 120, 3), "Cyan", False, yaw=side * 40)
    text(prefix + "_EntrySign", "CROSS TO START", (ox + 2040, oy, 5), 40, (244, 170, 78))
    mesh(prefix + "_ArenaFloor", (center_x, oy, -30), (half * 2, half * 2, 60))
    perimeter(prefix + "_Arena", (center_x, oy), (half * 2, half * 2), 700)
    outline(prefix + "_ArenaEdge", (center_x, oy), (half * 2 - 80, half * 2 - 80), spec["accent"], 7)
    # The boundary is exactly where the deferred encounter becomes eligible.
    mesh(prefix + "_CombatThreshold", (ox + 2200, oy, 3), (24, 700, 4), "Amber", False)
    stripes(prefix + "_Warning", ox + 2245, oy, 660)
    text(prefix + "_Lesson", spec["lesson"], (ox + 2600, oy, 4), 43)
    text(prefix + "_ArenaTitle", spec["title"], (center_x + half - 420, oy, 4), 105)
    # Subtle 4 m calibration marks leave attack telegraphs readable.
    for axis in range(-int(half / 400) + 1, int(half / 400)):
        mesh(f"{prefix}_GridX_{axis}", (center_x + axis * 400, oy, 1), (2, half * 2 - 180, 1), "Grid", False)
        mesh(f"{prefix}_GridY_{axis}", (center_x, oy + axis * 400, 1), (half * 2 - 180, 2, 1), "Grid", False)
    # Edge cover preserves a wide central dodging lane and minion routes.
    for side in (-1, 1):
        for row in (-1, 1):
            x, y = center_x + row * half * 0.48, oy + side * (half - 360)
            mesh(f"{prefix}_Cover_{side}_{row}", (x, y, 60), (300, 150, 120), "Metal")
            mesh(f"{prefix}_CoverCap_{side}_{row}", (x, y, 122), (290, 140, 4), "Trim", False)
    encounter = spawn(unreal.load_class(None, "/Script/TunaSweeper.TunaSweeperBossEncounter"),
                      prefix + "_Encounter", (center_x, oy, 200))
    encounter.get_editor_property("combat_bounds").set_box_extent(V(half, half, 400), True)
    encounter.set_editor_property("encounter_id", ident)
    encounter.set_editor_property("display_name", spec["title"])
    encounter.set_editor_property("boss_class", unreal.load_class(None, "/Script/TunaSweeper." + spec["class"]))
    encounter.set_editor_property("boss_spawn_offset", V(-1000, 0, -90))
    encounter.set_editor_property("boss_spawn_rotation", R(0, 180, 0))
    navigation(prefix + "_Navigation", (ox + (2200 + half * 2 - 900) / 2, oy, 150),
               ((2200 + half * 2 + 900) / 2 + 100, half + 100, 400))


def build():
    global WORLD
    assert not unreal.EditorAssetLibrary.does_asset_exist(MAP), "Do not overwrite an existing test map; delete only this newly generated map explicitly before rerunning"
    WORLD = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    assert WORLD
    for spec in [
        ("Floor", (0.20, 0.245, 0.275)), ("SafeFloor", (0.12, 0.21, 0.235)),
        ("Wall", (0.08, 0.11, 0.14)), ("Metal", (0.11, 0.155, 0.19)),
        ("Trim", (0.42, 0.48, 0.50)), ("Grid", (0.28, 0.33, 0.355)),
        ("Cyan", (0.015, 0.62, 0.75), 0.6),
        ("Amber", (0.95, 0.39, 0.055), 0.5),
        ("Red", (0.80, 0.11, 0.075), 0.4),
    ]:
        material(*spec)
    WORLD.get_world_settings().set_editor_property("default_game_mode", unreal.load_class(None, "/Script/TunaSweeper.TunaSweeperBossTestGameMode"))
    WORLD.get_world_settings().set_editor_property("kill_z", -1200.0)
    build_hub()
    for arena in ARENAS:
        build_arena(arena)
    for name, rotation, intensity, color in (
        ("Lighting_Key", R(-58, -28, 0), 4.0, (235, 244, 255)),
        ("Lighting_Fill", R(-42, 145, 0), 1.8, (170, 204, 235)),
    ):
        light = spawn(unreal.DirectionalLight, name, (0, 0, 3000), rotation)
        comp = light.get_component_by_class(unreal.DirectionalLightComponent)
        comp.set_mobility(unreal.ComponentMobility.MOVABLE)
        comp.set_intensity(intensity)
        comp.set_light_color(unreal.LinearColor(color[0] / 255, color[1] / 255, color[2] / 255, 1.0))
        comp.set_editor_property("light_source_angle", 8.0)
        comp.set_forward_shading_priority(1 if name.endswith("Key") else 0)
        if name.endswith("Fill"):
            comp.set_editor_property("cast_shadows", False)
    post = spawn(unreal.PostProcessVolume, "Lighting_Exposure", (0, 0, 0))
    post.set_editor_property("unbound", True)
    settings = post.get_editor_property("settings")
    settings.set_editor_property("override_auto_exposure_method", True)
    settings.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_MANUAL)
    settings.set_editor_property("override_auto_exposure_bias", True)
    settings.set_editor_property("auto_exposure_bias", 1.0)
    settings.set_editor_property("override_auto_exposure_apply_physical_camera_exposure", True)
    settings.set_editor_property("auto_exposure_apply_physical_camera_exposure", False)
    post.set_editor_property("settings", settings)
    navs = unreal.GameplayStatics.get_all_actors_of_class(WORLD, unreal.RecastNavMesh)
    if not navs:
        navs = [spawn(unreal.RecastNavMesh, "Navigation_Recast", (0, 0, 0))]
    for nav in navs:
        nav.set_editor_property("runtime_generation", unreal.RuntimeGenerationType.DYNAMIC)
        nav.set_editor_property("force_rebuild_on_load", True)
    assert unreal.EditorLoadingAndSavingUtils.save_map(WORLD, MAP)
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps({"map": MAP, "passed": True, "arenas": ARENAS,
                                  "navigation": "Saved bounds + dynamic runtime Recast", "generator": "one-off"}, indent=2), encoding="utf-8")
    unreal.log("BOSS_COMBAT_TEST_MAP_GENERATED")


if __name__ == "__main__":
    try:
        build()
    finally:
        unreal.SystemLibrary.quit_editor()
