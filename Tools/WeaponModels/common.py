"""Shared Blender authoring helpers for the additive TunaSweeper weapon pack.

Builders use Unreal coordinates in centimetres: +X muzzle/forward, +Y right,
+Z up.  Helpers convert to Blender's -Y forward, -X export-right, +Z up so
the legacy FBX importer's handedness conversion restores UE +Y right.  The world
origin remains the receiver/attachment pivot and FBX import is expected at
identity transform in UE 5.7.
"""
from __future__ import annotations

import bpy
import bmesh
import json
import math
from pathlib import Path
from mathutils import Vector


ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "TunaSweeper/SourceArt/Weapons/TunaWeaponCollection"
ATLAS = OUT / "Textures/T_TunaWeaponCollection_Atlas.png"

# ImageGen atlas cells are listed from the top row.  All materials sample the
# same source image through per-part UVs; constants carry the PBR distinction.
TILES = {
    "Charcoal": (0, 0, 0.72, 0.38, 0.0),
    "OlivePolymer": (1, 0, 0.04, 0.58, 0.0),
    "WarmGrayPolymer": (2, 0, 0.02, 0.55, 0.0),
    "Rubber": (3, 0, 0.0, 0.86, 0.0),
    "Gunmetal": (0, 1, 0.82, 0.32, 0.0),
    "Steel": (1, 1, 0.92, 0.24, 0.0),
    "Titanium": (2, 1, 0.78, 0.28, 0.0),
    "SmokedPolymer": (3, 1, 0.05, 0.28, 0.0),
    "IvoryCeramic": (0, 2, 0.03, 0.34, 0.0),
    "GraphiteCeramic": (1, 2, 0.08, 0.30, 0.0),
    "CyanAccent": (2, 2, 0.12, 0.22, 2.5),
    "VioletLens": (3, 2, 0.18, 0.18, 1.4),
    "TanGrip": (0, 3, 0.0, 0.78, 0.0),
    "BlackGrip": (1, 3, 0.0, 0.86, 0.0),
    "Magazine": (2, 3, 0.42, 0.48, 0.0),
    "EdgeWear": (3, 3, 0.74, 0.36, 0.0),
}

materials = {}
parts = []
collisions = []
entries = []
objects = []
current = {}
family = ""


def ue(value):
    """UE cm tuple -> Blender metre vector."""
    x, y, z = value
    return Vector((-y / 100.0, -x / 100.0, z / 100.0))


def init(label):
    global materials, parts, collisions, entries, objects, current, family
    family = label
    materials, parts, collisions, entries, objects, current = {}, [], [], [], [], {}
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.scale_length = 1.0
    for path in (OUT / "Models", OUT / "Previews", OUT / "Manifests"):
        path.mkdir(parents=True, exist_ok=True)
    image = bpy.data.images.load(str(ATLAS))
    image.pack()
    image.filepath = "//Textures/T_TunaWeaponCollection_Atlas.png"
    for key, (_, _, metallic, roughness, emission) in TILES.items():
        mat = bpy.data.materials.new("M_TunaWeapon_" + key)
        mat.use_nodes = True
        nodes = mat.node_tree.nodes
        bsdf = nodes.get("Principled BSDF")
        bsdf.inputs["Metallic"].default_value = metallic
        bsdf.inputs["Roughness"].default_value = roughness
        tex = nodes.new("ShaderNodeTexImage")
        tex.image = image
        mat.node_tree.links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
        if emission:
            bsdf.inputs["Emission Color"].default_value = (0.0, 0.7, 1.0, 1.0) if key == "CyanAccent" else (0.35, 0.08, 0.8, 1.0)
            bsdf.inputs["Emission Strength"].default_value = emission
        materials[key] = mat


def begin(name, category, style, reference, design_notes):
    global parts, collisions, current
    parts, collisions = [], []
    current = {
        "name": name,
        "category": category,
        "style": style,
        "reference": reference,
        "design_notes": design_notes,
        "pivot": "receiver attachment reference at UE (0,0,0)",
        "axis": "+X muzzle/forward, +Y right, +Z up",
    }


