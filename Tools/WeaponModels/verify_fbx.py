"""Fresh-process read-only FBX round-trip checks for the six weapon models."""
from pathlib import Path
import hashlib
import json
import math
import bpy
from mathutils import Vector


ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "TunaSweeper/SourceArt/Weapons/TunaWeaponCollection"
manifest = json.loads((SOURCE / "model_manifest.json").read_text(encoding="utf-8"))
report = {
    "verification": "fresh Blender process FBX round trip",
    "blender": bpy.app.version_string,
    "axis_contract": manifest["axis_contract"],
    "assets": [],
    "passed": False,
}


def bounds_ue_cm(obj):
    points = [obj.matrix_world @ Vector(corner) for corner in obj.bound_box]
    ue_points = [Vector((-point.y * 100.0, -point.x * 100.0, point.z * 100.0)) for point in points]
    return [min(point[axis] for point in ue_points) for axis in range(3)] + [max(point[axis] for point in ue_points) for axis in range(3)]


for entry in manifest["assets"]:
    path = SOURCE / "Models" / f"{entry['name']}.fbx"
    digest_before = hashlib.sha256(path.read_bytes()).hexdigest()
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=str(path), use_anim=False)
    meshes = [obj for obj in bpy.context.scene.objects if obj.type == "MESH" and not obj.name.startswith("UCX_")]
    collisions = [obj for obj in bpy.context.scene.objects if obj.type == "MESH" and obj.name.startswith("UCX_")]
    assert len(meshes) == 1, (entry["name"], "render mesh count", len(meshes))
    obj = meshes[0]
    obj.data.calc_loop_triangles()
    triangles = len(obj.data.loop_triangles)
    assert triangles == entry["triangles"], (entry["name"], triangles, entry["triangles"])
    assert all(math.isfinite(value) for vertex in obj.data.vertices for value in vertex.co), entry["name"]
    assert all(triangle.area > 1e-12 for triangle in obj.data.loop_triangles), (entry["name"], "degenerate triangle")
    assert obj.data.uv_layers.active, (entry["name"], "missing UV0")
    uv = obj.data.uv_layers.active
    assert all(math.isfinite(value) and -1e-6 <= value <= 1.000001 for corner in uv.data for value in corner.uv), (entry["name"], "invalid atlas UV")
    material_names = [material.name for material in obj.data.materials]
    assert all(material is not None for material in obj.data.materials), entry["name"]
    assert set(material_names) == set(entry["materials"]), (entry["name"], material_names, entry["materials"])
    actual_bounds = bounds_ue_cm(obj)
    bounds_error = max(abs(actual - expected) for actual, expected in zip(actual_bounds, entry["bounds_cm"]))
    assert bounds_error < 0.1, (entry["name"], "bounds mismatch cm", bounds_error)
    assert len(collisions) == entry["collision_boxes"] and len(collisions) >= 2, (entry["name"], "collision count")
    sockets = {socket["name"]: socket for socket in entry["sockets"]}
    assert set(sockets) == {"MuzzleSocket", "LaserSightSocket", "ShellEjectionSocket"}, entry["name"]
    assert sockets["MuzzleSocket"]["location_cm"][0] >= actual_bounds[3] - 1.5, (entry["name"], "muzzle socket not near forward bound")
    assert sockets["ShellEjectionSocket"]["location_cm"][1] > 0, (entry["name"], "shell socket must be on +Y/right")
    assert all(socket["rotation_deg"] == [0.0, 0.0, 0.0] and socket["scale"] == [1.0, 1.0, 1.0] for socket in sockets.values())
    assert hashlib.sha256(path.read_bytes()).hexdigest() == digest_before, "FBX changed during verification"
    report["assets"].append({
        "name": entry["name"], "sha256": digest_before,
        "triangles": triangles, "vertices_round_trip": len(obj.data.vertices), "vertices_source": entry["vertices"],
        "uv_channels": len(obj.data.uv_layers), "materials": material_names,
        "collision_meshes": len(collisions), "bounds_cm": actual_bounds,
        "max_bounds_error_cm": bounds_error, "sockets_from_manifest": list(sockets),
        "passed": True,
    })

report["total_triangles"] = sum(entry["triangles"] for entry in report["assets"])
report["texture_sha256"] = hashlib.sha256((SOURCE / manifest["texture"]).read_bytes()).hexdigest()
report["passed"] = True
(SOURCE / "fbx_validation.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
print(f"WEAPON_FBX_VALIDATION_PASSED {len(report['assets'])} assets")
