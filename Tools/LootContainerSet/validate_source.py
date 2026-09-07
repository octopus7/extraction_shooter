"""Fresh-process Blender 4.5 source and independent FBX validation.

Run Blender --background --python Tools/LootContainerSet/validate_source.py.
Only source_validation.json is written; authored .blend/FBX files are read-only.
The exporter mirrors Y and reverses winding before FBX export. Undo that
compensation after importing and compare every triangle and corner attribute.
"""
import json
import math
import traceback
from pathlib import Path

import bmesh
import bpy
from mathutils import Matrix

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'TunaSweeper/SourceArt/Environment/LootContainerSet'
TOL = 1e-4
ATTR_TOL = 1e-4
EXPECTED_NAMES = {f'SM_LC_{kind}_{part}' for kind in ('Wood', 'Metal', 'Supply')
                  for part in ('Body', 'Lid')}
report = {'passed': False, 'assertions': 0, 'assets': [], 'failures': [],
          'blender_version': bpy.app.version_string,
          'bounds_tolerance_m': TOL,
          'validation': 'Fresh-process .blend reload, independent FBX reload, '
                        'triangle geometry/winding/UV/color parity; source files unchanged'}


def require(condition, message):
    report['assertions'] += 1
    if not condition:
        raise AssertionError(message)


def delta(a, b):
    require(len(a) == len(b), 'Vector lengths differ')
    return max(abs(x - y) for x, y in zip(a, b))


def bounds(mesh):
    return ([min(v.co[i] for v in mesh.vertices) for i in range(3)] +
            [max(v.co[i] for v in mesh.vertices) for i in range(3)])


def identity(obj):
    return max(abs(obj.matrix_world[i][j] - (1.0 if i == j else 0.0))
               for i in range(4) for j in range(4)) < TOL


def color_at(mesh, layer, loop):
    index = loop if layer.domain == 'CORNER' else mesh.loops[loop].vertex_index
    return tuple(layer.data[index].color)


def triangle_records(mesh):
    uv0, uv1 = mesh.uv_layers['UVMap'], mesh.uv_layers['DirtUV']
    colors = mesh.color_attributes['SurfaceParams']
    return [{'co': [tuple(mesh.vertices[i].co) for i in tri.vertices],
             'normal': tuple(tri.normal),
             'uv0': [tuple(uv0.data[i].uv) for i in tri.loops],
             'uv1': [tuple(uv1.data[i].uv) for i in tri.loops],
             'color': [color_at(mesh, colors, i) for i in tri.loops]}
            for tri in mesh.loop_triangles]


