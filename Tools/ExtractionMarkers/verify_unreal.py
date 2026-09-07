"""Read-only UE 5.7 stored asset audit; cannot import or save packages."""
from pathlib import Path
import unreal,json
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'TunaSweeper/SourceArt/Environment/ExtractionMarkers';DEST='/Game/Interaction/ExtractionMarkers';BASE='/Game/Environment/ModularInteriorPreview';REUSE='/Game/Environment/ModularInteriorExpansion'
m=json.loads((OUT/'model_manifest.json').read_text());editor=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
report={'passed':False,'engine':unreal.SystemLibrary.get_engine_version(),'assets':[]}
for e in m['assets']:
 mesh=unreal.load_asset((REUSE if e['reused_ue'] else DEST)+'/Meshes/'+e['name']);assert mesh
 b=mesh.get_bounds();c=b.origin;r=b.box_extent;bounds=[c.x-r.x,c.y-r.y,c.z-r.z,c.x+r.x,c.y+r.y,c.z+r.z];err=max(abs(a-b*100) for a,b in zip(bounds,e['bounds_m']));assert err<.1,(e['name'],bounds,e['bounds_m'])
 body=mesh.get_editor_property('body_setup');agg=body.get_editor_property('agg_geom');boxes=agg.get_editor_property('box_elems');assert len(boxes)==len(e['collision_boxes']);assert not agg.get_editor_property('convex_elems')
 for box,(lo,hi) in zip(boxes,e['collision_boxes']):
  c=box.get_editor_property('center');actual=[c.x,c.y,c.z]+[box.get_editor_property(k) for k in ('x','y','z')];expected=[(a+b)*50 for a,b in zip(lo,hi)]+[(b-a)*100 for a,b in zip(lo,hi)];assert max(abs(a-b) for a,b in zip(actual,expected))<.01
 uv=editor.get_num_uv_channels(mesh,0);assert uv>=2;assert mesh.get_editor_property('light_map_coordinate_index')==2
 slots=list(mesh.get_editor_property('static_materials'));assert len(slots)==e['material_slots']
 for slot in slots:assert slot.get_editor_property('material_interface')
 assert not mesh.get_editor_property('nanite_settings').get_editor_property('enabled')
 lod=unreal.GeometryScript_AssetUtils.copy_mesh_from_static_mesh(mesh,unreal.DynamicMesh(),unreal.GeometryScriptCopyMeshFromAssetOptions(),unreal.GeometryScriptMeshReadLOD(),None);dynamic=next(x for x in lod if isinstance(x,unreal.DynamicMesh)) if isinstance(lod,tuple) else lod;tris=dynamic.get_triangle_count();assert tris==e['triangles']
 report['assets'].append({'name':e['name'],'triangles':tris,'slots':len(slots),'bounds_error_cm':err,'uv_channels':uv,'simple_boxes':len(boxes),'nanite':False})
lib=unreal.MaterialEditingLibrary;report['materials']=[]
for path,count in [(DEST+'/Materials/M_EM_Surface',2),(DEST+'/Materials/M_EM_Green',0),(BASE+'/Materials/M_MI_Steel',2),(REUSE+'/Materials/M_MI_ExpansionAmber',0)]:
 mat=unreal.load_asset(path);assert mat and mat.get_editor_property('blend_mode')==unreal.BlendMode.BLEND_OPAQUE
 nodes=set();pending=[lib.get_material_property_input_node(mat,getattr(unreal.MaterialProperty,'MP_'+p)) for p in ('BASE_COLOR','ROUGHNESS','METALLIC','EMISSIVE_COLOR')]
 while pending:
  n=pending.pop()
  if not n or n in nodes:continue
  nodes.add(n);pending.extend(lib.get_inputs_for_material_expression(mat,n))
 samples=[n for n in nodes if isinstance(n,unreal.MaterialExpressionTextureSample)];assert len(samples)==count
 report['materials'].append({'path':path,'reachable_texture_samples':len(samples),'textures':[n.get_editor_property('texture').get_path_name() for n in samples]})
report['textures']=[]
for path,size in [(DEST+'/Textures/T_EM_Atlas',1024),(BASE+'/Textures/T_MI_DirtMask',1024),(BASE+'/Textures/T_MI_Atlas',2048)]:
 t=unreal.load_asset(path);actual=[t.blueprint_get_size_x(),t.blueprint_get_size_y()];assert actual==[size,size],(path,actual,size);report['textures'].append({'path':path,'size':actual})
bp=unreal.EditorAssetLibrary.load_blueprint_class('/Game/Interaction/BP_ExtractionPoint');assert bp
cdo=unreal.get_default_object(bp);report['extraction_bp_defaults']={}
for k in ('extraction_radius','radius_ring_width','radius_visual_z_offset','extraction_hold_seconds','extraction_particle_system'):
 try:report['extraction_bp_defaults'][k]=str(cdo.get_editor_property(k))
 except Exception:report['extraction_bp_defaults'][k]='not exposed under this Python name'
for path in ('/Game/FX/NS_ExtractionSmoke','/Game/Effects/NS_ExtractionSmokeSignal'):assert unreal.load_asset(path)
report['passed']=True;(OUT/'unreal_reload_validation.json').write_text(json.dumps(report,indent=2));unreal.log('EM_UE_ASSETS_PASSED')