def _uv_to_tile(obj, material_name):
    uv_layer = obj.data.uv_layers.new(name="UVMap") if not obj.data.uv_layers else obj.data.uv_layers.active
    coordinates = [vertex.co for vertex in obj.data.vertices]
    minima = [min(vertex[i] for vertex in coordinates) for i in range(3)]
    maxima = [max(vertex[i] for vertex in coordinates) for i in range(3)]
    column, row, _, _, _ = TILES[material_name]
    for polygon in obj.data.polygons:
        dominant = max(range(3), key=lambda axis: abs(polygon.normal[axis]))
        axes = [axis for axis in range(3) if axis != dominant]
        for loop_index in polygon.loop_indices:
            vertex = obj.data.vertices[obj.data.loops[loop_index].vertex_index].co
            local = [(vertex[a] - minima[a]) / max(maxima[a] - minima[a], 1e-8) for a in axes]
            # A generous inset keeps filtering/mips out of neighbouring cells.
            uv_layer.data[loop_index].uv = (
                (column + 0.06 + 0.88 * local[0]) / 4.0,
                (3 - row + 0.06 + 0.88 * local[1]) / 4.0,
            )


def finish(obj, part_name, material="Charcoal", bevel_cm=0.12, smooth=True):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    if obj.type != "MESH":
        bpy.ops.object.convert(target="MESH")
        obj = bpy.context.object
    obj.name = current["name"] + "_" + part_name
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    if bevel_cm > 0:
        modifier = obj.modifiers.new("Game-ready bevel", "BEVEL")
        modifier.width = bevel_cm / 100.0
        modifier.segments = 2
        bpy.ops.object.modifier_apply(modifier=modifier.name)
    mesh = bmesh.new()
    mesh.from_mesh(obj.data)
    bmesh.ops.remove_doubles(mesh, verts=list(mesh.verts), dist=1e-7)
    bmesh.ops.dissolve_degenerate(mesh, edges=list(mesh.edges), dist=1e-7)
    bmesh.ops.recalc_face_normals(mesh, faces=list(mesh.faces))
    mesh.to_mesh(obj.data)
    mesh.free()
    for polygon in obj.data.polygons:
        polygon.use_smooth = smooth
    if smooth:
        modifier = obj.modifiers.new("Weighted normals", "WEIGHTED_NORMAL")
        modifier.keep_sharp = True
        bpy.ops.object.modifier_apply(modifier=modifier.name)
    _uv_to_tile(obj, material)
    obj.data.materials.clear()
    obj.data.materials.append(materials[material])
    parts.append(obj)
    obj.select_set(False)
    return obj


def box(name, location_cm, size_cm, material="Charcoal", bevel_cm=0.12, rotation=(0, 0, 0)):
    bpy.ops.mesh.primitive_cube_add(size=1, location=ue(location_cm))
    obj = bpy.context.object
    sx, sy, sz = size_cm
    obj.dimensions = (sy / 100.0, sx / 100.0, sz / 100.0)
    # Input rotations follow UE roll/pitch/yaw degrees.  Builders generally use
    # yaw=0 so identity export stays unambiguous.
    roll, pitch, yaw = [math.radians(v) for v in rotation]
    obj.rotation_euler = (roll, -pitch, -yaw)
    return finish(obj, name, material, bevel_cm, True)


def cylinder(name, location_cm, radius_cm, depth_cm, material="Gunmetal", axis=(1, 0, 0), vertices=24, bevel_cm=0.08):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius_cm / 100.0, depth=depth_cm / 100.0, location=ue(location_cm))
    obj = bpy.context.object
    obj.rotation_euler = ue(axis).to_track_quat("Z", "Y").to_euler()
    return finish(obj, name, material, bevel_cm, True)


def wedge(name, location_cm, size_cm, taper_front=0.8, material="Charcoal", bevel_cm=0.1):
    """Chamfer-friendly tapered box. Front is +UE X."""
    sx, sy, sz = size_cm
    hx, hy, hz = sx / 2.0, sy / 2.0, sz / 2.0
    fy = hy * taper_front
    fz = hz * taper_front
    local_ue = [
        (-hx, -hy, -hz), (-hx, hy, -hz), (-hx, hy, hz), (-hx, -hy, hz),
        (hx, -fy, -fz), (hx, fy, -fz), (hx, fy, fz), (hx, -fy, fz),
    ]
    lx, ly, lz = location_cm
    verts = [ue((x + lx, y + ly, z + lz)) for x, y, z in local_ue]
    faces = [(0, 1, 2, 3), (4, 7, 6, 5), (0, 4, 5, 1), (1, 5, 6, 2), (2, 6, 7, 3), (3, 7, 4, 0)]
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    return finish(obj, name, material, bevel_cm, True)