def mesh_audit(obj, expected=None, surface=True):
    mesh = obj.data
    mesh.calc_loop_triangles()
    require(len(mesh.loop_triangles) > 0, obj.name + ': empty mesh')
    require(all(math.isfinite(c) for v in mesh.vertices for c in v.co),
            obj.name + ': nonfinite vertex')
    require(all(t.area > 1e-10 for t in mesh.loop_triangles),
            obj.name + ': degenerate triangle')
    require(all(p.normal.length > .999 and all(math.isfinite(c) for c in p.normal)
                for p in mesh.polygons), obj.name + ': invalid face normal')
    bm = bmesh.new()
    try:
        bm.from_mesh(mesh)
        require(all(e.is_manifold for e in bm.edges), obj.name + ': nonmanifold/loose edge')
        require(all(e.is_contiguous for e in bm.edges), obj.name + ': inconsistent face winding')
        require(all(v.link_faces for v in bm.verts), obj.name + ': isolated vertex')
        bmesh.ops.triangulate(bm, faces=list(bm.faces))
        unseen = set(bm.faces)
        volumes = []
        while unseen:
            seed = unseen.pop()
            queue, component = [seed], [seed]
            while queue:
                for edge in queue.pop().edges:
                    for neighbor in edge.link_faces:
                        if neighbor in unseen:
                            unseen.remove(neighbor)
                            queue.append(neighbor)
                            component.append(neighbor)
            origin = component[0].verts[0].co.copy()
            volume = 0.0
            for face in component:
                a, b, c = (v.co - origin for v in face.verts)
                volume += a.dot(b.cross(c)) / 6.0
            require(volume > 1e-12, obj.name + ': nonpositive connected-solid volume')
            volumes.append(volume)
    finally:
        bm.free()
    result = {'triangles': len(mesh.loop_triangles), 'bounds_m': bounds(mesh),
              'connected_solids': len(volumes), 'minimum_solid_volume_m3': min(volumes),
              'summed_signed_volume_m3': sum(volumes), 'manifold': True,
              'consistent_winding': True, 'loose_geometry': 0,
              'degenerate_triangles': 0, 'valid_normals': True}
    if expected is not None:
        require(result['triangles'] == expected['triangles'], obj.name + ': triangle count differs')
        result['bounds_error_m'] = delta(result['bounds_m'], expected['bounds_m'])
        require(result['bounds_error_m'] < TOL, obj.name + ': bounds differ from manifest')
    if not surface:
        return result
    require(len(mesh.materials) == 1 and mesh.materials[0] is not None,
            obj.name + ': must have exactly one valid material slot')
    require(all(p.material_index == 0 for p in mesh.polygons), obj.name + ': material index')
    require(len(mesh.uv_layers) == 2 and
            [layer.name for layer in mesh.uv_layers] == ['UVMap', 'DirtUV'],
            obj.name + ': UV0 UVMap / UV1 DirtUV required')
    for layer in mesh.uv_layers:
        require(all(math.isfinite(c) for item in layer.data for c in item.uv),
                obj.name + ': nonfinite ' + layer.name)
    atlas = mesh.uv_layers['UVMap'].data
    require(all(-ATTR_TOL <= c <= 1.0 + ATTR_TOL for item in atlas for c in item.uv),
            obj.name + ': UV0 outside 0..1')
    for triangle in mesh.loop_triangles:
        for layer in mesh.uv_layers:
            a, b, c = [layer.data[i].uv for i in triangle.loops]
            area2 = abs((b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x))
            require(area2 > 1e-12, obj.name + ': zero-area ' + layer.name + ' triangle')
    surface_layer = mesh.color_attributes.get('SurfaceParams')
    require(surface_layer is not None and surface_layer.domain in {'POINT', 'CORNER'},
            obj.name + ': missing/invalid SurfaceParams')
    require(mesh.color_attributes.active_color is not None and
            mesh.color_attributes.active_color.name == 'SurfaceParams',
            obj.name + ': SurfaceParams must be active color')
    colors = [tuple(item.color) for item in surface_layer.data]
    require(colors and all(all(math.isfinite(c) and -ATTR_TOL <= c <= 1 + ATTR_TOL
                              for c in color) for color in colors),
            obj.name + ': invalid SurfaceParams range')
    require(all(c[1] > 0 and abs(c[2]) < ATTR_TOL and abs(c[3] - 1) < ATTR_TOL
                for c in colors), obj.name + ': expected roughness >0, B=0, A=1')
    result.update({'material_slots': 1, 'material': mesh.materials[0].name,
                   'uv_channels': ['UVMap', 'DirtUV'], 'zero_area_uv_triangles': 0,
                   'uv0_range_valid': True, 'surface_attribute': 'SurfaceParams',
                   'surface_domain': surface_layer.domain,
                   'metallic_range': [min(c[0] for c in colors), max(c[0] for c in colors)],
                   'roughness_range': [min(c[1] for c in colors), max(c[1] for c in colors)]})
    return result


