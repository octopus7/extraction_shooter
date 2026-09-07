"""One-off UE 5.7 import. Saves only /Game/Environment/LabSupplyProps.

Remove this generator immediately after committing its validated output.
"""
from pathlib import Path
import json
import struct
import unreal

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'TunaSweeper/SourceArt/Environment/LabSupplyProps'
DEST = '/Game/Environment/LabSupplyProps'
BASE = '/Game/Environment/ModularInteriorPreview'
m = json.loads((OUT / 'model_manifest.json').read_text())
(OUT / 'unreal_import_validation.json').write_text(json.dumps({'mode': 'import', 'passed': False, 'status': 'started'}, indent=2))
assert len(m['assets']) == 12
assets = unreal.AssetToolsHelpers.get_asset_tools()
lib = unreal.MaterialEditingLibrary
editor = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.get_default_object(unreal.StaticMeshEditorSubsystem)


def save(asset):
    assert asset.get_path_name().startswith(DEST + '/'), asset.get_path_name()
    assert unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)


def validate_texture_size(texture, png, expected):
    """Check authored/imported dimensions independently of asynchronous residency."""
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
        # UE5.7 Texture.h: this computes platform size from Source without building.
        built = texture.blueprint_get_built_texture_size()
        import re
        built_text = built.export_text()
        imported = [int(float(re.search(axis + r'=([-+0-9.eE]+)', built_text).group(1))) for axis in ('X', 'Y')]
        method = 'computed_built_size'
        unreal.log('LSP texture source reflection unavailable: ' + str(error))
    unreal.log('LSP_TEXTURE_DIMENSIONS ' + json.dumps({'texture': texture.get_path_name(),
               'source_png': source_png, 'imported': imported, 'method': method,
               'resident_at_check': resident}))
    assert imported == expected, (texture.get_path_name(), method, imported, expected)


task = unreal.AssetImportTask()
task.filename = str(OUT / 'Textures/T_LSP_Atlas.png')
task.destination_path = DEST + '/Textures'
task.destination_name = 'T_LSP_Atlas'
task.automated = True
task.replace_existing = True
task.save = True
assets.import_asset_tasks([task])
atlas = unreal.load_asset(DEST + '/Textures/T_LSP_Atlas')
assert atlas
atlas.set_editor_property('srgb', True)
atlas.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_DEFAULT)
atlas.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_WORLD)
atlas.set_editor_property('never_stream', False)
validate_texture_size(atlas, OUT / 'Textures/T_LSP_Atlas.png', [2048, 2048])
save(atlas)
mask = unreal.load_asset(BASE + '/Textures/T_MI_DirtMask')
assert mask and not mask.get_editor_property('srgb')
validate_texture_size(mask, ROOT / 'TunaSweeper/SourceArt/Environment/ModularInteriorPreview/Textures/T_MI_DirtMask.png', [1024, 1024])

mat = unreal.load_asset(DEST + '/Materials/M_LSP_Surface')
if not mat:
    mat = assets.create_asset('M_LSP_Surface', DEST + '/Materials', unreal.Material, unreal.MaterialFactoryNew())
assert mat
lib.delete_all_material_expressions(mat)
mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_OPAQUE)
mat.set_editor_property('two_sided', False)


def node(kind, **properties):
    expression = lib.create_material_expression(mat, getattr(unreal, 'MaterialExpression' + kind), 0, 0)
    for key, value in properties.items():
        expression.set_editor_property(key, value)
    return expression


def link(source, output, target, input_name):
    names = list(lib.get_material_expression_input_names(target))
    if input_name not in names and len(names) == 1:
        input_name = names[0]
    assert lib.connect_material_expressions(source, output, target, input_name), (input_name, names)


def prop(source, output, target):
    assert lib.connect_material_property(source, output, getattr(unreal.MaterialProperty, 'MP_' + target))


def scalar(name, value, cpd=None):
    result = node('ScalarParameter', parameter_name=name, default_value=value)
    if cpd is not None:
        result.set_editor_property('use_custom_primitive_data', True)
        result.set_editor_property('primitive_data_index', cpd)
    return result


def mul(a, a_output, b, b_output):
    result = node('Multiply')
    link(a, a_output, result, 'A')
    link(b, b_output, result, 'B')
    return result


def lerp(a, a_output, b, b_output, alpha):
    result = node('LinearInterpolate')
    link(a, a_output, result, 'A')
    link(b, b_output, result, 'B')
    link(alpha, '', result, 'Alpha')
    return result


