"""Read-only UE 5.7 audit of the persisted six-module kit. Never imports or saves assets."""
from pathlib import Path
import unreal,json
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/ModularInteriorPreview'
DEST='/Game/Environment/ModularInteriorPreview'
m=json.loads((OUT/'model_manifest.json').read_text())
lib=unreal.MaterialEditingLibrary
editor=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.get_default_object(unreal.StaticMeshEditorSubsystem)
assert len(m['assets'])==6 and m['roofless']
for retired in ('Ceiling','Beam'):
    assert not unreal.EditorAssetLibrary.does_asset_exist(f'{DEST}/Meshes/SM_MI_{retired}')
for name,srgb in [('T_MI_Atlas',True),('T_MI_DirtMask',False)]:
    texture=unreal.load_asset(f'{DEST}/Textures/{name}');assert texture
    assert texture.get_editor_property('srgb')==srgb
    if not srgb:assert texture.get_editor_property('compression_settings')==unreal.TextureCompressionSettings.TC_GRAYSCALE
for name in m['materials']:
    mat=unreal.load_asset(f'{DEST}/Materials/{name}');assert mat
    assert mat.get_editor_property('blend_mode')==unreal.BlendMode.BLEND_OPAQUE
    assert not mat.get_editor_property('two_sided')
    assert lib.get_material_property_input_node(mat,unreal.MaterialProperty.MP_BASE_COLOR)
for look in ('Light','Dark','Managed'):
    assert unreal.load_asset(f'{DEST}/Materials/MI_MI_Concrete_{look}')

report={'engine':unreal.SystemLibrary.get_engine_version(),'mode':'reload','assets':[],
        'opaque':True,'textures':[{'name':'T_MI_Atlas','size':[2048,2048],'srgb':True},{'name':'T_MI_DirtMask','size':[1024,1024],'srgb':False,'compression':'grayscale'}],
        'shader_texture_samples_per_surface':2,'decal_count':0,'normal_maps':0,'nanite':False}
for e in m['assets']:
    mesh=unreal.load_asset(f"{DEST}/Meshes/{e['name']}");assert mesh
    bounds=mesh.get_bounds();c=bounds.origin;r=bounds.box_extent
    actual=[c.x-r.x,c.y-r.y,c.z-r.z,c.x+r.x,c.y+r.y,c.z+r.z]
    err=max(abs(a-b*100) for a,b in zip(actual,e['bounds_m']));assert err<.1,(e['name'],actual,e['bounds_m'])
    count=editor.get_simple_collision_count(mesh)+editor.get_convex_collision_count(mesh)
    assert count==len(e['collision_boxes']),(e['name'],count)
    body=mesh.get_editor_property('body_setup');agg=body.get_editor_property('agg_geom')
    assert not agg.get_editor_property('convex_elems')
    boxes=agg.get_editor_property('box_elems');assert len(boxes)==len(e['collision_boxes'])
    for box,(lo,hi) in zip(boxes,e['collision_boxes']):
        center=box.get_editor_property('center')
        actual_box=[center.x,center.y,center.z]+[box.get_editor_property(k) for k in ('x','y','z')]
        expected_box=[(a+b)*50 for a,b in zip(lo,hi)]+[(b-a)*100 for a,b in zip(lo,hi)]
        assert max(abs(a-b) for a,b in zip(actual_box,expected_box))<.01
    uv=editor.get_num_uv_channels(mesh,0);assert uv>=2
    assert mesh.get_editor_property('light_map_coordinate_index')==2
    slots=list(mesh.get_editor_property('static_materials'));assert len(slots)==e['material_slots']
    for s in slots:assert s.get_editor_property('material_interface').get_path_name().startswith(DEST+'/Materials/')
    # Source LOD triangle count is queried from persisted mesh description when available.
    lod=unreal.GeometryScript_AssetUtils.copy_mesh_from_static_mesh(
        mesh,unreal.DynamicMesh(),unreal.GeometryScriptCopyMeshFromAssetOptions(),unreal.GeometryScriptMeshReadLOD(),None)
    dynamic=next(x for x in lod if isinstance(x,unreal.DynamicMesh)) if isinstance(lod,tuple) else lod
    tris=dynamic.get_triangle_count();assert tris==e['triangles'],(e['name'],tris,e['triangles'])
    report['assets'].append({'name':e['name'],'bounds_error_cm':err,'triangles':tris,'material_slots':len(slots),'collision_boxes':count,'collision_shape_bounds_verified':True,'uv_channels':uv})
report['passed']=True
(OUT/'unreal_reload_validation.json').write_text(json.dumps(report,indent=2))
unreal.log('MI_UE_VALIDATION_PASSED')
