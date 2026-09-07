"""Read-only source and FBX audit in a fresh Blender 4.5 background process.

Run: blender --background --python Tools/LabSupplyProps/validate_source.py
Only source_validation.json is written; the .blend and FBX files stay unchanged.
"""
import importlib.util
import json
import math
import sys
import traceback
from pathlib import Path

import bmesh
import bpy
from mathutils import Matrix

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'TunaSweeper/SourceArt/Environment/LabSupplyProps'
TOL = 1e-5
report = {'passed': False, 'assets': [], 'failures': [],
          'validation': 'fresh .blend load and independent FBX reload; no source writes'}


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def bounds(mesh):
    return ([min(v.co[i] for v in mesh.vertices) for i in range(3)] +
            [max(v.co[i] for v in mesh.vertices) for i in range(3)])


def max_error(a, b):
    require(len(a) == len(b), 'Bounds/vector lengths differ')
    return max(abs(x - y) for x, y in zip(a, b))


def mesh_audit(obj, expect, check_surface=True):
    mesh = obj.data
    mesh.calc_loop_triangles()
    require(len(mesh.loop_triangles) > 0, obj.name + ': empty mesh')
    require(all(math.isfinite(c) for v in mesh.vertices for c in v.co),
            obj.name + ': nonfinite vertex')
    require(all(t.area > 1e-10 for t in mesh.loop_triangles),
            obj.name + ': degenerate triangle')
    require(all(p.normal.length > .99 and all(math.isfinite(c) for c in p.normal)
                for p in mesh.polygons), obj.name + ': invalid face normal')
    bm = bmesh.new()
    try:
        bm.from_mesh(mesh)
        require(all(e.is_manifold for e in bm.edges), obj.name + ': nonmanifold edge')
        require(all(v.link_faces for v in bm.verts), obj.name + ': isolated vertex')
        bmesh.ops.triangulate(bm, faces=list(bm.faces))
        unseen = set(bm.faces)
        volumes = []
        while unseen:
            face = unseen.pop()
            queue, connected = [face], [face]
            while queue:
                current = queue.pop()
                for edge in current.edges:
                    for neighbor in edge.link_faces:
                        if neighbor in unseen:
                            unseen.remove(neighbor)
                            queue.append(neighbor)
                            connected.append(neighbor)
            # Shift origin into each component to reduce cancellation in thin boxes.
            origin = connected[0].verts[0].co.copy()
            volume = 0.0
            for face in connected:
                a, b, c = (v.co - origin for v in face.verts)
                volume += a.dot(b.cross(c)) / 6.0
            require(volume > 1e-12, obj.name + ': nonpositive solid volume')
            volumes.append(volume)
    finally:
        bm.free()
    result = {'triangles': len(mesh.loop_triangles), 'bounds_m': bounds(mesh),
              'connected_solids': len(volumes), 'minimum_solid_volume_m3': min(volumes),
              'summed_signed_volume_m3': sum(volumes), 'manifold': True,
              'degenerate_triangles': 0, 'finite_normals': True}
    if expect is not None:
        require(len(mesh.loop_triangles) == expect['triangles'], obj.name + ': triangle count')
        result['bounds_error_m'] = max_error(result['bounds_m'], expect['bounds_m'])
        require(result['bounds_error_m'] < TOL, obj.name + ': bounds differ')
    if check_surface:
        require(len(mesh.materials) == 1, obj.name + ': material slot count')
        require(mesh.materials[0] is not None and
                mesh.materials[0].name.split('.')[0] == 'M_LSP_Surface',
                obj.name + ': shared material name')
        require(all(p.material_index == 0 for p in mesh.polygons), obj.name + ': material index')
        require(len(mesh.uv_layers) == 2 and mesh.uv_layers.get('DirtUV') is not None,
                obj.name + ': UV channel count/name')
        for layer in mesh.uv_layers:
            require(all(math.isfinite(c) for uv in layer.data for c in uv.uv),
                    obj.name + ': nonfinite UV')
        uv = mesh.uv_layers[0].data
        require(mesh.uv_layers[0].name == 'UVMap', obj.name + ': atlas channel name/order')
        require(all(-TOL <= c <= 1 + TOL for item in uv for c in item.uv),
                obj.name + ': atlas UV outside 0..1')
        tile_counts = {}
        face_tiles = {}
        dirt = mesh.uv_layers['DirtUV'].data
        dirt_error = 0.0
        for face in mesh.polygons:
            coords = [uv[i].uv for i in face.loop_indices]
            col = int(math.floor(sum(c.x for c in coords) / len(coords) * 4))
            row_bottom = int(math.floor(sum(c.y for c in coords) / len(coords) * 4))
            require(0 <= col < 4 and 0 <= row_bottom < 4, obj.name + ': invalid atlas cell')
            lo = ((col + .025) / 4, (row_bottom + .025) / 4)
            hi = ((col + .975) / 4, (row_bottom + .975) / 4)
            require(all(lo[axis] - TOL <= coord[axis] <= hi[axis] + TOL
                        for coord in coords for axis in range(2)),
                    obj.name + ': face crosses tile or contracted UV gutter')
            tile = (3 - row_bottom) * 4 + col
            face_tiles[face.index] = tile
            tile_counts[str(tile)] = tile_counts.get(str(tile), 0) + len(face.loop_indices) - 2
            # UV1 must actually encode planar meters/2, not Blender's default square.
            # Near-tied dominant axes can differ slightly after FBX float conversion.
            normal = [abs(n) for n in face.normal]
            candidate_axes = [axis for axis in range(3) if max(normal) - normal[axis] < TOL]
            errors = []
            for axis in candidate_axes:
                plane = [a for a in range(3) if a != axis]
                errors.append(max(abs(dirt[i].uv[j] -
                                      mesh.vertices[mesh.loops[i].vertex_index].co[plane[j]] / 2)
                                  for i in face.loop_indices for j in range(2)))
            error = min(errors)
            require(error < TOL, obj.name + ': DirtUV is not planar meters/2')
            dirt_error = max(dirt_error, error)
        for tri in mesh.loop_triangles:
            a, b, c = [uv[i].uv for i in tri.loops]
            area2 = abs((b.x-a.x)*(c.y-a.y) - (b.y-a.y)*(c.x-a.x))
            require(area2 > 1e-12, obj.name + ': zero-area atlas UV triangle')
        attrs = mesh.color_attributes
        surface = attrs.get('SurfaceParams')
        require(surface is not None, obj.name + ': missing SurfaceParams')
        require(attrs.active_color is not None and attrs.active_color.name == surface.name,
                obj.name + ': SurfaceParams not active')
        require(surface.domain in {'POINT', 'CORNER'}, obj.name + ': color domain')
        colors = [tuple(item.color) for item in surface.data]
        require(colors and all(len(color) == 4 and all(math.isfinite(c) and
                    -1e-6 <= c <= 1.000001 for c in color) for color in colors),
                obj.name + ': invalid four-channel surface parameters')
        require(all(c[1] > 0 for c in colors), obj.name + ': missing roughness values')
        require(all(abs(c[2]) < TOL and abs(c[3] - 1) < TOL for c in colors),
                obj.name + ': SurfaceParams expected blue=0 and alpha=1')
        for face in mesh.polygons:
            tile = face_tiles[face.index]
            expected = ((.68, .43) if tile in (3, 15) else
                        (0, .82) if tile in (8, 9, 10, 13, 14) else (.12, .64))
            for loop in face.loop_indices:
                index = loop if surface.domain == 'CORNER' else mesh.loops[loop].vertex_index
                require(max_error(colors[index][:2], expected) < TOL,
                        obj.name + ': SurfaceParams metallic/roughness disagree with tile')
        result.update({'material_slots': 1, 'uv_channels': 2,
                       'zero_area_atlas_uv_triangles': 0, 'surface_attribute': surface.name,
                       'atlas_uv_range_and_gutter_valid': True,
                       'atlas_tile_triangle_counts': tile_counts,
                       'dirt_uv_planar_max_error': dirt_error,
                       'surface_rgba_matches_tile': True,
                       'surface_domain': surface.domain,
                       'metallic_range': [min(c[0] for c in colors), max(c[0] for c in colors)],
                       'roughness_range': [min(c[1] for c in colors), max(c[1] for c in colors)]})
    return result


