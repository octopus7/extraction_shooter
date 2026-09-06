"""Read-only persisted mesh/material audit. This file cannot import or save assets."""
from pathlib import Path
import unreal,json
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/ModularInteriorExpansion'
DEST='/Game/Environment/ModularInteriorExpansion'
BASE='/Game/Environment/ModularInteriorPreview'
m=json.loads((OUT/'model_manifest.json').read_text())
editor=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.get_default_object(unreal.StaticMeshEditorSubsystem)
report={'engine':unreal.SystemLibrary.get_engine_version(),'mode':'reload','passed':False,'assets':[]}
assert len(m['assets'])==18
for e in m['base_assets']+m['assets']:
    folder=BASE if e in m['base_assets'] else DEST
    mesh=unreal.load_asset(folder+'/Meshes/'+e['name']);assert mesh
    b=mesh.get_bounds();c=b.origin;r=b.box_extent
    actual=[c.x-r.x,c.y-r.y,c.z-r.z,c.x+r.x,c.y+r.y,c.z+r.z]
    err=max(abs(a-b*100) for a,b in zip(actual,e['bounds_m']));assert err<.1,(e['name'],actual)
    body=mesh.get_editor_property('body_setup');agg=body.get_editor_property('agg_geom')
    assert body.get_editor_property('collision_trace_flag')==unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AND_COMPLEX
    assert not agg.get_editor_property('convex_elems')
    boxes=agg.get_editor_property('box_elems');assert len(boxes)==len(e['collision_boxes'])
    for box,(lo,hi) in zip(boxes,e['collision_boxes']):
        c=box.get_editor_property('center')
        actual=[c.x,c.y,c.z]+[box.get_editor_property(k) for k in ('x','y','z')]
        expected=[(a+b)*50 for a,b in zip(lo,hi)]+[(b-a)*100 for a,b in zip(lo,hi)]
        assert max(abs(a-b) for a,b in zip(actual,expected))<.01
    uv=editor.get_num_uv_channels(mesh,0);assert uv>=2,(e['name'],uv)
    build=editor.get_lod_build_settings(mesh,0)
    assert build.get_editor_property('generate_lightmap_u_vs') and build.get_editor_property('dst_lightmap_index')==2
    assert mesh.get_editor_property('light_map_coordinate_index')==2
    slots=list(mesh.get_editor_property('static_materials'));assert len(slots)==e['material_slots']
    for slot in slots:
        mat=slot.get_editor_property('material_interface');assert mat
        assert mat.get_path_name().startswith((BASE+'/Materials/',DEST+'/Materials/'))
    assert not mesh.get_editor_property('nanite_settings').get_editor_property('enabled')
    lod=unreal.GeometryScript_AssetUtils.copy_mesh_from_static_mesh(mesh,unreal.DynamicMesh(),unreal.GeometryScriptCopyMeshFromAssetOptions(),unreal.GeometryScriptMeshReadLOD(),None)
    dynamic=next(x for x in lod if isinstance(x,unreal.DynamicMesh)) if isinstance(lod,tuple) else lod
    tris=dynamic.get_triangle_count();assert tris==e['triangles'],(e['name'],tris,e['triangles'])
    report['assets'].append({'name':e['name'],'triangles':tris,'slots':len(slots),'bounds_error_cm':err,'collision_boxes':len(boxes),'collision_bounds_verified':True,'source_uv_channels':uv,'generated_lightmap_uv_index':2,'nanite':False})
atlas=unreal.load_asset(DEST+'/Textures/T_MIE_ServiceAtlas');assert atlas and atlas.get_editor_property('srgb')
assert atlas.blueprint_get_size_x()==2048 and atlas.blueprint_get_size_y()==2048
lib=unreal.MaterialEditingLibrary
report['material_graphs']=[]
for suffix in ('Surface','Amber','Glass'):
    mat=unreal.load_asset(DEST+'/Materials/M_MI_Expansion'+suffix);assert mat
    expected=unreal.BlendMode.BLEND_TRANSLUCENT if suffix=='Glass' else unreal.BlendMode.BLEND_OPAQUE
    assert mat.get_editor_property('blend_mode')==expected
    nodes=set();pending=[lib.get_material_property_input_node(mat,getattr(unreal.MaterialProperty,'MP_'+p)) for p in ('BASE_COLOR','ROUGHNESS','METALLIC','EMISSIVE_COLOR','OPACITY')]
    while pending:
        n=pending.pop()
        if not n or n in nodes:continue
        nodes.add(n);pending.extend(lib.get_inputs_for_material_expression(mat,n))
    samples=[n for n in nodes if isinstance(n,unreal.MaterialExpressionTextureSample)]
    assert len(samples)==(2 if suffix=='Surface' else 0),(suffix,len(samples))
    used=[n.get_editor_property('texture').get_path_name() for n in samples]
    if suffix=='Surface':
        assert len(used)==2 and any('T_MI_DirtMask' in p for p in used) and any('T_MIE_ServiceAtlas' in p for p in used)
    if suffix=='Glass':
        opacity=lib.get_material_property_input_node(mat,unreal.MaterialProperty.MP_OPACITY)
        assert abs(opacity.get_editor_property('default_value')-.22)<1e-6
        assert not lib.get_material_property_input_node(mat,unreal.MaterialProperty.MP_REFRACTION)
    report['material_graphs'].append({'name':mat.get_name(),'reachable_texture_sample_nodes':len(samples),'used_textures':used})
report['materials']={'opaque_structure':True,'glass_translucent_slots':1,'glass_refraction':False,'service_texture_reads':2,'constant_glass_and_amber_texture_reads':0,'new_texture_size':[2048,2048]}
report['passed']=True
(OUT/'unreal_reload_validation.json').write_text(json.dumps(report,indent=2))
unreal.log('EXPANSION_UE_ASSETS_PASSED')