def undo_fbx_compensation(obj):
    # Imported FBX nodes may express unit conversion as transforms. Bake only
    # into the transient audit copy, then undo the documented handedness change.
    obj.data.transform(obj.matrix_world)
    obj.matrix_world = Matrix.Identity(4)
    obj.data.transform(Matrix.Diagonal((1, -1, 1, 1)))
    bm = bmesh.new()
    try:
        bm.from_mesh(obj.data)
        bmesh.ops.reverse_faces(bm, faces=list(bm.faces))
        bm.to_mesh(obj.data)
    finally:
        bm.free()
    obj.data.update()


def triangle_parity(source, imported, name):
    require(len(source) == len(imported), name + ': parity triangle count')
    unmatched = list(source)
    maxima = {'position_m': 0.0, 'normal_component': 0.0, 'uv0': 0.0,
              'uv1': 0.0, 'color': 0.0}
    for candidate in imported:
        matches = []
        for index, original in enumerate(unmatched):
            for shift in range(3):
                order = [(i + shift) % 3 for i in range(3)]
                position_error = max(max(abs(a - b) for a, b in
                    zip(candidate['co'][i], original['co'][order[i]])) for i in range(3))
                if position_error >= TOL:
                    continue
                errors = {'position_m': position_error,
                          'normal_component': max(abs(a - b) for a, b in
                              zip(candidate['normal'], original['normal']))}
                for key in ('uv0', 'uv1', 'color'):
                    errors[key] = max(max(abs(a - b) for a, b in
                        zip(candidate[key][i], original[key][order[i]])) for i in range(3))
                # Same-coordinate triangles can exist on touching solids; choose
                # the candidate whose corner attributes also match.
                matches.append((max(errors.values()), index, errors))
        require(bool(matches), name + ': FBX triangle position/winding has no source match')
        _, index, errors = min(matches, key=lambda item: item[0])
        require(errors['normal_component'] < ATTR_TOL, name + ': FBX face normal differs')
        for key in ('uv0', 'uv1', 'color'):
            require(errors[key] < ATTR_TOL, name + ': FBX corner ' + key + ' differs')
        for key, value in errors.items():
            maxima[key] = max(maxima[key], value)
        unmatched.pop(index)
    require(not unmatched, name + ': unmatched source triangles')
    return maxima


