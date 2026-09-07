"""Read-only facility opening and walkway checks using authored UCX boxes.

Use Python 3 or Blender --background --python this_file.py. Only
layout_validation.json is written. This checks sampled static collision
clearance; it does not claim runtime ladder climbing or navmesh validation.
"""
from pathlib import Path
import hashlib
import json
import math
from itertools import product

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'TunaSweeper/SourceArt/Environment/FacilityRooms'
CAPSULE_RADIUS_CM = 35.0
SAMPLE_INTERVAL_CM = 10.0
BODY_Z_RANGE_CM = (35.0, 180.0)
EPSILON = .01


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
    return assets, inputs


def digest(paths):
    return {str(path.relative_to(SOURCE)).replace('\\', '/'): hashlib.sha256(path.read_bytes()).hexdigest()
            for path in sorted(set(paths))}


def local_bounds(spec):
    return [spec['center'][axis] - spec['size'][axis] / 2 for axis in range(3)] + [
        spec['center'][axis] + spec['size'][axis] / 2 for axis in range(3)]


def world_bounds(spec, placement):
    """Transform each source box corner: reflect Y, rotate UE yaw, translate."""
    angle = math.radians(placement.get('yaw_deg', 0))
    cosine, sine = math.cos(angle), math.sin(angle)
    translation = placement['location_cm']
    scale = placement.get('scale', [1, 1, 1])
    corners = []
    for signs in product((-1, 1), repeat=3):
        point = [(spec['center'][axis] + signs[axis] * spec['size'][axis] / 2) * scale[axis] for axis in range(3)]
        x, y, z = point[0], -point[1], point[2]
        corners.append([translation[0] + cosine*x - sine*y,
                        translation[1] + sine*x + cosine*y, translation[2] + z])
    return [min(point[axis] for point in corners) for axis in range(3)] + [
        max(point[axis] for point in corners) for axis in range(3)]


def central_gap(specs, axis, other_coordinates):
    """Gap around local zero at the given cross section, measured from boxes."""
    intervals = []
    for spec in specs:
        bounds = local_bounds(spec)
        if all(bounds[a] - EPSILON <= value <= bounds[a+3] + EPSILON for a, value in other_coordinates.items()):
            intervals.append((bounds[axis], bounds[axis+3]))
    if any(low + EPSILON < 0 < high - EPSILON for low, high in intervals):
        return 0.0
    left = [high for low, high in intervals if high <= EPSILON]
    right = [low for low, high in intervals if low >= -EPSILON]
    assert left and right, ('Missing sides around opening', axis, intervals)
    return min(right) - max(left)


def verify_openings(assets):
    results = []
    doorway = assets['SM_FacilityRooms_Doorway200']
    specs = doorway['collision_specs_cm']
    width = central_gap(specs, 0, {1: 0, 2: 115})
    clear_top = min(local_bounds(spec)[2] for spec in specs if
                    local_bounds(spec)[0] < 0 < local_bounds(spec)[3] and
                    local_bounds(spec)[1] <= 0 <= local_bounds(spec)[4])
    results.append({'name': doorway['name'], 'expected_width_cm': 120, 'collision_width_cm': width,
                    'collision_height_cm': clear_top, 'passed': abs(width - 120) < .1 and clear_top >= 229.9})
    for hatch_name in ('HatchLanding200', 'WoodHatchLanding200'):
        hatch = assets['SM_FacilityRooms_' + hatch_name]
        width = central_gap(hatch['collision_specs_cm'], 0, {1: 0, 2: 10})
        depth = central_gap(hatch['collision_specs_cm'], 1, {0: 0, 2: 10})
        results.append({'name': hatch['name'], 'expected_width_depth_cm': [90, 90],
                        'collision_width_depth_cm': [width, depth],
                        'passed': abs(width - 90) < .1 and abs(depth - 90) < .1})
    window = assets['SM_FacilityRooms_WoodWindowWall200']
    shell_specs, allowed_muntins, blockers = [], [], []
    for index, spec in enumerate(window['collision_specs_cm']):
        bounds = local_bounds(spec)
        overlap_x = min(bounds[3], 47.5) - max(bounds[0], -47.5)
        overlap_z = min(bounds[5], 180) - max(bounds[2], 85)
        if overlap_x > .1 and overlap_z > .1:
            # Only narrow central bars contained inside the intended aperture
            # may occupy its interior. Full-wall and oversized boxes fail.
            vertical = (spec['size'][0] <= 8 and abs(spec['center'][0]) <= 4
                        and bounds[2] >= 84.9 and bounds[5] <= 180.1)
            horizontal = (spec['size'][2] <= 8 and abs(spec['center'][2] - 132.5) <= 4
                          and bounds[0] >= -47.6 and bounds[3] <= 47.6)
            (allowed_muntins if vertical or horizontal else blockers).append({'index': index, 'bounds_cm': bounds})
        else:
            shell_specs.append(spec)
    width = central_gap(shell_specs, 0, {1: 0, 2: 105})
    crossing_x = [local_bounds(spec) for spec in shell_specs
                  if local_bounds(spec)[0] <= 20 <= local_bounds(spec)[3]
                  and local_bounds(spec)[1] <= 0 <= local_bounds(spec)[4]]
    bottom = max(bounds[5] for bounds in crossing_x if bounds[5] < 132.5)
    top = min(bounds[2] for bounds in crossing_x if bounds[2] > 132.5)
    results.append({'name': window['name'], 'expected_width_height_cm': [95, 95],
                    'expected_z_range_cm': [85, 180], 'collision_width_height_cm': [width, top-bottom],
                    'collision_z_range_cm': [bottom, top], 'allowed_muntins': allowed_muntins,
                    'interior_blocking_boxes': blockers,
                    'passed': not blockers and abs(width-95) < .1 and abs(bottom-85) < .1 and abs(top-180) < .1})
    return results


