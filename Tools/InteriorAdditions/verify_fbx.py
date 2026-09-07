"""Read-only FBX round-trip checks in a fresh Blender 4.5 process.

blender -b --factory-startup --python Tools/InteriorAdditions/verify_fbx.py
Only the JSON verification report is written; source/FBX files remain unchanged.
"""
from pathlib import Path
import hashlib
import json
import math
import bpy
from mathutils import Vector


ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "TunaSweeper/SourceArt/Environment/InteriorAdditions"
MANIFEST = json.loads((SOURCE / "model_manifest.json").read_text(encoding="utf-8"))
report = {"verification": "fresh Blender process FBX round trip", "blender": bpy.app.version_string,
          "assets": [], "passed": False}
for entry in MANIFEST["assets"]:
    path = SOURCE / "Models" / (entry["name"] + ".fbx")
    before = hashlib.sha256(path.read_bytes()).hexdigest()
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=str(path), use_anim=False)
    meshes = [obj for obj in bpy.context.scene.objects if obj.type == "MESH" and not obj.name.startswith("UCX_")]
    collisions = [obj for obj in bpy.context.scene.objects if obj.type == "MESH" and obj.name.startswith("UCX_")]
    assert len(meshes) == 1, (entry["name"], "Render mesh count", len(meshes))
    obj = meshes[0]
    obj.data.calc_loop_triangles()
    triangles = len(obj.data.loop_triangles)
    assert triangles == entry["triangles"], (entry["name"], "Triangle count", triangles, entry["triangles"])
    assert all(math.isfinite(value) for vertex in obj.data.vertices for value in vertex.co), entry["name"]
    assert all(triangle.area > 1e-12 for triangle in obj.data.loop_triangles), (entry["name"], "Degenerate triangle")
    assert obj.data.uv_layers.active, (entry["name"], "Missing UV map")
    uv = obj.data.uv_layers.active
    assert all(math.isfinite(value) and -1e-6 <= value <= 1.000001 for corner in uv.data for value in corner.uv), (entry["name"], "Invalid atlas UV")
    assert all(material is not None for material in obj.data.materials), entry["name"]
    materials = [material.name for material in obj.data.materials]
    assert set(materials) == set(entry["materials"]), (entry["name"], "Materials", materials, entry["materials"])
    points = [obj.matrix_world @ Vector(corner) for corner in obj.bound_box]
    bounds = [min(point[axis] for point in points) * 100.0 for axis in range(3)]
    bounds += [max(point[axis] for point in points) * 100.0 for axis in range(3)]
    error = max(abs(actual - expected) for actual, expected in zip(bounds, entry["bounds_cm"]))
    assert error < 0.1, (entry["name"], "Bounds mismatch cm", error, bounds, entry["bounds_cm"])
    if "collision_boxes" in entry:
        assert len(collisions) == entry["collision_boxes"], (entry["name"], "Collision mesh count")
    assert hashlib.sha256(path.read_bytes()).hexdigest() == before, "FBX changed during verification"
    report["assets"].append({"name": entry["name"], "sha256": before,
                             "triangles": triangles, "vertices": len(obj.data.vertices),
                             "uv_channels": len(obj.data.uv_layers), "materials": materials,
                             "collision_meshes": len(collisions), "bounds_cm": bounds,
                             "max_bounds_error_cm": error, "passed": True})
report["total_triangles"] = sum(entry["triangles"] for entry in report["assets"])
report["passed"] = True
destination = SOURCE / "fbx_validation.json"
destination.write_text(json.dumps(report, indent=2), encoding="utf-8")
print("INTERIOR_FBX_VALIDATION_PASSED " + str(len(report["assets"])))
