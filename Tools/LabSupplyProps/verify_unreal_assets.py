"""Read-only UE 5.7 persisted asset audit: never imports, modifies or saves assets."""
from pathlib import Path
import json
import struct
import unreal

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'TunaSweeper/SourceArt/Environment/LabSupplyProps'
DEST = '/Game/Environment/LabSupplyProps'
BASE = '/Game/Environment/ModularInteriorPreview'
m = json.loads((OUT / 'model_manifest.json').read_text())
editor = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.get_default_object(unreal.StaticMeshEditorSubsystem)
lib = unreal.MaterialEditingLibrary
report = {'engine': unreal.SystemLibrary.get_engine_version(), 'mode': 'reload', 'passed': False, 'assets': []}
(OUT / 'unreal_reload_validation.json').write_text(json.dumps(report, indent=2))
assert len(m['assets']) == 12
assert len({e['name'] for e in m['assets']}) == 12


def validate_texture_size(texture, png, expected):
    """PNG and persisted UE source dimensions; runtime residency is diagnostic."""
    header = png.read_bytes()[:24]
    assert header[:8] == b'\x89PNG\r\n\x1a\n' and header[12:16] == b'IHDR', str(png)
    source_png = list(struct.unpack('>II', header[16:24]))
    assert source_png == expected, (str(png), source_png, expected)
    resident = [texture.blueprint_get_size_x(), texture.blueprint_get_size_y()]
    try:
        source = texture.get_editor_property('source')
        imported = [source.get_editor_property('size_x'), source.get_editor_property('size_y')]
        method = 'persisted_texture_source'
    except Exception as error:
        # This UE5.7 function reads Source, not asynchronous platform mip data.
        built = texture.blueprint_get_built_texture_size()
        import re
        built_text = built.export_text()
        imported = [int(float(re.search(axis + r'=([-+0-9.eE]+)', built_text).group(1))) for axis in ('X', 'Y')]
        method = 'computed_built_size'
        unreal.log('LSP texture source reflection unavailable: ' + str(error))
    result = {'texture': texture.get_path_name(), 'source_png': source_png,
              'imported': imported, 'method': method, 'resident_at_check': resident}
    unreal.log('LSP_TEXTURE_DIMENSIONS ' + json.dumps(result))
    assert imported == expected, result
    return result


for entry in m['assets']:
    assert entry['name'] == 'SM_LSP_' + entry['key']
    mesh = unreal.load_asset(DEST + '/Meshes/' + entry['name'])
    assert mesh, entry['name']
    bounds = mesh.get_bounds()
    center, radius = bounds.origin, bounds.box_extent
    actual = [center.x - radius.x, center.y - radius.y, center.z - radius.z,
              center.x + radius.x, center.y + radius.y, center.z + radius.z]
    error = max(abs(a - b * 100) for a, b in zip(actual, entry['bounds_m']))
    assert error < .1, (entry['name'], actual, entry['bounds_m'])
    body = mesh.get_editor_property('body_setup')
    aggregate = body.get_editor_property('agg_geom')
    assert body.get_editor_property('collision_trace_flag') == unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AND_COMPLEX
    assert not aggregate.get_editor_property('convex_elems')
    boxes = aggregate.get_editor_property('box_elems')
    assert len(boxes) == len(entry['collision_boxes'])
    count = editor.get_simple_collision_count(mesh) + editor.get_convex_collision_count(mesh)
    assert count == len(boxes), (entry['name'], count, len(boxes))
    for box, (lo, hi) in zip(boxes, entry['collision_boxes']):
        center = box.get_editor_property('center')
        actual_box = [center.x, center.y, center.z] + [box.get_editor_property(k) for k in ('x', 'y', 'z')]
        expected_box = [(a + b) * 50 for a, b in zip(lo, hi)] + [(b - a) * 100 for a, b in zip(lo, hi)]
        assert max(abs(a - b) for a, b in zip(actual_box, expected_box)) < .01
        rotation = box.get_editor_property('rotation')
        assert max(abs(rotation.pitch), abs(rotation.yaw), abs(rotation.roll)) < .01
    uv = editor.get_num_uv_channels(mesh, 0)
    assert uv >= 2, (entry['name'], uv)
    build = editor.get_lod_build_settings(mesh, 0)
    assert build.get_editor_property('generate_lightmap_u_vs')
    assert build.get_editor_property('src_lightmap_index') == 0
    assert build.get_editor_property('dst_lightmap_index') == 2
    assert not build.get_editor_property('recompute_normals')
    assert mesh.get_editor_property('light_map_coordinate_index') == 2
    slots = list(mesh.get_editor_property('static_materials'))
    assert len(slots) == entry['material_slots'] == 1
    assert slots[0].get_editor_property('material_interface').get_path_name() == DEST + '/Materials/M_LSP_Surface.M_LSP_Surface'
    assert not mesh.get_editor_property('nanite_settings').get_editor_property('enabled')
    import_data = mesh.get_editor_property('asset_import_data')
    assert import_data.get_editor_property('vertex_color_import_option') == unreal.VertexColorImportOption.REPLACE
    lod = unreal.GeometryScript_AssetUtils.copy_mesh_from_static_mesh(
        mesh, unreal.DynamicMesh(), unreal.GeometryScriptCopyMeshFromAssetOptions(), unreal.GeometryScriptMeshReadLOD(), None)
    dynamic = next(x for x in lod if isinstance(x, unreal.DynamicMesh)) if isinstance(lod, tuple) else lod
    triangles = dynamic.get_triangle_count()
    assert triangles == entry['triangles'], (entry['name'], triangles, entry['triangles'])
    report['assets'].append({
        'name': entry['name'], 'triangles': triangles, 'material_slots': len(slots),
        'bounds_error_cm': error, 'collision_boxes': len(boxes),
        'collision_shape_bounds_verified': True, 'collision_rotation_verified': True,
        'source_uv_channels': uv, 'generated_lightmap_uv_index': 2,
        'vertex_color_import': 'REPLACE', 'nanite': False,
    })

