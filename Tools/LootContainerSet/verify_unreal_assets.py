"""Read-only UE 5.7 persisted assets audit; no import, save, or editor quit.

Can be exec'd by the one-off importer. GeometryScript copies are transient.
The report variable and unreal_reload_validation.json contain the same result.
"""
from pathlib import Path
import hashlib
import json
import math
import re
import struct
import traceback
import unreal

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'TunaSweeper/SourceArt/Environment/LootContainerSet'
DEST = '/Game/Interaction/LootContainerSet'
SHARED = '/Game/Environment/LabSupplyProps'
BASE = '/Game/Environment/ModularInteriorPreview'
MATERIAL = SHARED + '/Materials/M_LSP_Surface'
EXPECTED_NAMES = {f'SM_LC_{kind}_{part}' for kind in ('Wood', 'Metal', 'Supply')
                  for part in ('Body', 'Lid')}
editor = (unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
          or unreal.get_default_object(unreal.StaticMeshEditorSubsystem))
lib = unreal.MaterialEditingLibrary
queries = unreal.GeometryScript_MeshQueries
report = {'engine': unreal.SystemLibrary.get_engine_version(), 'mode': 'reload',
          'passed': False, 'assertions': 0, 'assets': [], 'failures': []}


def require(condition, message):
    report['assertions'] += 1
    if not condition:
        raise AssertionError(message)


def values(result):
    return list(result) if isinstance(result, (list, tuple)) else [result]


def typed_outputs(result, kind, count, label):
    output = values(result)
    flags = [value for value in output if isinstance(value, bool)]
    require(flags and all(flags), label + ': query reports invalid triangle attribute')
    chosen = [value for value in output if isinstance(value, kind)]
    require(len(chosen) == count, label + ': unexpected GeometryScript outputs')
    return chosen


def xyz(vector):
    return [vector.x, vector.y, vector.z]


def finite(coords):
    return all(math.isfinite(c) for c in coords)


def package_file(path):
    require(path.startswith('/Game/'), 'Unexpected package root: ' + path)
    return ROOT / 'TunaSweeper/Content' / (path[len('/Game/'):] + '.uasset')


def hashes(paths):
    return {path: hashlib.sha256(package_file(path).read_bytes()).hexdigest()
            for path in sorted(paths)}


def validate_texture_size(texture, png, expected):
    header = png.read_bytes()[:24]
    require(header[:8] == b'\x89PNG\r\n\x1a\n' and header[12:16] == b'IHDR', str(png))
    source_png = list(struct.unpack('>II', header[16:24]))
    require(source_png == expected, 'Unexpected PNG size: ' + str(png))
    resident = [texture.blueprint_get_size_x(), texture.blueprint_get_size_y()]
    try:
        source = texture.get_editor_property('source')
        imported = [source.get_editor_property('size_x'), source.get_editor_property('size_y')]
        method = 'persisted_texture_source'
    except Exception:
        # UE 5.7 Texture.h: computed from Source, independent of async mip residency.
        built_text = texture.blueprint_get_built_texture_size().export_text()
        imported = [int(float(re.search(axis + r'=([-+0-9.eE]+)', built_text).group(1)))
                    for axis in ('X', 'Y')]
        method = 'computed_built_size'
    require(imported == expected, 'Unexpected UE texture source size: ' + texture.get_path_name())
    return {'texture': texture.get_path_name(), 'source_png': source_png,
            'imported': imported, 'method': method, 'resident_at_check': resident}


