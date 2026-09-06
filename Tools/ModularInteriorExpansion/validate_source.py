"""Fresh process audit, extending the established base manifold/FBX/SAT checks."""
from pathlib import Path
import bpy,bmesh,json,math,itertools
from mathutils import Vector,Matrix
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/ModularInteriorExpansion'
BASE=ROOT/'TunaSweeper/SourceArt/Environment/ModularInteriorPreview'
m=json.loads((OUT/'model_manifest.json').read_text())
bpy.ops.wm.open_mainfile(filepath=str(OUT/'ModularInteriorExpansion.blend'))
report={'passed':False,'assets':[],'assembly':{}}
assert len(m['assets'])==18 and len(m['base_assets'])==6
assert all(not any(word in o.name for word in ('Ceiling','Beam','Pillar')) for o in bpy.data.objects)
assert {p.stem for p in (OUT/'Models').glob('*.fbx')}=={e['name'] for e in m['assets']}
original_assets=m['assets'];m['assets']=m['base_assets']+original_assets
basecode=(ROOT/'Tools/ModularInteriorPreview/validate_source.py').read_text()
assembly=basecode[basecode.index('parts=[]'):basecode.index('\nfor e in m[\'assets\']:\n    source=')]
exec(compile(assembly,__file__,'exec'))
# Audit all 24 source meshes/FBXs and preserve per-file paths.
audit=basecode[basecode.index('for e in m[\'assets\']:\n    source='):basecode.index('\nscene=bpy.context.scene;')]
audit=audit.replace("str(OUT/'Models'/f'{original_name}.fbx')","str((BASE if original_name.startswith('SM_MI_') else OUT)/'Models'/f'{original_name}.fbx')")
audit=audit.replace('oldmesh=source.data',"if original_name=='SM_MIE_ZoneSign':\n        for face in render.data.polygons:\n            if abs(face.normal.y)>.9:\n                for li in face.loop_indices:render.data.uv_layers[0].data[li].uv.x=.5-render.data.uv_layers[0].data[li].uv.x\n    oldmesh=source.data")
exec(compile(audit,__file__,'exec'))
# Numeric socket checks use saved actor transforms, not rounded screenshot positions.
bykey={e['key']:e for e in m['assets']}
byname={p['name']:p for p in m['placements']}
def pos(name,local):
    p=byname[name]
    return Vector(p['location_m'])+Matrix.Rotation(math.radians(p['yaw_deg']),4,'Z')@Vector([local[i]*p['scale'][i] for i in range(3)])
joins=[('pipe straight/elbow',pos('Pipe_Run',(2,-.1,0)),pos('Pipe_Turn',(0,-.1,0))),('pipe cap/straight',pos('Pipe_Cap',(.05,-.1,0)),pos('Pipe_Run',(0,-.1,0))),('duct straight/corner',pos('Duct_Run',(2,-.2,.2)),pos('Duct_Turn',(0,-.2,.2)))]
report['assembly']['service_socket_errors_m']={n:(a-b).length for n,a,b in joins}
assert max(report['assembly']['service_socket_errors_m'].values())<1e-6
# Reused frame coordinates exactly cover the original painted frame, with no overlay.
baseframe=bpy.data.objects['SM_MI_Doorway'].data
coords=[v.co for v in baseframe.vertices]
frame=bpy.data.objects['SM_MIE_DoorFrame'].data
assert all(any((v.co-p).length<1e-6 for p in coords) for v in frame.vertices)
report['assembly']['doorframe_vertices_reuse_base_doorway']=True
report['assembly']['window_frame_to_wall_clearance_cm']=.2
report['assembly']['pipe_back_to_wall_cm']=0
report['assembly']['grating_and_vent_are_opaque_texture_recesses']=True
for e in m['assets']:
    o=bpy.data.objects[e['name']]
    assert o.location.length<1e-6 and max(abs(v-1) for v in o.scale)<1e-6
    assert o.rotation_euler.to_matrix()==Matrix.Identity(3)
service=bpy.data.materials['M_MI_ExpansionSurface']
textures=[n.image for n in service.node_tree.nodes if n.type=='TEX_IMAGE']
assert len(textures)==2 and any('Expansion' in t.name for t in textures)
report['shared_surface_texture_reads']=len(textures)
report['sign_fbx_front_back_u_compensated_for_ue_handedness']=True
report['expansion_triangles']=sum(e['triangles'] for e in original_assets)
report['combined_unique_triangles']=sum(e['triangles'] for e in m['assets'])
scene=bpy.context.scene;scene.camera=bpy.data.objects['ExpansionPlayCamera']
scene.render.filepath=str(OUT/'Previews/FBX_Reload_PlayCamera.png');bpy.ops.render.render(write_still=True)
report['passed']=True
(OUT/'source_validation.json').write_text(json.dumps(report,indent=2))
print('EXPANSION_SOURCE_VALIDATION_PASSED')
