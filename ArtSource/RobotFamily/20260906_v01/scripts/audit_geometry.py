"""Extra asset audit: outward winding per closed island, rigid pose lengths, texture provenance."""
import bpy,bmesh,json,hashlib
from pathlib import Path
from mathutils import Vector
OUT=Path(__file__).resolve().parents[1]
source=OUT.parents[2]/"GeneratedImages/Enemies/RobotFamily/20260906_225120_reference_v01/robot_atlas_imagegen_source.png"
texture=OUT/"textures/T_RobotFamily_BaseColor.png"
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
assert sha(source)==sha(texture)
result={"imagegen_source_sha256":sha(source),"applied_texture_sha256":sha(texture),"models":{}}
for name in ["Q1_Scout","Q2_Bulwark","H1_Carrier","B1_Sentry"]:
 bpy.ops.wm.open_mainfile(filepath=str(OUT/(name+".blend")))
 ob=bpy.data.objects["SK_"+name];rig=bpy.data.objects["Armature"];me=ob.data
 # Find disconnected closed primitives and compute signed volume from winding.
 neighbors={v.index:set() for v in me.vertices}
 for e in me.edges:
  a,b=e.vertices;neighbors[a].add(b);neighbors[b].add(a)
 visited=set();islands=[]
 for v in me.vertices:
  if v.index in visited:continue
  pending=[v.index];ids=set()
  while pending:
   i=pending.pop()
   if i in ids:continue
   ids.add(i);visited.add(i);pending.extend(neighbors[i]-ids)
  islands.append(ids)
 me.calc_loop_triangles();volumes=[]
 for ids in islands:
  vol=0
  for tri in me.loop_triangles:
   if tri.vertices[0] in ids:
    a,b,c=[me.vertices[i].co for i in tri.vertices]
    vol+=a.dot(b.cross(c))/6
  volumes.append(vol)
 assert min(volumes)>0,(name,min(volumes))
 # Rigid weighting means every intra-bone distance remains unchanged by poses.
 maxerr=0;ground_min=1e9
 for frame in range(1,37):
  bpy.context.scene.frame_set(frame);bpy.context.view_layer.update()
  deps=bpy.context.evaluated_depsgraph_get();ev=ob.evaluated_get(deps);evaluated=ev.to_mesh()
  ground_min=min(ground_min,min(v.co.z for v in evaluated.vertices))
  for edge in me.edges:
   a,b=edge.vertices
   before=(me.vertices[a].co-me.vertices[b].co).length
   after=(evaluated.vertices[a].co-evaluated.vertices[b].co).length
   maxerr=max(maxerr,abs(before-after))
  ev.to_mesh_clear()
 assert maxerr<.001,(name,maxerr)
 assert ground_min>-.01,(name,ground_min)
 result["models"][name]={"closed_rigid_islands":len(islands),"minimum_signed_volume_cm3":min(volumes),"max_rigid_edge_length_error_cm":maxerr,"minimum_vertex_z_during_pose_cm":ground_min,"pose_frames_checked":36,"texture_size":list(next(im for im in bpy.data.images if im.source=='FILE').size)}
(OUT/"validation/additional_audit.json").write_text(json.dumps(result,indent=2),encoding="utf-8")
print(json.dumps(result,indent=2))