def run():
    manifest = json.loads((OUT / 'model_manifest.json').read_text(encoding='utf-8'))
    entries = manifest['assets']
    require(len(entries) == 6 and {e['name'] for e in entries} == EXPECTED_NAMES,
            'Expected precisely six Body/Lid asset entries')
    require({p.stem for p in (OUT / 'Models').glob('*.fbx')} == EXPECTED_NAMES,
            'FBX file set differs from six core assets')
    bpy.ops.wm.open_mainfile(filepath=str(OUT / 'LootContainerSet.blend'))
    require(abs(bpy.context.scene.unit_settings.scale_length - 1.0) < TOL,
            'Source scene must use meter scale 1')
    require(bpy.context.scene.unit_settings.system == 'METRIC', 'Source scene must use METRIC units')
    for entry in entries:
        name = entry['name']
        source = bpy.data.objects.get(name)
        require(source is not None and source.type == 'MESH', name + ': missing source object')
        require(identity(source), name + ': nonidentity source location/rotation/scale')
        require(not source.modifiers, name + ': unapplied source modifiers')
        source_result = mesh_audit(source, entry)
        source_records = triangle_records(source.data)
        box = source_result['bounds_m']
        require(abs(box[0] + box[3]) < TOL and abs(box[2]) < TOL,
                name + ': X-centered pivot and lower Z=0 required')
        if name.endswith('_Body'):
            require(abs(box[1] + box[4]) < TOL, name + ': body Y must be centered')
        else:
            require(abs(box[1]) < TOL and box[4] > TOL,
                    name + ': lid must start at rear Y=0 and extend +Y')
        collision_boxes = [list(lo) + list(hi) for lo, hi in entry['collision_boxes']]
        require(bool(collision_boxes), name + ': missing simple collision')
        for collision_box in collision_boxes:
            require(len(collision_box) == 6 and all(math.isfinite(c) for c in collision_box)
                    and all(collision_box[i + 3] > collision_box[i] for i in range(3)),
                    name + ': invalid collision-box bounds')
            require(all(collision_box[i] >= box[i] - TOL and
                        collision_box[i + 3] <= box[i + 3] + TOL for i in range(3)),
                    name + ': collision extends outside render bounds')
        before = set(bpy.data.objects)
        source.name = name + '_audit_source'
        try:
            bpy.ops.import_scene.fbx(filepath=str(OUT / 'Models' / (name + '.fbx')),
                                     colors_type='LINEAR')
            imported = set(bpy.data.objects) - before
            meshes = [o for o in imported if o.type == 'MESH']
            render = [o for o in meshes if not o.name.startswith('UCX_')]
            require(len(render) == 1, name + ': expected one FBX render mesh')
            render = render[0]
            require(render.name == name, name + ': FBX render name differs')
            raw_transform = [list(row) for row in render.matrix_world]
            undo_fbx_compensation(render)
            imported_result = mesh_audit(render, entry)
            require(identity(render), name + ': normalized FBX nonidentity transform')
            require(imported_result['connected_solids'] == source_result['connected_solids'],
                    name + ': FBX connected-solid count differs')
            parity = triangle_parity(source_records, triangle_records(render.data), name)
            collisions = [o for o in meshes if o.name.startswith('UCX_')]
            require(len(collisions) == len(collision_boxes), name + ': FBX collision count differs')
            prefix = 'UCX_' + name + '_'
            require(all(o.name.startswith(prefix) and o.name[len(prefix):].isdigit()
                        for o in collisions), name + ': invalid UCX naming')
            unmatched = list(collision_boxes)
            collision_error = 0.0
            for collision in collisions:
                undo_fbx_compensation(collision)
                audit = mesh_audit(collision, surface=False)
                require(audit['connected_solids'] == 1 and audit['triangles'] == 12,
                        collision.name + ': collision must be one closed box')
                distances = [delta(audit['bounds_m'], expected) for expected in unmatched]
                nearest = min(range(len(distances)), key=distances.__getitem__)
                require(distances[nearest] < TOL, collision.name + ': UCX bounds differ')
                collision_error = max(collision_error, distances[nearest])
                # Bounds alone could allow a tetrahedron or skewed solid. Every
                # vertex must coincide with a corner of its axis-aligned box.
                bb = audit['bounds_m']
                require(all(min(abs(v.co[i] - bb[i]), abs(v.co[i] - bb[i + 3])) < TOL
                            for v in collision.data.vertices for i in range(3)),
                        collision.name + ': UCX is not an axis-aligned box')
                unmatched.pop(nearest)
            report['assets'].append({'name': name, 'source': source_result,
                                     'fbx_reload': imported_result,
                                     'fbx_raw_world_matrix': raw_transform,
                                     'triangle_corner_parity_max_errors': parity,
                                     'collision_boxes': len(collisions),
                                     'collision_names': sorted(o.name for o in collisions),
                                     'collision_max_bounds_error_m': collision_error,
                                     'identity_source_transform': True, 'pivot_valid': True,
                                     'dimensions_m': [box[i + 3] - box[i] for i in range(3)]})
        finally:
            for obj in set(bpy.data.objects) - before:
                bpy.data.objects.remove(obj, do_unlink=True)
            source.name = name
    report['total_unique_asset_triangles'] = sum(e['source']['triangles'] for e in report['assets'])
    report['total_collision_boxes'] = sum(e['collision_boxes'] for e in report['assets'])
    report['passed'] = True


try:
    run()
except Exception as error:
    report['failures'].append(str(error))
    report['traceback'] = traceback.format_exc()
    raise
finally:
    OUT.mkdir(parents=True, exist_ok=True)
    (OUT / 'source_validation.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
print('LC_SOURCE_VALIDATION_PASSED', report['total_unique_asset_triangles'], report['assertions'])
