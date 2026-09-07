"""Import authored noise assets and validate the direct-placement BP (UE 5.7)."""
import json, os
from pathlib import Path
import unreal
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'TunaSweeper/SourceArt/Props/NoiseEmitter'
DEST='/Game/Meshes/Props/NoiseEmitter'
VERIFY=os.environ.get('NOISE_VERIFY_ONLY')=='1'
spec=json.loads((OUT/'model_validation.json').read_text())
assets=unreal.AssetToolsHelpers.get_asset_tools()
editor=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.get_default_object(unreal.StaticMeshEditorSubsystem)
def save(asset):
    assert unreal.EditorAssetLibrary.save_loaded_asset(asset,only_if_is_dirty=False)
materials={}
for name,color in spec['materials'].items():
    m=unreal.load_asset(DEST+'/'+name)
    if not VERIFY:
        m=m or assets.create_asset(name,DEST,unreal.Material,unreal.MaterialFactoryNew())
        unreal.MaterialEditingLibrary.delete_all_material_expressions(m)
        rgb=unreal.MaterialEditingLibrary.create_material_expression(m,unreal.MaterialExpressionConstant3Vector,-500,0)
        rgb.set_editor_property('constant',unreal.LinearColor(*color))
        pulse=unreal.MaterialEditingLibrary.create_material_expression(m,unreal.MaterialExpressionScalarParameter,-500,150)
        pulse.set_editor_property('parameter_name','PulseColorScale'); pulse.set_editor_property('default_value',1.0)
        mult=unreal.MaterialEditingLibrary.create_material_expression(m,unreal.MaterialExpressionMultiply,-250,0)
        unreal.MaterialEditingLibrary.connect_material_expressions(rgb,'',mult,'A')
        unreal.MaterialEditingLibrary.connect_material_expressions(pulse,'',mult,'B')
        unreal.MaterialEditingLibrary.connect_material_property(mult,'',unreal.MaterialProperty.MP_BASE_COLOR)
        rough=unreal.MaterialEditingLibrary.create_material_expression(m,unreal.MaterialExpressionConstant,-250,200)
        rough.set_editor_property('r',.72)
        unreal.MaterialEditingLibrary.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS)
        unreal.MaterialEditingLibrary.recompile_material(m); save(m)
    assert m
    materials[name]=m
report={'engine':unreal.SystemLibrary.get_engine_version(),'mode':'reload' if VERIFY else 'import','meshes':[]}
for name in ['SM_NoiseEmitter_Body','SM_NoiseEmitter_Horn']:
    if not VERIFY:
        options=unreal.FbxImportUI()
        for k,v in {'import_mesh':True,'import_as_skeletal':False,'import_animations':False,'import_materials':False,'import_textures':False,'automated_import_should_detect_type':False,'mesh_type_to_import':unreal.FBXImportType.FBXIT_STATIC_MESH}.items(): options.set_editor_property(k,v)
        data=options.static_mesh_import_data
        for k,v in {'combine_meshes':True,'auto_generate_collision':False,'generate_lightmap_u_vs':True,'convert_scene':True,'convert_scene_unit':True,'force_front_x_axis':False,'transform_vertex_to_absolute':True,'build_nanite':False,'import_uniform_scale':1.0}.items(): data.set_editor_property(k,v)
        data.set_editor_property('normal_import_method',unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
        task=unreal.AssetImportTask(); task.filename=str(OUT/(name+'.fbx')); task.destination_path=DEST; task.destination_name=name
        task.automated=True; task.replace_existing=True; task.save=True; task.options=options; task.factory=unreal.FbxFactory()
        assets.import_asset_tasks([task])
    mesh=unreal.load_asset(DEST+'/'+name); assert isinstance(mesh,unreal.StaticMesh)
    if not VERIFY:
        for i,slot in enumerate(mesh.get_editor_property('static_materials')):
            slotname=str(slot.get_editor_property('material_slot_name'))
            assert slotname in materials,slotname
            mesh.set_material(i,materials[slotname])
        mesh.set_editor_property('allow_cpu_access',True); save(mesh)
    assert mesh.get_editor_property('allow_cpu_access')
    bounds=mesh.get_bounds(); c=bounds.origin; e=bounds.box_extent
    actual=[c.x-e.x,c.y-e.y,c.z-e.z,c.x+e.x,c.y+e.y,c.z+e.z]
    expected=[-26,-26,0,26,26,172] if name.endswith('Body') else [0,-52,-52,118,52,52]
    # Body includes the 68cm hub, exceeding the footplate width.
    if name.endswith('Body'): expected=[-34,-34,0,34,34,172]
    assert max(abs(a-b) for a,b in zip(actual,expected))<.1,(name,actual,expected)
    assert editor.get_num_uv_channels(mesh,0)>=1
    assert editor.get_simple_collision_count(mesh)+editor.get_convex_collision_count(mesh)==0
    report['meshes'].append({'name':name,'bounds_cm':actual,'cpu_access':True,'vertices':editor.get_number_verts(mesh,0)})
bp_path='/Game/Interaction/BP_PeriodicNoiseEmitter'
bp=unreal.load_asset(bp_path)
if not VERIFY:
    if not bp:
        factory=unreal.BlueprintFactory(); factory.set_editor_property('parent_class',unreal.TunaSweeperPeriodicNoiseEmitterActor)
        bp=assets.create_asset('BP_PeriodicNoiseEmitter','/Game/Interaction',unreal.Blueprint,factory)
    defaults=unreal.get_default_object(bp.generated_class())
    defaults.set_editor_property('body_source_mesh',unreal.load_asset(DEST+'/SM_NoiseEmitter_Body'))
    defaults.set_editor_property('horn_source_mesh',unreal.load_asset(DEST+'/SM_NoiseEmitter_Horn'))
    unreal.BlueprintEditorLibrary.compile_blueprint(bp); save(bp)
assert bp
defaults=unreal.get_default_object(bp.generated_class())
for prop,value in [('noise_interval_seconds',2.0),('noise_loudness',1.0),('noise_max_range',2600.0)]: assert abs(defaults.get_editor_property(prop)-value)<.001
for prop in ['body_source_mesh','horn_source_mesh']: assert defaults.get_editor_property(prop)
world=unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
actor=unreal.EditorLevelLibrary.spawn_actor_from_class(bp.generated_class(),unreal.Vector(0,0,0))
component=actor.get_component_by_class(unreal.ProceduralMeshComponent)
assert component.get_num_sections()>=5,component.get_num_sections()
report['blueprint']=bp_path; report['constructed_sections']=component.get_num_sections(); report['passed']=True
(OUT/('unreal_reload_validation.json' if VERIFY else 'unreal_import_validation.json')).write_text(json.dumps(report,indent=2)+'\n')
unreal.log('NOISE_EMITTER_VALIDATION_PASSED')
