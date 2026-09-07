"""Read-only validation of the complete facility FBX collection in Blender 4.5.

Run Blender --background --factory-startup --python-exit-code 1 --python this_file.py.
Only fbx_validation.json is written. Every render mesh and UCX box is imported
into a fresh scene and compared with the source manifests in authoring axes.
"""
from pathlib import Path
import hashlib
import json
import math
import traceback

import bpy
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'TunaSweeper/SourceArt/Environment/FacilityRooms'
TOLERANCE_CM = .1


def load_assets():
    merged = SOURCE / 'model_manifest.json'
    paths = [merged] if merged.exists() else sorted((SOURCE / 'Manifests').glob('*.json'))
    assets = {}
    inputs = []
    for path in paths:
        data = json.loads(path.read_text(encoding='utf-8'))
        rows = data.get('assets', [])
        if not rows or not all('target_size_cm' in row and 'collision_specs_cm' in row for row in rows):
            continue
        inputs.append(path)
        for row in rows:
            assert row['name'] not in assets, ('Duplicate asset', row['name'])
            assets[row['name']] = row
    assert assets, 'No model manifests available'
    return list(assets.values()), inputs


def digest(paths):
    return {str(path.relative_to(SOURCE)).replace('\\', '/'): hashlib.sha256(path.read_bytes()).hexdigest()
            for path in sorted(set(paths))}


def bounds_cm(obj):
    points = [obj.matrix_world @ vertex.co for vertex in obj.data.vertices]
    return [min(point[axis] for point in points) * 100 for axis in range(3)] + [
        max(point[axis] for point in points) * 100 for axis in range(3)]


def spec_bounds(spec):
    return [spec['center'][axis] - spec['size'][axis] / 2 for axis in range(3)] + [
        spec['center'][axis] + spec['size'][axis] / 2 for axis in range(3)]


def bound_error(actual, expected):
    return max(abs(a - b) for a, b in zip(actual, expected))


def verify_open_space(entry, mesh, colliders):
    """Probe actual triangles and imported collision through useful openings."""
    name = entry['name'].removeprefix('SM_FacilityRooms_')
    rays = []
    if name == 'Doorway200':
        rays = [([x, -40, z], [x, 40, z]) for x in (-55, 0, 55) for z in (10, 115, 225)]
    elif name in {'HatchLanding200', 'WoodHatchLanding200'}:
        rays = [([x, y, -15], [x, y, 35]) for x in (-40, 0, 40) for y in (-40, 0, 40)]
    elif name == 'WoodWindowWall200':
        # Keep probes inside the 95 x 95 cm aperture and away from the central
        # vertical/horizontal muntins, which are legitimate window geometry.
        rays = [([x, -45, z], [x, 45, z]) for x in (-40, -20, 20, 40) for z in (92, 105, 160, 173)]
    for start, end in rays:
        for obj in [mesh] + colliders:
            inverse = obj.matrix_world.inverted()
            origin = inverse @ (Vector(start) / 100)
            target = inverse @ (Vector(end) / 100)
            direction = target - origin
            hit = obj.ray_cast(origin, direction.normalized(), distance=direction.length)[0]
            assert not hit, ('blocked actual opening', name, obj.name, start, end)
    return [{'start_cm': start, 'end_cm': end} for start, end in rays]