def undo_fbx_compensation(obj):
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


def run():
    manifest = json.loads((OUT / 'model_manifest.json').read_text(encoding='utf-8'))
    metadata = {}
    for path in sorted((ROOT / 'Tools/LabSupplyProps/parts').glob('*.py')):
        if path.name.startswith('_'):
            continue
        spec = importlib.util.spec_from_file_location('audit_' + path.stem, path)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        for asset in module.assets():
            require(asset['key'] not in metadata, 'Duplicate authoring asset key')
            metadata[asset['key']] = asset
    require(len(metadata) == len(manifest['assets']) == 12, 'Expected exactly twelve assets')
    require({e['key'] for e in manifest['assets']} == set(metadata), 'Authoring/manifest keys differ')
    require({p.stem for p in (OUT / 'Models').glob('*.fbx')} ==
            {e['name'] for e in manifest['assets']}, 'FBX file set differs')
    bpy.ops.wm.open_mainfile(filepath=str(OUT / 'LabSupplyProps.blend'))
    require(abs(bpy.context.scene.unit_settings.scale_length - 1.0) < TOL,
            'Source scene must use meter scale 1')
    for entry in manifest['assets']:
        name = entry['name']
        source = bpy.data.objects.get(name)
        require(source is not None and source.type == 'MESH', name + ': missing source mesh')
        require(max(abs(source.matrix_world[i][j] - (1 if i == j else 0))
                    for i in range(4) for j in range(4)) < TOL,
                name + ': nonidentity source transform')
        require(entry['material_slots'] == 1, name + ': manifest material count')
        require(max_error(entry['pivot_m'], [0, 0, 0]) < TOL, name + ': pivot metadata')
        source_result = mesh_audit(source, entry)
        bb = source_result['bounds_m']
        dim = [bb[i+3] - bb[i] for i in range(3)]
        require(max_error(dim, metadata[entry['key']]['dimensions']) < TOL,
                name + ': dimensions disagree with authoring contract')
        require(abs(bb[2]) < TOL, name + ': source bottom must be Z=0')
        require(abs(bb[0] + bb[3]) < TOL, name + ': X pivot not centered')
        # The wall enclosure alone uses a rear-bottom pivot instead of XY center.
        is_wall = abs(bb[4]) < TOL and bb[1] < -TOL
        require(is_wall or abs(bb[1] + bb[4]) < TOL, name + ': Y pivot convention')
        expected_collision = [list(lo) + list(hi) for lo, hi in entry['collision_boxes']]
        author_collision = [list(lo) + list(hi) for lo, hi in metadata[entry['key']]['collision']]
        require(len(expected_collision) == len(author_collision), name + ': collision authoring count')
        require(all(max_error(a, b) < TOL for a, b in zip(expected_collision, author_collision)),
                name + ': collision metadata differs from authoring')
        for box in expected_collision:
            require(all(math.isfinite(c) for c in box) and
                    all(box[i+3] > box[i] for i in range(3)), name + ': invalid collision box')
        before = set(bpy.data.objects)
        original_name = source.name
        source.name = original_name + '_audit_source'
        try:
            # Source exporter writes linear color data; avoid the importer's sRGB default.
            bpy.ops.import_scene.fbx(filepath=str(OUT / 'Models' / (name + '.fbx')),
                                     colors_type='LINEAR')
            imported = set(bpy.data.objects) - before
            render = [o for o in imported if o.type == 'MESH' and not o.name.startswith('UCX_')]
            require(len(render) == 1, name + ': expected one FBX render mesh')
            render = render[0]
            undo_fbx_compensation(render)
            imported_result = mesh_audit(render, entry)
            require(imported_result['connected_solids'] == source_result['connected_solids'],
                    name + ': FBX solid count differs')
            require(imported_result['atlas_tile_triangle_counts'] ==
                    source_result['atlas_tile_triangle_counts'], name + ': FBX atlas tiles differ')
            collisions = [o for o in imported if o.type == 'MESH' and o.name.startswith('UCX_')]
            require(len(collisions) == len(expected_collision), name + ': FBX collision count')
            unmatched = list(expected_collision)
            collision_error = 0.0
            for obj in collisions:
                undo_fbx_compensation(obj)
                audit = mesh_audit(obj, None, check_surface=False)
                require(audit['connected_solids'] == 1 and audit['triangles'] == 12,
                        obj.name + ': simple collision must be a closed box')
                distances = [max_error(audit['bounds_m'], box) for box in unmatched]
                index = min(range(len(distances)), key=distances.__getitem__)
                require(distances[index] < TOL, obj.name + ': mirrored UCX bounds differ')
                collision_error = max(collision_error, distances[index])
                unmatched.pop(index)
            report['assets'].append({'name': name, 'key': entry['key'], 'dimensions_m': dim,
                                     'source': source_result, 'fbx_reload': imported_result,
                                     'collision_boxes': len(collisions),
                                     'collision_max_bounds_error_m': collision_error,
                                     'source_unit_transform': True, 'pivot_valid': True})
        finally:
            for obj in set(bpy.data.objects) - before:
                bpy.data.objects.remove(obj, do_unlink=True)
            source.name = original_name
    report['passed'] = True
    report['total_unique_asset_triangles'] = sum(e['source']['triangles'] for e in report['assets'])
    report['total_collision_boxes'] = sum(e['collision_boxes'] for e in report['assets'])


try:
    run()
except Exception as exc:
    report['failures'].append(str(exc))
    report['traceback'] = traceback.format_exc()
    raise
finally:
    (OUT / 'source_validation.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
print('LSP_SOURCE_VALIDATION_PASSED')