def circle_aabb_distance(point, bounds):
    nearest = [min(max(point[axis], bounds[axis]), bounds[axis+3]) for axis in (0, 1)]
    return math.hypot(point[0] - nearest[0], point[1] - nearest[1])


def check_level(level, assets):
    colliders = []
    placements = level['placements']
    for placement in placements:
        assert placement['mesh'] in assets, ('Missing placement mesh', placement['mesh'])
        for index, spec in enumerate(assets[placement['mesh']]['collision_specs_cm']):
            colliders.append({'label': placement['label'], 'mesh': placement['mesh'],
                              'collision_index': index, 'bounds_cm': world_bounds(spec, placement)})
    floor_z = level['floor_z_cm']
    min_z, max_z = floor_z + BODY_Z_RANGE_CM[0], floor_z + BODY_Z_RANGE_CM[1]
    body_colliders = [collider for collider in colliders if collider['bounds_cm'][2] < max_z - EPSILON
                      and collider['bounds_cm'][5] > min_z + EPSILON]
    segments = []
    checks = level.get('walkway_checks', level.get('walkways', []))
    assert checks, ('No walkway checks', level['name'])
    for index, segment in enumerate(checks):
        x0, y0, x1, y1 = segment
        length = math.hypot(x1-x0, y1-y0)
        steps = max(1, math.ceil(length / SAMPLE_INTERVAL_CM))
        hits = {}
        minimum_clearance = None
        for step in range(steps + 1):
            point = [x0 + (x1-x0)*step/steps, y0 + (y1-y0)*step/steps]
            for collider in body_colliders:
                distance = circle_aabb_distance(point, collider['bounds_cm'])
                clearance = distance - CAPSULE_RADIUS_CM
                minimum_clearance = clearance if minimum_clearance is None else min(minimum_clearance, clearance)
                if clearance < -EPSILON:
                    key = (collider['label'], collider['collision_index'])
                    if key not in hits:
                        hits[key] = dict(collider, first_blocked_point_cm=point,
                                         last_blocked_point_cm=point, blocked_samples=0,
                                         minimum_clearance_cm=clearance)
                    hits[key]['blocked_samples'] += 1
                    hits[key]['last_blocked_point_cm'] = point
                    hits[key]['minimum_clearance_cm'] = min(hits[key]['minimum_clearance_cm'], clearance)
        segments.append({'index': index, 'segment_cm': segment, 'samples': steps+1,
                         'minimum_clearance_cm': minimum_clearance, 'blocked_colliders': list(hits.values()),
                         'passed': not hits})
    return {'name': level['name'], 'placement_count': len(placements), 'collision_box_count': len(colliders),
            'body_z_range_cm': [min_z, max_z], 'walkway_checks': segments,
            'passed': all(segment['passed'] for segment in segments)}


def main():
    assets, paths = load_assets()
    layout_path = SOURCE / 'level_layout.json'
    layout = json.loads(layout_path.read_text(encoding='utf-8'))
    paths.append(layout_path)
    before = digest(paths)
    report = {'verification': 'authored collision AABB transformed to UE coordinates; sampled capsule path clearance',
              'capsule_radius_cm': CAPSULE_RADIUS_CM, 'sample_interval_cm': SAMPLE_INTERVAL_CM,
              'body_height_above_floor_cm': list(BODY_Z_RANGE_CM),
              'axis_contract': 'Blender X/-Y/Z -> UE X/+Y/Z, then yaw and location',
              'opening_checks': [], 'levels': [], 'errors': [], 'passed': False}
    try:
        report['opening_checks'] = verify_openings(assets)
    except Exception as error:
        report['errors'].append({'check': 'openings', 'error': str(error)})
    for level in layout['levels']:
        try:
            report['levels'].append(check_level(level, assets))
        except Exception as error:
            report['errors'].append({'check': level['name'], 'error': str(error)})
    after = digest(paths)
    report['source_hashes_before'] = before
    report['source_hashes_after'] = after
    report['source_files_unchanged'] = before == after
    report['passed'] = (not report['errors'] and report['source_files_unchanged']
                        and all(item['passed'] for item in report['opening_checks'])
                        and all(item['passed'] for item in report['levels']))
    (SOURCE / 'layout_validation.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print('FACILITY_LAYOUT_VALIDATION', json.dumps({'passed': report['passed'], 'errors': report['errors'],
          'openings': report['opening_checks'], 'levels': [{'name': item['name'], 'passed': item['passed'],
          'blocked_segments': [{'index': segment['index'], 'colliders': sorted({hit['label'] for hit in segment['blocked_colliders']})}
                               for segment in item['walkway_checks'] if not segment['passed']]}
          for item in report['levels']]}))
    assert report['passed'], 'Facility layout validation failed; see layout_validation.json'


if __name__ == '__main__':
    main()