def frame_bars(name, center_cm, outer_cm, bar_cm, material="Rubber", depth_cm=None):
    """Four rectangular bars forming a readable side-profile cutout."""
    sx, sy, sz = outer_cm
    thickness = bar_cm
    depth = depth_cm if depth_cm is not None else sy
    cx, cy, cz = center_cm
    box(name + "Top", (cx, cy, cz + sz / 2 - thickness / 2), (sx, depth, thickness), material, thickness * 0.18)
    box(name + "Bottom", (cx, cy, cz - sz / 2 + thickness / 2), (sx, depth, thickness), material, thickness * 0.18)
    box(name + "Rear", (cx - sx / 2 + thickness / 2, cy, cz), (thickness, depth, sz - 2 * thickness), material, thickness * 0.18)
    box(name + "Front", (cx + sx / 2 - thickness / 2, cy, cz), (thickness, depth, sz - 2 * thickness), material, thickness * 0.18)


def rail_teeth(prefix, start_x, end_x, y, z, count, material="Gunmetal", width_cm=0.55, height_cm=0.3):
    step = (end_x - start_x) / max(count - 1, 1)
    for index in range(count):
        box(f"{prefix}{index:02d}", (start_x + index * step, y, z), (width_cm, 1.5, height_cm), material, 0.03)


def collision_box(location_cm, size_cm):
    collisions.append((tuple(location_cm), tuple(size_cm)))


def socket(name, location_cm, rotation=(0.0, 0.0, 0.0)):
    assert name in {"MuzzleSocket", "LaserSightSocket", "ShellEjectionSocket"}
    current.setdefault("sockets", []).append({"name": name, "location_cm": list(location_cm), "rotation_deg": list(rotation), "scale": [1.0, 1.0, 1.0]})


def _bounds_ue_cm(obj):
    points = [obj.matrix_world @ Vector(corner) for corner in obj.bound_box]
    # Inverse of ue(): Blender -x -> UE y; Blender -y -> UE x.
    ue_points = [Vector((-point.y * 100.0, -point.x * 100.0, point.z * 100.0)) for point in points]
    return [min(point[axis] for point in ue_points) for axis in range(3)] + [max(point[axis] for point in ue_points) for axis in range(3)]


def end(expected_size_cm, size_tolerance_cm=1.0):
    assert parts and current
    assert {entry["name"] for entry in current.get("sockets", [])} == {"MuzzleSocket", "LaserSightSocket", "ShellEjectionSocket"}
    bpy.ops.object.select_all(action="DESELECT")
    for obj in parts:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    obj = bpy.context.object
    obj.name = current["name"]
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    bpy.context.scene.cursor.location = (0, 0, 0)
    bpy.ops.object.origin_set(type="ORIGIN_CURSOR")
    # Consolidate same material datablocks after joining parts.
    old = list(obj.data.materials)
    unique, remap = [], {}
    for index, material in enumerate(old):
        if material not in unique:
            unique.append(material)
        remap[index] = unique.index(material)
    polygon_indices = [remap[polygon.material_index] for polygon in obj.data.polygons]
    obj.data.materials.clear()
    for material in unique:
        obj.data.materials.append(material)
    for polygon, material_index in zip(obj.data.polygons, polygon_indices):
        polygon.material_index = material_index
    obj.data.update()
    obj.data.calc_loop_triangles()
    assert all(math.isfinite(value) for vertex in obj.data.vertices for value in vertex.co), obj.name
    assert all(triangle.area > 1e-13 for triangle in obj.data.loop_triangles), (obj.name, "degenerate triangle")
    assert obj.data.uv_layers.active, obj.name + " missing UV0"
    # Preserve a second authored UV channel in the FBX. UE may regenerate this
    # channel for lightmaps, but providing it avoids importer-dependent loss of
    # the destination channel on combined static meshes.
    uv0 = obj.data.uv_layers.active
    uv1 = obj.data.uv_layers.get("LightmapUV") or obj.data.uv_layers.new(name="LightmapUV")
    for source, target in zip(uv0.data, uv1.data):
        target.uv = source.uv
    obj.data.uv_layers.active = uv0
    bounds = _bounds_ue_cm(obj)
    size = [bounds[index + 3] - bounds[index] for index in range(3)]
    assert max(abs(actual - expected) for actual, expected in zip(size, expected_size_cm)) <= size_tolerance_cm, (obj.name, size, expected_size_cm)
    entry = dict(current)
    entry.update(
        bounds_cm=bounds,
        size_cm=size,
        triangles=len(obj.data.loop_triangles),
        vertices=len(obj.data.vertices),
        materials=[material.name for material in unique],
        texture="T_TunaWeaponCollection_Atlas",
        uv_channels_source=len(obj.data.uv_layers),
        collision_boxes=len(collisions),
    )
    export_collisions = []
    for index, (location, size_cm) in enumerate(collisions):
        bpy.ops.mesh.primitive_cube_add(size=1, location=ue(location))
        collider = bpy.context.object
        sx, sy, sz = size_cm
        collider.name = f"UCX_{obj.name}_{index:02d}"
        collider.dimensions = (sy / 100.0, sx / 100.0, sz / 100.0)
        bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
        export_collisions.append(collider)
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    for collider in export_collisions:
        collider.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.export_scene.fbx(
        filepath=str(OUT / "Models" / f"{obj.name}.fbx"), use_selection=True,
        object_types={"MESH"}, apply_unit_scale=True, apply_scale_options="FBX_SCALE_NONE",
        axis_forward="-Y", axis_up="Z", mesh_smooth_type="FACE", add_leaf_bones=False,
        bake_anim=False, path_mode="STRIP",
    )
    for collider in export_collisions:
        bpy.data.objects.remove(collider, do_unlink=True)
    obj.select_set(False)
    entries.append(entry)
    objects.append(obj)
    return obj


