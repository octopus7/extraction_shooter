"""One-off curved matte migration; commit assets then remove this generator."""
import unreal
import math

ROOT = '/Game/UI/Title/'
lib = unreal.MaterialEditingLibrary
assets = unreal.AssetToolsHelpers.get_asset_tools()
texture = unreal.load_asset(ROOT + 'T_TitleMatteLakeExtended')
assert texture
texture.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_SIMPLE_AVERAGE)
texture.set_editor_property('power_of_two_mode', unreal.TexturePowerOfTwoSetting.STRETCH_TO_POWER_OF_TWO)
texture.set_editor_property('address_x', unreal.TextureAddress.TA_CLAMP)
texture.set_editor_property('address_y', unreal.TextureAddress.TA_CLAMP)
unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)

# Import a tessellated concave screen. Its mesh UVs are unused by the material.
from pathlib import Path
source = Path('D:/github/extraction_shooter/TunaSweeper/SourceArt/Title/SM_TitleCurvedScreen.obj')
source.parent.mkdir(parents=True, exist_ok=True)
count = 32
lines = ['o TitleCurvedScreen']
for row in range(count + 1):
    for col in range(count + 1):
        x, y = (col / count - 0.5) * 100, (row / count - 0.5) * 100
        z = 0.0025 * (x*x + y*y)
        # OBJ importer retains Z-up and flips source Y into Unreal handedness.
        lines.append(f'v {x} {-y} {z}')
        lines.append(f'vt {col/count} {row/count}')
lines.append('s 1')
for row in range(count):
    for col in range(count):
        i = row * (count + 1) + col + 1
        ids = [i, i+1, i+count+2, i+count+1]
        lines.append('f ' + ' '.join(f'{n}/{n}' for n in ids))
source.write_text('\n'.join(lines)+'\n')
options = unreal.FbxImportUI()
for key, value in {'import_mesh':True, 'import_as_skeletal':False, 'import_animations':False, 'import_materials':False, 'import_textures':False, 'automated_import_should_detect_type':False, 'mesh_type_to_import':unreal.FBXImportType.FBXIT_STATIC_MESH}.items():
    options.set_editor_property(key, value)
data = options.static_mesh_import_data
data.set_editor_property('combine_meshes', True)
data.set_editor_property('auto_generate_collision', False)
data.set_editor_property('generate_lightmap_u_vs', False)
data.set_editor_property('normal_import_method', unreal.FBXNormalImportMethod.FBXNIM_COMPUTE_NORMALS)
task = unreal.AssetImportTask()
for key,value in {'filename':str(source),'destination_path':ROOT.rstrip('/'),'destination_name':'SM_TitleCurvedScreen','automated':True,'replace_existing':True,'replace_existing_settings':True,'save':True}.items():
    task.set_editor_property(key,value)
task.options = options
task.factory = unreal.FbxFactory()
assets.import_asset_tasks([task])
mesh = unreal.load_asset(ROOT+'SM_TitleCurvedScreen')
assert mesh
bounds = mesh.get_bounding_box()
print('CURVED_SCREEN_BOUNDS',bounds)
assert mesh.get_num_triangles(0)==2048
unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False)
print('CURVED_SCREEN_BUILT', mesh.get_num_triangles(0))

mat = unreal.load_asset(ROOT + 'M_TitleMatteLake')
lib.delete_all_material_expressions(mat)
mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
mat.set_editor_property('two_sided', True)
# Translucent unlit remains depth-tested behind the character and is not an
# opaque emissive GBuffer surface feeding Lumen screen-space diffuse traces.
mat.set_editor_property('use_translucency_vertex_fog', False)
mat.set_editor_property('disable_depth_test', False)
tex = lib.create_material_expression(mat, unreal.MaterialExpressionTextureObjectParameter, -800, -200)
tex.set_editor_property('parameter_name', 'MatteTexture')
tex.set_editor_property('texture', texture)
screen = lib.create_material_expression(mat, unreal.MaterialExpressionScreenPosition, -800, 0)
size = lib.create_material_expression(mat, unreal.MaterialExpressionViewSize, -800, 200)
radius = lib.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -800, 400)
radius.set_editor_property('parameter_name', 'EdgeBlurPixels')
radius.set_editor_property('default_value', 14.0)
custom = lib.create_material_expression(mat, unreal.MaterialExpressionCustom, -450, 0)
custom.set_editor_property('description', 'Camera projected matte with aspect cover and peripheral blur')
custom.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT3)
custom_inputs = []
for name in ['Matte', 'UV', 'ViewSize', 'BlurPixels']:
    entry = unreal.CustomInput()
    entry.set_editor_property('input_name', name)
    custom_inputs.append(entry)