base = node('TextureSample', texture=atlas)
surface = node('VertexColor')  # FBX SurfaceParams: R metallic, G roughness.
uv = node('TextureCoordinate', coordinate_index=1)
scaled = mul(uv, '', scalar('DirtScale', 1.0, 1), '')
offset = node('AppendVector')
link(scalar('DirtOffsetU', 0.0, 2), '', offset, 'A')
link(scalar('DirtOffsetV', 0.0, 3), '', offset, 'B')
coordinates = node('Add')
link(scaled, '', coordinates, 'A')
link(offset, '', coordinates, 'B')
dirt = node('TextureSample', texture=mask, sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE)
link(coordinates, '', dirt, 'UVs')
coverage = mul(dirt, 'R', scalar('DirtStrength', .35, 0), '')
clamped = node('Saturate')
link(coverage, '', clamped, 'Input')
dust = node('VectorParameter', parameter_name='DirtColor', default_value=unreal.LinearColor(.12, .105, .075, 1))
prop(lerp(base, 'RGB', dust, 'RGB', clamped), '', 'BASE_COLOR')
prop(lerp(surface, 'G', scalar('DustRoughness', .94), '', clamped), '', 'ROUGHNESS')
prop(lerp(surface, 'R', scalar('DustMetallic', 0), '', clamped), '', 'METALLIC')
lib.layout_material_expressions(mat)
lib.recompile_material(mat)
save(mat)

for entry in m['assets']:
    assert entry['name'] == 'SM_LSP_' + entry['key']
    assert entry['material_slots'] == 1 and entry['materials'] == ['M_LSP_Surface']
    options = unreal.FbxImportUI()
    for key, value in {
        'import_mesh': True, 'import_as_skeletal': False, 'import_animations': False,
        'import_materials': False, 'import_textures': False,
        'automated_import_should_detect_type': False,
        'mesh_type_to_import': unreal.FBXImportType.FBXIT_STATIC_MESH,
    }.items():
        options.set_editor_property(key, value)
    data = options.static_mesh_import_data
    for key, value in {
        'combine_meshes': True, 'auto_generate_collision': False,
        'one_convex_hull_per_ucx': True, 'generate_lightmap_u_vs': True,
        'convert_scene': True, 'convert_scene_unit': True, 'force_front_x_axis': False,
        'transform_vertex_to_absolute': True, 'build_nanite': False,
        'remove_degenerates': True, 'import_uniform_scale': 1.,
        'vertex_color_import_option': unreal.VertexColorImportOption.REPLACE,
        'normal_import_method': unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS,
    }.items():
        data.set_editor_property(key, value)
    task = unreal.AssetImportTask()
    task.filename = str(OUT / 'Models' / (entry['name'] + '.fbx'))
    task.destination_path = DEST + '/Meshes'
    task.destination_name = entry['name']
    task.automated = True
    task.replace_existing = True
    task.replace_existing_settings = True
    task.save = True
    task.options = options
    task.factory = unreal.FbxFactory()
    assets.import_asset_tasks([task])
    mesh = unreal.load_asset(DEST + '/Meshes/' + entry['name'])
    assert mesh
    assert len(mesh.get_editor_property('static_materials')) == 1
    mesh.set_material(0, mat)
    build = editor.get_lod_build_settings(mesh, 0)
    # Preserve source UV0 atlas and UV1 DirtUV; place generated lightmap in UV2.
    for key, value in {
        'recompute_normals': False, 'recompute_tangents': True,
        'generate_lightmap_u_vs': True, 'src_lightmap_index': 0, 'dst_lightmap_index': 2,
    }.items():
        build.set_editor_property(key, value)
    editor.set_lod_build_settings(mesh, 0, build)
    mesh.set_editor_property('light_map_coordinate_index', 2)
    body = mesh.get_editor_property('body_setup')
    aggregate = body.get_editor_property('agg_geom')
    boxes = []
    for lo, hi in entry['collision_boxes']:
        element = unreal.KBoxElem()
        element.set_editor_property('center', unreal.Vector(*[(a + b) * 50 for a, b in zip(lo, hi)]))
        element.set_editor_property('rotation', unreal.Rotator())
        for index, axis in enumerate(('x', 'y', 'z')):
            element.set_editor_property(axis, (hi[index] - lo[index]) * 100)
        boxes.append(element)
    aggregate.set_editor_property('convex_elems', [])
    aggregate.set_editor_property('box_elems', boxes)
    body.set_editor_property('agg_geom', aggregate)
    body.set_editor_property('collision_trace_flag', unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AND_COMPLEX)
    save(mesh)

# The same read-only audit also runs independently in a fresh editor process.
validator = Path(__file__).with_name('verify_unreal_assets.py')
exec(compile(validator.read_text(), str(validator), 'exec'))
report['mode'] = 'import'
(OUT / 'unreal_import_validation.json').write_text(json.dumps(report, indent=2))
unreal.log('LSP_UE_IMPORT_VALIDATION_PASSED')