def save_family():
    manifest = {
        "family": family,
        "engine": "Unreal Engine 5.7",
        "axis_contract": "+X muzzle/forward, +Y right, +Z up",
        "unit_contract": "authoring cm, Blender m, FBX unit metadata, UE identity import",
        "pivot_contract": "receiver attachment reference at origin; centered laterally",
        "texture": "Textures/T_TunaWeaponCollection_Atlas.png",
        "ue_material_authoring": {
            "color_segmentation": "semantic material slots remain separate on every mesh",
            "editable_color_parameter": "Tint",
            "texture_usage": "ImageGen atlas multiplied by per-material Tint",
        },
        "materials": {
            key: {"name": materials[key].name, "atlas_cell": [values[0], values[1]], "metallic": values[2], "roughness": values[3], "emission": values[4]}
            for key, values in TILES.items()
        },
        "assets": entries,
        "total_triangles": sum(entry["triangles"] for entry in entries),
    }
    (OUT / "Manifests" / f"{family}.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT / f"TunaWeaponCollection_{family}.blend"))
    print(f"WEAPON_FAMILY_COMPLETE {family} {len(entries)} meshes {manifest['total_triangles']} triangles")


def setup_studio(resolution=(1400, 900)):
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_EEVEE_NEXT"
    scene.render.resolution_x, scene.render.resolution_y = resolution
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.view_settings.look = "AgX - Medium High Contrast"
    world = bpy.data.worlds.new("WeaponStudio") if not bpy.data.worlds else bpy.data.worlds[0]
    scene.world = world
    world.use_nodes = True
    world.node_tree.nodes["Background"].inputs["Color"].default_value = (0.035, 0.045, 0.055, 1)
    world.node_tree.nodes["Background"].inputs["Strength"].default_value = 0.55
    for name, location, energy, size in (
        ("Key", (-2.5, 0.5, 2.7), 1100, 2.5),
        ("Fill", (2.0, 0.2, 1.5), 700, 2.0),
        ("Rim", (0.0, 2.0, 2.4), 900, 2.0),
    ):
        data = bpy.data.lights.new("PREVIEW_" + name, "AREA")
        data.energy, data.shape, data.size = energy, "DISK", size
        light = bpy.data.objects.new("PREVIEW_" + name, data)
        scene.collection.objects.link(light)
        light.location = location
        light.rotation_euler = (-Vector(location)).to_track_quat("-Z", "Y").to_euler()
    camera_data = bpy.data.cameras.new("ReviewCamera")
    camera = bpy.data.objects.new("PREVIEW_Camera", camera_data)
    scene.collection.objects.link(camera)
    scene.camera = camera
    camera_data.lens = 62
    return camera


def render_view(camera, filepath, camera_ue_cm, target_ue_cm=(0, 0, 0), visible=None):
    if visible is not None:
        for obj in objects:
            obj.hide_render = obj not in visible
    camera.location = ue(camera_ue_cm)
    target = ue(target_ue_cm)
    camera.rotation_euler = (target - camera.location).to_track_quat("-Z", "Y").to_euler()
    bpy.context.scene.render.filepath = str(filepath)
    bpy.ops.render.render(write_still=True)