atlas = unreal.load_asset(DEST + '/Textures/T_LSP_Atlas')
mask = unreal.load_asset(BASE + '/Textures/T_MI_DirtMask')
assert atlas and mask
assert atlas.get_editor_property('srgb')
assert not mask.get_editor_property('srgb')
assert mask.get_editor_property('compression_settings') == unreal.TextureCompressionSettings.TC_GRAYSCALE
report['texture_dimensions'] = [
    validate_texture_size(atlas, OUT / 'Textures/T_LSP_Atlas.png', [2048, 2048]),
    validate_texture_size(mask, ROOT / 'TunaSweeper/SourceArt/Environment/ModularInteriorPreview/Textures/T_MI_DirtMask.png', [1024, 1024]),
]

mat = unreal.load_asset(DEST + '/Materials/M_LSP_Surface')
assert mat and mat.get_editor_property('blend_mode') == unreal.BlendMode.BLEND_OPAQUE
assert not mat.get_editor_property('two_sided')
nodes = set()
pending = [lib.get_material_property_input_node(mat, getattr(unreal.MaterialProperty, 'MP_' + name))
           for name in ('BASE_COLOR', 'ROUGHNESS', 'METALLIC')]
assert all(pending)
while pending:
    expression = pending.pop()
    if not expression or expression in nodes:
        continue
    nodes.add(expression)
    pending.extend(lib.get_inputs_for_material_expression(mat, expression))
samples = [n for n in nodes if isinstance(n, unreal.MaterialExpressionTextureSample)]
assert len(samples) == 2, len(samples)
used = sorted(n.get_editor_property('texture').get_path_name() for n in samples)
assert used == sorted([atlas.get_path_name(), mask.get_path_name()])
vertex_colors = [n for n in nodes if isinstance(n, unreal.MaterialExpressionVertexColor)]
assert len(vertex_colors) == 1
parameters = {str(n.get_editor_property('parameter_name')): n for n in nodes
              if isinstance(n, unreal.MaterialExpressionScalarParameter)}
for name, index, value in [('DirtStrength', 0, .35), ('DirtScale', 1, 1.),
                           ('DirtOffsetU', 2, 0.), ('DirtOffsetV', 3, 0.)]:
    parameter = parameters[name]
    assert parameter.get_editor_property('use_custom_primitive_data')
    assert parameter.get_editor_property('primitive_data_index') == index
    assert abs(parameter.get_editor_property('default_value') - value) < 1e-6
report['materials'] = {
    'name': 'M_LSP_Surface', 'opaque': True, 'two_sided': False,
    'reachable_texture_sample_nodes': len(samples), 'used_textures': used,
    'reachable_vertex_color_nodes': len(vertex_colors),
    'surface_channels': {'R': 'metallic', 'G': 'roughness'},
    'custom_primitive_data': {'0': 'DirtStrength=.35', '1': 'DirtScale=1', '2': 'DirtOffsetU=0', '3': 'DirtOffsetV=0'},
    'atlas_size': [2048, 2048], 'shared_dirt_mask_size': [1024, 1024],
}
report['total_unique_mesh_triangles'] = sum(e['triangles'] for e in report['assets'])
report['passed'] = True
(OUT / 'unreal_reload_validation.json').write_text(json.dumps(report, indent=2))
unreal.log('LSP_UE_ASSETS_VALIDATION_PASSED')