def verify_one(entry):
    path = SOURCE / 'Models' / (entry['name'] + '.fbx')
    before = hashlib.sha256(path.read_bytes()).hexdigest()
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=str(path), use_anim=False)
    meshes = [obj for obj in bpy.context.scene.objects if obj.type == 'MESH' and not obj.name.startswith('UCX_')]
    collisions = [obj for obj in bpy.context.scene.objects if obj.type == 'MESH' and obj.name.startswith('UCX_')]
    assert len(meshes) == 1, ('render mesh count', len(meshes))
    obj = meshes[0]
    assert obj.name == entry['name'], ('render mesh name', obj.name)
    obj.data.calc_loop_triangles()
    triangles = len(obj.data.loop_triangles)
    assert triangles == entry['triangles'], ('triangle count', triangles, entry['triangles'])
    assert all(math.isfinite(value) for vertex in obj.data.vertices for value in vertex.co), 'nonfinite vertex'
    assert all(triangle.area > 1e-13 for triangle in obj.data.loop_triangles), 'degenerate triangle'
    assert len(obj.data.uv_layers) == 2, ('UV channel count', len(obj.data.uv_layers))
    for uv in obj.data.uv_layers:
        assert all(math.isfinite(value) and -1e-6 <= value <= 1.000001
                   for corner in uv.data for value in corner.uv), ('invalid UV values', uv.name)
    materials = [material.name if material else None for material in obj.data.materials]
    assert len(materials) == len(entry['materials']) and set(materials) == set(entry['materials']), (
        'material slots', materials, entry['materials'])
    assert all(0 <= polygon.material_index < len(materials) for polygon in obj.data.polygons), 'invalid material index'
    actual_bounds = bounds_cm(obj)
    bounds_error = bound_error(actual_bounds, entry['bounds_cm'])
    assert bounds_error < TOLERANCE_CM, ('bounds cm', actual_bounds, entry['bounds_cm'], bounds_error)
    assert max(abs(value) for value in obj.matrix_world.translation) < 1e-6, 'nonzero mesh pivot'
    specs = entry['collision_specs_cm']
    assert len(collisions) == len(specs) == entry['collision_boxes'], ('UCX count', len(collisions), len(specs))
    collision_lookup = {obj.name: obj for obj in collisions}
    collision_report = []
    for index, spec in enumerate(specs):
        name = 'UCX_' + entry['name'] + '_%02d' % index
        collider = collision_lookup.get(name)
        assert collider is not None, ('missing named UCX box', name)
        collider.data.calc_loop_triangles()
        assert len(collider.data.vertices) == 8 and len(collider.data.loop_triangles) == 12, ('not a box', name)
        assert all(math.isfinite(value) for vertex in collider.data.vertices for value in vertex.co), ('invalid UCX vertex', name)
        assert all(triangle.area > 1e-13 for triangle in collider.data.loop_triangles), ('degenerate UCX', name)
        actual = bounds_cm(collider)
        expected = spec_bounds(spec)
        error = bound_error(actual, expected)
        assert error < TOLERANCE_CM, ('UCX bounds cm', name, actual, expected, error)
        collision_report.append({'name': name, 'bounds_cm': actual, 'max_bounds_error_cm': error})
    after = hashlib.sha256(path.read_bytes()).hexdigest()
    assert before == after, 'FBX file changed during verification'
    opening_probes = verify_open_space(entry, obj, collisions)
    return {'name': entry['name'], 'passed': True, 'triangles': triangles, 'vertices': len(obj.data.vertices),
            'uv_channels': 2, 'materials': materials, 'bounds_cm': actual_bounds,
            'max_bounds_error_cm': bounds_error, 'collision_boxes': len(collisions),
            'collision_validation': collision_report, 'opening_ray_probes': opening_probes,
            'sha256_before': before, 'sha256_after': after}


def main():
    entries, manifest_paths = load_assets()
    protected = manifest_paths + [SOURCE / 'Models' / (entry['name'] + '.fbx') for entry in entries]
    protected += list(SOURCE.glob('*.blend'))
    protected += [path for folder in ('References', 'Textures') for path in (SOURCE / folder).rglob('*') if path.is_file()]
    before = digest(protected)
    report = {'verification': 'fresh Blender scenes: render mesh and individual UCX bounds',
              'blender': bpy.app.version_string, 'axis_contract': 'Blender X width/-Y front/Z up, cm',
              'bounds_tolerance_cm': TOLERANCE_CM, 'assets': [], 'errors': [], 'passed': False}
    for entry in entries:
        try:
            report['assets'].append(verify_one(entry))
        except Exception as error:
            report['errors'].append({'name': entry['name'], 'error': str(error), 'traceback': traceback.format_exc()})
            print('FACILITY_FBX_ASSET_FAILED', entry['name'], str(error))
    after = digest(protected)
    report['source_hashes_before'] = before
    report['source_hashes_after'] = after
    report['source_files_unchanged'] = before == after
    report['expected_asset_count'] = len(entries)
    report['validated_asset_count'] = len(report['assets'])
    report['total_triangles'] = sum(entry['triangles'] for entry in report['assets'])
    report['total_collision_boxes'] = sum(entry['collision_boxes'] for entry in report['assets'])
    report['passed'] = not report['errors'] and report['source_files_unchanged']
    (SOURCE / 'fbx_validation.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print('FACILITY_FBX_VALIDATION', json.dumps({key: report[key] for key in (
        'passed', 'expected_asset_count', 'validated_asset_count', 'total_triangles', 'total_collision_boxes', 'source_files_unchanged')}))
    assert report['passed'], 'Facility FBX validation failed; see fbx_validation.json'


if __name__ == '__main__':
    main()
