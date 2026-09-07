"""One-off asset and review BP/map generator; remove after its validated commit."""
from pathlib import Path
import unreal,json,traceback,os
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/LootContainerSet'
DEST='/Game/Interaction/LootContainerSet'
MAT='/Game/Environment/LabSupplyProps/Materials/M_LSP_Surface'
def save(o):
    assert o.get_path_name().startswith(DEST+'/')
    assert unreal.EditorAssetLibrary.save_loaded_asset(o,only_if_is_dirty=False)
def main():
    m=json.loads((OUT/'model_manifest.json').read_text())
    assets=unreal.AssetToolsHelpers.get_asset_tools();ed=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    mat=unreal.load_asset(MAT);assert mat
    for entry in ([] if os.getenv('LC_MAP_ONLY') else m['assets']):
        options=unreal.FbxImportUI()
        for k,v in dict(import_mesh=True,import_as_skeletal=False,import_animations=False,import_materials=False,import_textures=False,automated_import_should_detect_type=False,mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH).items():options.set_editor_property(k,v)
        for k,v in dict(combine_meshes=True,auto_generate_collision=False,one_convex_hull_per_ucx=True,generate_lightmap_u_vs=True,convert_scene=True,convert_scene_unit=True,force_front_x_axis=False,transform_vertex_to_absolute=True,build_nanite=False,remove_degenerates=True,import_uniform_scale=1.,vertex_color_import_option=unreal.VertexColorImportOption.REPLACE,normal_import_method=unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS).items():options.static_mesh_import_data.set_editor_property(k,v)
        t=unreal.AssetImportTask();t.filename=str(OUT/'Models'/(entry['name']+'.fbx'));t.destination_path=DEST+'/Meshes';t.destination_name=entry['name'];t.automated=True;t.replace_existing=True;t.replace_existing_settings=True;t.save=True;t.options=options;t.factory=unreal.FbxFactory()
        assets.import_asset_tasks([t]);mesh=unreal.load_asset(DEST+'/Meshes/'+entry['name']);assert mesh
        mesh.set_material(0,mat)
        b=ed.get_lod_build_settings(mesh,0)
        for k,v in dict(recompute_normals=False,recompute_tangents=True,generate_lightmap_u_vs=True,src_lightmap_index=0,dst_lightmap_index=2).items():b.set_editor_property(k,v)
        ed.set_lod_build_settings(mesh,0,b);mesh.set_editor_property('light_map_coordinate_index',2)
        bs=mesh.get_editor_property('body_setup');agg=bs.get_editor_property('agg_geom');boxes=[]
        for lo,hi in entry['collision_boxes']:
            box=unreal.KBoxElem();box.set_editor_property('center',unreal.Vector(*[(a+b)*50 for a,b in zip(lo,hi)]));box.set_editor_property('rotation',unreal.Rotator())
            for i,k in enumerate(['x','y','z']):box.set_editor_property(k,(hi[i]-lo[i])*100)
            boxes.append(box)
        agg.set_editor_property('convex_elems',[]);agg.set_editor_property('box_elems',boxes);bs.set_editor_property('agg_geom',agg);bs.set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AND_COMPLEX);save(mesh)
    # Empty review subclasses inherit the production native opening/closing code.
    # Definition 0 deliberately keeps review geometry independent of live loot rows.
    parent=unreal.load_class(None,'/Script/TunaSweeper.TunaSweeperLootContainerActor');assert parent
    for spec in m['sets']:
        name='BP_LC_Review_'+spec['key'];path=DEST+'/Review/'+name
        bp=unreal.load_asset(path)
        if not bp:
            f=unreal.BlueprintFactory();f.set_editor_property('parent_class',parent);bp=assets.create_asset(name,DEST+'/Review',unreal.Blueprint,f)
        cdo=unreal.get_default_object(bp.generated_class())
        for k,v in dict(container_definition_id=0,contents_id=0,body_mesh_override=unreal.load_asset(DEST+'/Meshes/SM_LC_'+spec['key']+'_Body'),lid_mesh_override=unreal.load_asset(DEST+'/Meshes/SM_LC_'+spec['key']+'_Lid'),lid_pivot_relative_location=unreal.Vector(*[v*100 for v in spec['hinge_m']])).items():cdo.set_editor_property(k,v)
        body=cdo.get_body_mesh_component();lid=cdo.get_lid_mesh_component()
        for comp,part in [(body,'Body'),(lid,'Lid')]:
            comp.set_editor_property('relative_location',unreal.Vector());comp.set_editor_property('relative_scale3d',unreal.Vector(1,1,1));comp.set_editor_property('relative_rotation',unreal.Rotator());comp.set_static_mesh(unreal.load_asset(DEST+'/Meshes/SM_LC_'+spec['key']+'_'+part));comp.set_material(0,mat)
            comp.set_collision_profile_name('BlockAllDynamic');comp.set_mobility(unreal.ComponentMobility.MOVABLE)
            for i,v in enumerate([.18,1,0,0]):comp.set_default_custom_primitive_data_float(i,v)
        unreal.BlueprintEditorLibrary.compile_blueprint(bp);save(bp)
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    level=DEST+'/Review/L_LootContainerSet'
    if unreal.EditorAssetLibrary.does_asset_exist(level):
        assert levels.load_level(level)
        actors.destroy_actors([a for a in actors.get_all_level_actors() if a.get_actor_label().startswith('LC_')])
    else:assert levels.new_level(level)
    def spawn(cls,name,loc,rot=None):
        a=actors.spawn_actor_from_class(cls,unreal.Vector(*loc),rot or unreal.Rotator());a.set_actor_label(name);return a
    for x,spec in zip([-180,0,200],m['sets']):
        cls=unreal.EditorAssetLibrary.load_blueprint_class(DEST+'/Review/BP_LC_Review_'+spec['key'])
        a=spawn(cls,'LC_'+spec['key'],[x,0,0])
        # Assert BP component templates survived compile and inherited construction.
        for c in [a.get_body_mesh_component(),a.get_lid_mesh_component()]:
            assert c.get_editor_property('relative_location').is_nearly_zero(),str(c.get_editor_property('relative_location'))
            assert (c.get_editor_property('relative_scale3d')-unreal.Vector(1,1,1)).is_nearly_zero()
    floor=spawn(unreal.StaticMeshActor,'LC_ReviewGround',[0,0,-7]);floor.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'));floor.set_actor_scale3d(unreal.Vector(100,100,.1));floor.static_mesh_component.set_collision_profile_name('BlockAll');floor.static_mesh_component.set_material(0,unreal.load_asset('/Engine/BasicShapes/BasicShapeMaterial'))
    sky=spawn(unreal.SkyLight,'LC_Daylight_Ambient',[0,0,400]);sc=sky.get_component_by_class(unreal.SkyLightComponent);sc.set_mobility(unreal.ComponentMobility.MOVABLE);sc.set_editor_property('source_type',unreal.SkyLightSourceType.SLS_SPECIFIED_CUBEMAP);sc.set_cubemap(unreal.load_asset('/Engine/MapTemplates/Sky/DaylightAmbientCubemap'));sc.set_intensity(1.5)
    sun=spawn(unreal.DirectionalLight,'LC_Daylight_Sun',[0,0,600],unreal.Rotator(pitch=-45,yaw=45));sun.get_component_by_class(unreal.DirectionalLightComponent).set_intensity(4)
    for i,loc in enumerate([[-200,200,650],[300,-200,600]]):
        a=spawn(unreal.RectLight,'LC_Daylight_'+str(i),loc,unreal.Rotator(pitch=-90));c=a.get_component_by_class(unreal.RectLightComponent);c.set_mobility(unreal.ComponentMobility.MOVABLE);c.set_intensity(2200);c.set_attenuation_radius(1800);c.set_source_width(600);c.set_source_height(600)
    for name,loc,target in [('Oblique',[400,640,640],[0,0,65]),('Topdown',[-1,0,1050],[0,0,0]),('Rear',[-450,-700,600],[0,0,65])]:
        a=spawn(unreal.CameraActor,'LC_Camera_'+name,loc,unreal.MathLibrary.find_look_at_rotation(unreal.Vector(*loc),unreal.Vector(*target)));c=a.camera_component;c.set_field_of_view(48);c.set_aspect_ratio(1.44)
        settings=c.get_editor_property('post_process_settings')
        settings.set_editor_property('override_auto_exposure_bias',True);settings.set_editor_property('auto_exposure_bias',1)
        c.set_editor_property('post_process_settings',settings);c.set_editor_property('post_process_blend_weight',1)
    assert levels.save_current_level()
    code=Path(__file__).with_name('verify_unreal_assets.py');scope={'__file__':str(code)};exec(compile(code.read_text(),str(code),'exec'),scope)
    r=scope['report'];r['mode']='import';(OUT/'unreal_import_validation.json').write_text(json.dumps(r,indent=2))
    (OUT/'unreal_map_creation.json').write_text(json.dumps({'passed':True,'map':level,'review_bps':3,'live_definition_ids_modified':False},indent=2))
try: main()
except Exception: unreal.log_error(traceback.format_exc())
unreal.SystemLibrary.quit_editor()