custom.set_editor_property('inputs', custom_inputs)
custom.set_editor_property('code', '''
float aspect = ViewSize.x / max(ViewSize.y, 1.0);
// The square outpainting reveals more sky and ground on taller viewports.
float2 crop = aspect >= 1.0 ? float2(1.0, 1.0/aspect) : float2(aspect, 1.0);
float2 projected = (UV - 0.5) * crop + 0.5;
float edge = smoothstep(0.5, 1.0, max(abs(UV.x-0.5), abs(UV.y-0.5))*2.0);
// Widen the filter footprint continuously at the periphery. Mip filtering
// avoids the doubled edges of a few widely spaced blur taps.
float footprint = max(1.0, BlurPixels * edge);
return Texture2DSampleGrad(Matte, MatteSampler, projected,
    ddx(projected) * footprint, ddy(projected) * footprint).rgb;
''')
for src, output, name in [(tex,'','Matte'), (screen,'ViewportUV','UV'), (size,'','ViewSize'), (radius,'','BlurPixels')]:
    assert lib.connect_material_expressions(src, output, custom, name)
exposure = lib.create_material_expression(mat, unreal.MaterialExpressionEyeAdaptation, -400, 400)
divide = lib.create_material_expression(mat, unreal.MaterialExpressionDivide, -150, 0)
lib.connect_material_expressions(custom, '', divide, 'A')
lib.connect_material_expressions(exposure, '', divide, 'B')
lib.connect_material_property(divide, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
opacity = lib.create_material_expression(mat, unreal.MaterialExpressionConstant, -150, 200)
opacity.r = 1.0
lib.connect_material_property(opacity, '', unreal.MaterialProperty.MP_OPACITY)
lib.recompile_material(mat)
unreal.EditorAssetLibrary.save_loaded_asset(mat, only_if_is_dirty=False)
mesh.set_material(0, mat)
unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False)

bp = unreal.load_asset(ROOT + 'BP_TitleStudio')
sub = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
data_lib = unreal.SubobjectDataBlueprintFunctionLibrary
for handle in sub.k2_gather_subobject_data_for_blueprint(bp):
    obj = data_lib.get_object_for_blueprint(data_lib.get_data(handle), bp)
    if isinstance(obj, unreal.StaticMeshComponent) and obj.get_name() == 'MatteBackdrop':
        obj.set_static_mesh(mesh)
        obj.set_material(0, mat)
        obj.set_editor_property('visible_in_ray_tracing', False)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
unreal.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False)
character_bp = unreal.load_asset(ROOT + 'BP_TitlePresentationActor')
unreal.get_default_object(character_bp.generated_class()).set_editor_property('title_exposure_compensation', 3.0)
unreal.BlueprintEditorLibrary.compile_blueprint(character_bp)
unreal.EditorAssetLibrary.save_loaded_asset(character_bp, only_if_is_dirty=False)
world = unreal.EditorLoadingAndSavingUtils.load_map('/Game/Maps/IntroMap')
for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if isinstance(actor, unreal.TunaSweeperTitlePresentationActor):
        actor.set_editor_property('title_exposure_compensation', 3.0)
    if isinstance(actor, unreal.TunaSweeperTitleStudioActor):
        for comp in actor.get_components_by_class(unreal.StaticMeshComponent):
            if comp.get_name() == 'MatteBackdrop':
                comp.set_static_mesh(mesh)
                comp.set_material(0, mat)
                comp.set_editor_property('visible_in_ray_tracing', False)
assert unreal.EditorLoadingAndSavingUtils.save_map(world, '/Game/Maps/IntroMap')
print('PROJECTED_TITLE_MIGRATION_OK')
unreal.SystemLibrary.quit_editor()
