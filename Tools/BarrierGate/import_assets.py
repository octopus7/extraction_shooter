"""One-off editor asset creator; no map edits. Removed after asset commit."""
from pathlib import Path
import unreal

ROOT=Path(__file__).resolve().parents[2]
SOURCE=ROOT/'TunaSweeper/SourceArt/Environment/BarrierGate'
DEST='/Game/Environment/BarrierGate'
assets=unreal.AssetToolsHelpers.get_asset_tools()

def save(obj):
    assert obj.get_path_name().startswith(DEST+'/')
    assert unreal.EditorAssetLibrary.save_loaded_asset(obj,only_if_is_dirty=False)

def import_file(filename,folder,name,options=None):
    task=unreal.AssetImportTask()
    task.filename=str(SOURCE/filename);task.destination_path=DEST+'/'+folder;task.destination_name=name
    task.automated=True;task.replace_existing=True;task.save=True
    if options:
        task.options=options;task.factory=unreal.FbxFactory()
    assets.import_asset_tasks([task])
    obj=unreal.load_asset(DEST+'/'+folder+'/'+name)
    assert obj,filename
    return obj

def const(mat,value,prop):
    node=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionConstant)
    node.set_editor_property('r',value)
    unreal.MaterialEditingLibrary.connect_material_property(node,'',prop)

def create_material(name):
    mat=unreal.load_asset(DEST+'/Materials/'+name) if unreal.EditorAssetLibrary.does_asset_exist(DEST+'/Materials/'+name) else None
    mat=mat or assets.create_asset(name,DEST+'/Materials',unreal.Material,unreal.MaterialFactoryNew())
    unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)
    return mat

materials={}
for suffix,metal,rough in [('Housing',.35,.48),('Arm',.2,.36)]:
    texname='T_BarrierHousing_BaseColor' if suffix=='Housing' else 'T_BarrierArm_Stripes'
    tex=import_file(texname+'.png','Textures',texname)
    tex.set_editor_property('srgb',True)
    tex.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_WORLD)
    save(tex)
    mat=create_material('M_Barrier'+suffix)
    node=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionTextureSample,-300,0)
    node.set_editor_property('texture',tex)
    unreal.MaterialEditingLibrary.connect_material_property(node,'RGB',unreal.MaterialProperty.MP_BASE_COLOR)
    const(mat,metal,unreal.MaterialProperty.MP_METALLIC)
    const(mat,rough,unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.MaterialEditingLibrary.recompile_material(mat);save(mat)
    materials[suffix]=mat

led=create_material('M_BarrierLED')
color=unreal.MaterialEditingLibrary.create_material_expression(led,unreal.MaterialExpressionVectorParameter,-450,0)
color.set_editor_property('parameter_name','LEDColor');color.set_editor_property('default_value',unreal.LinearColor(1,.015,.005,1))
strength=unreal.MaterialEditingLibrary.create_material_expression(led,unreal.MaterialExpressionScalarParameter,-450,180)
strength.set_editor_property('parameter_name','Emission');strength.set_editor_property('default_value',8.0)
multiply=unreal.MaterialEditingLibrary.create_material_expression(led,unreal.MaterialExpressionMultiply,-200,100)
unreal.MaterialEditingLibrary.connect_material_expressions(color,'',multiply,'A')
unreal.MaterialEditingLibrary.connect_material_expressions(strength,'',multiply,'B')
unreal.MaterialEditingLibrary.connect_material_property(multiply,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
unreal.MaterialEditingLibrary.connect_material_property(color,'',unreal.MaterialProperty.MP_BASE_COLOR)
const(led,.28,unreal.MaterialProperty.MP_ROUGHNESS)
unreal.MaterialEditingLibrary.recompile_material(led);save(led)

mesh_editor=unreal.get_default_object(unreal.StaticMeshEditorSubsystem)
meshes={}
for suffix in ['Housing','Arm','RedLens','GreenLens']:
    name='SM_Barrier'+suffix
    opt=unreal.FbxImportUI()
    for k,v in dict(import_mesh=True,import_as_skeletal=False,import_materials=False,import_textures=False,
                    import_animations=False,automated_import_should_detect_type=False,mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH).items():
        opt.set_editor_property(k,v)
    data=opt.static_mesh_import_data
    for k,v in dict(combine_meshes=True,auto_generate_collision=False,generate_lightmap_u_vs=True,
                    convert_scene=True,convert_scene_unit=True,force_front_x_axis=False,
                    transform_vertex_to_absolute=True,build_nanite=False,import_uniform_scale=1.0).items():
        data.set_editor_property(k,v)
    data.set_editor_property('normal_import_method',unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
    mesh=import_file(name+'.fbx','Meshes',name,opt)
    mesh.set_material(0,materials.get(suffix,led))
    if suffix=='Housing':
        mesh_editor.remove_collisions(mesh)
        mesh_editor.add_simple_collisions(mesh,unreal.ScriptCollisionShapeType.BOX)
    save(mesh);meshes[suffix]=mesh

factory=unreal.BlueprintFactory()
factory.set_editor_property('parent_class',unreal.load_class(None,'/Script/TunaSweeper.TunaSweeperBarrierGateActor'))
bp=unreal.load_asset(DEST+'/BP_BarrierGate') if unreal.EditorAssetLibrary.does_asset_exist(DEST+'/BP_BarrierGate') else None
bp=bp or assets.create_asset('BP_BarrierGate',DEST,unreal.Blueprint,factory)
cdo=unreal.get_default_object(bp.generated_class())
for suffix,prop in [('Housing','housing_mesh'),('Arm','arm_mesh'),('RedLens','red_lens_mesh'),('GreenLens','green_lens_mesh')]:
    cdo.set_editor_property(prop,meshes[suffix])
cdo.set_editor_property('indicator_material',led)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
save(bp)
unreal.log('BARRIER_GATE_ASSETS_CREATED')