def audit_dynamic(mesh, entry):
    options = unreal.GeometryScriptCopyMeshFromAssetOptions()
    options.set_editor_property('apply_build_settings', False)
    lod_request = unreal.GeometryScriptMeshReadLOD()
    lod_request.set_editor_property('lod_type', unreal.GeometryScriptLODType.SOURCE_MODEL)
    lod_request.set_editor_property('lod_index', 0)
    copied = unreal.GeometryScript_AssetUtils.copy_mesh_from_static_mesh(
        mesh, unreal.DynamicMesh(), options, lod_request, None)
    dynamic = next((item for item in values(copied) if isinstance(item, unreal.DynamicMesh)), None)
    require(dynamic is not None, entry['name'] + ': failed to copy persisted SourceModel LOD0')
    count = dynamic.get_triangle_count()
    require(count == entry['triangles'], entry['name'] + ': actual triangle count differs')
    channels = queries.get_num_uv_sets(dynamic)
    require(channels >= 2, entry['name'] + ': missing source UV0/UV1')
    require(queries.get_has_triangle_normals(dynamic), entry['name'] + ': missing normals overlay')
    require(queries.get_has_vertex_colors(dynamic), entry['name'] + ': missing vertex colors overlay')
    observed = 0
    ranges = [[float('inf'), float('-inf')] for _ in range(4)]
    min_area = float('inf')
    uv_minmax = [[[float('inf'), float('-inf')] for _ in range(2)] for _ in range(2)]
    dirt_errors = [0.0, 0.0]  # Source V, or FBX->UE flipped V.
    origin = entry.get('assembly_origin_m')
    geometry_bounds = [[float('inf'), float('-inf')] for _ in range(3)]
    for tid in range(queries.get_num_triangle_i_ds(dynamic)):
        if not queries.is_valid_triangle_id(dynamic, tid):
            continue
        observed += 1
        positions = typed_outputs(queries.get_triangle_positions(dynamic, tid), unreal.Vector, 3,
                                  entry['name'] + ': positions')
        p = [xyz(position) for position in positions]
        require(all(finite(position) for position in p), entry['name'] + ': nonfinite position')
        for point in p:
            for axis in range(3):
                geometry_bounds[axis][0] = min(geometry_bounds[axis][0], point[axis])
                geometry_bounds[axis][1] = max(geometry_bounds[axis][1], point[axis])
        a = [p[1][i] - p[0][i] for i in range(3)]
        b = [p[2][i] - p[0][i] for i in range(3)]
        cross = [a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]]
        twice_area = math.sqrt(sum(c*c for c in cross))
        require(twice_area > 1e-6, entry['name'] + ': degenerate UE triangle')
        min_area = min(min_area, twice_area / 2)
        normals = typed_outputs(queries.get_triangle_normals(dynamic, tid), unreal.Vector, 3,
                                entry['name'] + ': normals')
        for normal in normals:
            n = xyz(normal)
            require(finite(n) and abs(sum(c*c for c in n) - 1) < .01,
                    entry['name'] + ': invalid split normal')
            # UE and Blender differ in winding conventions; both signs are
            # permitted here, but normals must align with the geometric plane.
            require(abs(sum(n[i]*cross[i] for i in range(3))/twice_area) > .999,
                    entry['name'] + ': split normal is not flat-face aligned')
        for channel in (0, 1):
            coords = typed_outputs(queries.get_triangle_u_vs(dynamic, channel, tid),
                                   unreal.Vector2D, 3, entry['name'] + ': UV' + str(channel))
            uv = [[coord.x, coord.y] for coord in coords]
            require(all(finite(coord) for coord in uv), entry['name'] + ': nonfinite UV')
            if channel == 0:
                require(all(-.001 <= c <= 1.001 for coord in uv for c in coord),
                        entry['name'] + ': UV0 outside atlas')
            area2 = abs((uv[1][0]-uv[0][0])*(uv[2][1]-uv[0][1])
                        -(uv[1][1]-uv[0][1])*(uv[2][0]-uv[0][0]))
            require(area2 > 1e-12, entry['name'] + ': collapsed UV triangle')
            for coord in uv:
                for axis in range(2):
                    uv_minmax[channel][axis][0] = min(uv_minmax[channel][axis][0], coord[axis])
                    uv_minmax[channel][axis][1] = max(uv_minmax[channel][axis][1], coord[axis])
            if channel == 1 and origin is not None:
                dominant = max(range(3), key=lambda axis: abs(cross[axis]))
                for point, actual in zip(p, uv):
                    assembled = [point[i]/100 + origin[i] for i in range(3)]
                    expected = ([assembled[0]/2, assembled[1]/2] if dominant == 2 else
                                [(assembled[0]+.731*assembled[1])/2, assembled[2]/2])
                    for flipped in (0, 1):
                        ev = 1 - expected[1] if flipped else expected[1]
                        dirt_errors[flipped] = max(dirt_errors[flipped],
                                                  abs(actual[0]-expected[0]), abs(actual[1]-ev))
        colors = typed_outputs(queries.get_triangle_vertex_colors(dynamic, tid),
                               unreal.LinearColor, 3, entry['name'] + ': vertex color')
        for color in colors:
            rgba = [color.r, color.g, color.b, color.a]
            require(finite(rgba) and all(-.001 <= c <= 1.001 for c in rgba),
                    entry['name'] + ': vertex RGBA range')
            require(abs(rgba[2]) < .005 and abs(rgba[3]-1) < .005,
                    entry['name'] + ': SurfaceParams B/A not preserved')
            require(any(max(abs(rgba[0]-metal), abs(rgba[1]-rough)) < .012
                        for metal, rough in [(0,.82),(.68,.43),(.12,.64)]),
                    entry['name'] + ': SurfaceParams R/G not preserved: ' + str(rgba))
            for channel, value in enumerate(rgba):
                ranges[channel][0] = min(ranges[channel][0], value)
                ranges[channel][1] = max(ranges[channel][1], value)
    require(observed == count, entry['name'] + ': GeometryScript traversal missed triangles')
    actual_bounds = [x[0] for x in geometry_bounds] + [x[1] for x in geometry_bounds]
    geometry_error = max(abs(a - b*100) for a, b in zip(actual_bounds, entry['bounds_m']))
    require(geometry_error < .01, entry['name'] + ': SourceModel geometry bounds differ')
    result = {'triangles': count, 'queried_triangles': observed, 'source_uv_channels': channels,
              'geometry_bounds_error_cm': geometry_error, 'degenerate_triangles': 0,
              'minimum_triangle_area_cm2': min_area, 'actual_split_normals_valid': True,
              'actual_uv_bounds': uv_minmax, 'actual_surface_rgba_ranges': ranges,
              'actual_vertex_colors_valid': True}
    if origin is not None:
        require(min(dirt_errors) < .002, entry['name'] + ': assembly-space DirtUV mismatch ' + str(dirt_errors))
        result['assembly_dirt_uv_max_error'] = min(dirt_errors)
        result['assembly_dirt_uv_v_flipped'] = dirt_errors[1] < dirt_errors[0]
    return result


def run():
    m = json.loads((OUT / 'model_manifest.json').read_text(encoding='utf-8'))
    require(len(m['assets']) == 6 and {e['name'] for e in m['assets']} == EXPECTED_NAMES,
            'Expected six core loot container assets')
    require(report['engine'].startswith('5.7.'), 'Expected UE 5.7')
    package_paths = [DEST + '/Meshes/' + e['name'] for e in m['assets']]
    package_paths += [MATERIAL, SHARED + '/Textures/T_LSP_Atlas', BASE + '/Textures/T_MI_DirtMask']
    before = hashes(package_paths)
    for entry in m['assets']:
        mesh = unreal.load_asset(DEST + '/Meshes/' + entry['name'])
        require(mesh is not None, entry['name'] + ': missing mesh asset')
        bound = mesh.get_bounds()
        center, radius = bound.origin, bound.box_extent
        actual = [center.x-radius.x, center.y-radius.y, center.z-radius.z,
                  center.x+radius.x, center.y+radius.y, center.z+radius.z]
        error = max(abs(a-b*100) for a,b in zip(actual, entry['bounds_m']))
        require(error < .01, entry['name'] + ': persisted mesh bounds differ')
        body = mesh.get_editor_property('body_setup')
        aggregate = body.get_editor_property('agg_geom')
        require(body.get_editor_property('collision_trace_flag') ==
                unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AND_COMPLEX, entry['name'] + ': collision mode')
        require(not aggregate.get_editor_property('convex_elems'), entry['name'] + ': unexpected convex hull')
        boxes = aggregate.get_editor_property('box_elems')
        require(len(boxes) == len(entry['collision_boxes']), entry['name'] + ': box collision count')
        count = editor.get_simple_collision_count(mesh) + editor.get_convex_collision_count(mesh)
        require(count == len(boxes), entry['name'] + ': additional collision shapes')
        box_error = 0.0
        for box, (lo, hi) in zip(boxes, entry['collision_boxes']):
            center = box.get_editor_property('center')
            actual_box = xyz(center) + [box.get_editor_property(k) for k in ('x','y','z')]
            expected_box = [(a+b)*50 for a,b in zip(lo,hi)] + [(b-a)*100 for a,b in zip(lo,hi)]
            current_error = max(abs(a-b) for a,b in zip(actual_box, expected_box))
            require(current_error < .01, entry['name'] + ': collision-box bounds')
            box_error = max(box_error, current_error)
            rotation = box.get_editor_property('rotation')
            require(max(abs(rotation.pitch), abs(rotation.yaw), abs(rotation.roll)) < .01,
                    entry['name'] + ': rotated collision box')
        uv = editor.get_num_uv_channels(mesh, 0)
        require(uv >= 2, entry['name'] + ': source UV count')
        build = editor.get_lod_build_settings(mesh, 0)
        require(build.get_editor_property('generate_lightmap_u_vs') and
                build.get_editor_property('src_lightmap_index') == 0 and
                build.get_editor_property('dst_lightmap_index') == 2,
                entry['name'] + ': lightmap generation must preserve UV0/UV1')
        require(not build.get_editor_property('recompute_normals'), entry['name'] + ': normals recomputed')
        require(build.get_editor_property('recompute_tangents'), entry['name'] + ': tangents not generated')
        require(mesh.get_editor_property('light_map_coordinate_index') == 2, entry['name'] + ': lightmap UV index')
        require(editor.get_lod_count(mesh) == 1, entry['name'] + ': unexpected LOD count')
        slots = list(mesh.get_editor_property('static_materials'))
        require(len(slots) == 1, entry['name'] + ': material slots')
        require(slots[0].get_editor_property('material_interface').get_path_name() ==
                MATERIAL + '.M_LSP_Surface', entry['name'] + ': shared material assignment')
        require(not mesh.get_editor_property('nanite_settings').get_editor_property('enabled'),
                entry['name'] + ': Nanite unexpectedly enabled')
        import_data = mesh.get_editor_property('asset_import_data')
        require(import_data.get_editor_property('vertex_color_import_option') ==
                unreal.VertexColorImportOption.REPLACE, entry['name'] + ': vertex colors not imported')
        require(import_data.get_editor_property('normal_import_method') ==
                unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS, entry['name'] + ': normal import mode')
        require(abs(import_data.get_editor_property('import_uniform_scale')-1) < 1e-6,
                entry['name'] + ': FBX import scale')
        dynamic = audit_dynamic(mesh, entry)
        report['assets'].append({'name': entry['name'], 'triangles': dynamic['triangles'],
                                 'material_slots': len(slots), 'bounds_error_cm': error,
                                 'collision_boxes': len(boxes), 'collision_max_bounds_error_cm': box_error,
                                 'collision_rotation_verified': True, 'source_uv_channels': uv,
                                 'generated_lightmap_uv_index': 2, 'vertex_color_import': 'REPLACE',
                                 'normal_import': 'IMPORT_NORMALS', 'nanite': False,
                                 'has_navigation_data': bool(mesh.get_editor_property('has_navigation_data')),
                                 'geometry_script': dynamic})
    atlas = unreal.load_asset(SHARED + '/Textures/T_LSP_Atlas')
    mask = unreal.load_asset(BASE + '/Textures/T_MI_DirtMask')
    require(atlas is not None and mask is not None, 'Missing shared textures')
    require(atlas.get_editor_property('srgb') and not mask.get_editor_property('srgb'), 'Texture color spaces')
    require(mask.get_editor_property('compression_settings') == unreal.TextureCompressionSettings.TC_GRAYSCALE,
            'Shared dirt mask compression')
    report['texture_dimensions'] = [
        validate_texture_size(atlas, OUT/'Textures/T_LSP_Atlas.png', [2048,2048]),
        validate_texture_size(mask, OUT/'Textures/T_MI_DirtMask.png', [1024,1024])]
    mat = unreal.load_asset(MATERIAL)
    require(mat is not None and mat.get_editor_property('blend_mode') == unreal.BlendMode.BLEND_OPAQUE,
            'Shared material must be Opaque')
    require(not mat.get_editor_property('two_sided'), 'Shared material must be single-sided')
    nodes = set()
    pending = [lib.get_material_property_input_node(mat, getattr(unreal.MaterialProperty, 'MP_'+name))
               for name in ('BASE_COLOR','ROUGHNESS','METALLIC')]
    require(all(pending), 'Missing shared material output connection')
    while pending:
        expression = pending.pop()
        if not expression or expression in nodes:
            continue
        nodes.add(expression)
        pending.extend(lib.get_inputs_for_material_expression(mat, expression))
    samples = [n for n in nodes if isinstance(n, unreal.MaterialExpressionTextureSample)]
    require(len(samples) == 2, 'Expected two reachable texture samples')
    used = sorted(n.get_editor_property('texture').get_path_name() for n in samples)
    require(used == sorted([atlas.get_path_name(),mask.get_path_name()]), 'Unexpected material texture dependency')
    vertex_colors = [n for n in nodes if isinstance(n, unreal.MaterialExpressionVertexColor)]
    require(len(vertex_colors) == 1, 'Missing shared vertex-color surface parameters')
    parameters = {str(n.get_editor_property('parameter_name')): n for n in nodes
                  if isinstance(n, unreal.MaterialExpressionScalarParameter)}
    for name,index,value in [('DirtStrength',0,.35),('DirtScale',1,1.),('DirtOffsetU',2,0.),('DirtOffsetV',3,0.)]:
        parameter = parameters[name]
        require(parameter.get_editor_property('use_custom_primitive_data') and
                parameter.get_editor_property('primitive_data_index') == index and
                abs(parameter.get_editor_property('default_value')-value) < 1e-6,
                'Invalid CPD parameter: '+name)
    report['materials'] = {'path': mat.get_path_name(), 'opaque': True, 'two_sided': False,
                           'reachable_texture_sample_nodes': len(samples), 'used_textures': used,
                           'reachable_vertex_color_nodes': len(vertex_colors),
                           'surface_channels': {'R':'metallic','G':'roughness'},
                           'custom_primitive_data': {'0':'DirtStrength=.35','1':'DirtScale=1',
                                                     '2':'DirtOffsetU=0','3':'DirtOffsetV=0'},
                           'atlas_size':[2048,2048], 'shared_dirt_mask_size':[1024,1024]}
    after = hashes(package_paths)
    require(before == after, 'Read-only audit changed a persisted package')
    report['package_sha256'] = after
    report['persisted_packages_unchanged_during_audit'] = True
    report['total_unique_mesh_triangles'] = sum(e['triangles'] for e in report['assets'])
    report['total_collision_boxes'] = sum(e['collision_boxes'] for e in report['assets'])
    report['passed'] = True


try:
    run()
except Exception as error:
    report['failures'].append(str(error))
    report['traceback'] = traceback.format_exc()
    raise
finally:
    (OUT/'unreal_reload_validation.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
unreal.log('LC_UE_ASSETS_VALIDATION_PASSED')
