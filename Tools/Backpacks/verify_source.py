"""Reload and validate saved backpack assets. Does not generate or edit models."""
import bpy,json,math,hashlib
from pathlib import Path
from collections import defaultdict
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'TunaSweeper/SourceArt/Props/Backpacks'
TARGET=json.loads((OUT/'UV/density_target.json').read_text())['target_pixels_per_metre']
SPECS={s['name']:s for s in json.loads((OUT/'texture_manifest.json').read_text())}
def area(t):return abs(sum(t[i][0]*t[(i+1)%len(t)][1]-t[(i+1)%len(t)][0]*t[i][1] for i in range(len(t)))/2)
def cross(a,b,c):return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])
def intersection(poly,clip):
 if cross(*clip)<0:clip=list(reversed(clip))
 for i,a in enumerate(clip):
  b=clip[(i+1)%3];src=poly;poly=[]
  if not src:break
  for j,q in enumerate(src):
   p=src[j-1];dp=cross(a,b,p);dq=cross(a,b,q)
   if (dp>=0)!=(dq>=0):
    f=dp/(dp-dq);poly.append((p[0]+f*(q[0]-p[0]),p[1]+f*(q[1]-p[1])))
   if dq>=0:poly.append(q)
 return area(poly) if len(poly)>=3 else 0
def measure(o,texture=True):
 m=o.data;m.calc_loop_triangles();assert len(m.loop_triangles)<500,o.name
 assert len(m.uv_layers)==2 and len(m.materials)==1,o.name
 uv=m.uv_layers[0].data;tris=[];edges=defaultdict(list);parents=list(range(len(m.polygons)))
 assert all((a.uv-b.uv).length<1e-6 for a,b in zip(uv,m.uv_layers[1].data)),(o.name,'UV1 differs from verified UV0')
 assert all((a.uv-b.uv).length<1e-6 for a,b in zip(uv,m.uv_layers[1].data)),(o.name,'UV1 differs from verified UV0')
 def find(i):
  while parents[i]!=i:parents[i]=parents[parents[i]];i=parents[i]
  return i
 for p in m.polygons:
  assert len(p.vertices)==3 and p.area>1e-10,(o.name,p.index,'geometry')
  t=[tuple(uv[i].uv) for i in p.loop_indices];assert area(t)>1e-12,(o.name,p.index,'UV')
  assert all(-1e-6<=v<=1+1e-6 and math.isfinite(v) for pt in t for v in pt)
  assert all(math.isfinite(v) for v in p.normal) and p.normal.length>.99
  tris.append(t)
  for j in range(3):
   keys=sorted([(int(p.vertices[j]),tuple(round(v,6) for v in t[j])),(int(p.vertices[(j+1)%3]),tuple(round(v,6) for v in t[(j+1)%3]))])
   edges[tuple(keys)].append(p.index)
 for fs in edges.values():
  for j in fs[1:]:parents[find(j)]=find(fs[0])
 groups=defaultdict(list)
 for i in range(len(parents)):groups[find(i)].append(i)
 overlaps=[]
 boxes=[(min(x for x,y in t),min(y for x,y in t),max(x for x,y in t),max(y for x,y in t)) for t in tris]
 for i,a in enumerate(boxes):
  for j in range(i):
   b=boxes[j]
   if a[0]>=b[2]-1e-8 or b[0]>=a[2]-1e-8 or a[1]>=b[3]-1e-8 or b[1]>=a[3]-1e-8:continue
   if intersection(tris[i],tris[j])>1e-9:overlaps.append([i,j])
 assert not overlaps,(o.name,'UV overlaps',overlaps[:8])
 name=o.name.removeprefix('SM_Backpack_');spec=SPECS[name];densities=[]
 for fs in groups.values():densities.append(spec['size']*math.sqrt(sum(area(tris[i]) for i in fs)/sum(m.polygons[i].area for i in fs)))
 density=spec['size']*math.sqrt(sum(area(t) for t in tris)/sum(p.area for p in m.polygons))
 assert abs(density/TARGET-1)<.001,(o.name,density,TARGET)
 # Blender average-island normalization must keep whole-island density aligned.
 assert max(abs(d/TARGET-1) for d in densities)<.03,(o.name,min(densities),max(densities))
 assert len(groups)<len(m.polygons)/3,(o.name,'fragmented UV')
 image=None
 if texture:
  image=m.materials[0].node_tree.nodes['GeneratedAtlas'].image
  assert image and image.packed_file and tuple(image.size)==(spec['size'],spec['size'])
  assert image.filepath.replace('\\','/').rsplit('/',1)[-1]==spec['texture']
 coords=[v.co for v in m.vertices];bounds=[min(v[i] for v in coords) for i in range(3)]+[max(v[i] for v in coords) for i in range(3)]
 return {'mesh':o.name,'triangles':len(m.loop_triangles),'vertices':len(m.vertices),'uv_islands':len(groups),'island_face_counts':sorted([len(fs) for fs in groups.values()],reverse=True),'uv_overlap_pairs':0,'density_pixels_per_metre':density,'island_density_min':min(densities),'island_density_max':max(densities),'texture':spec['texture'],'texture_size':spec['size'],'bounds_m':bounds,'dimensions_cm':[(bounds[i+3]-bounds[i])*100 for i in range(3)]}
bpy.ops.wm.open_mainfile(filepath=str(OUT/'Backpacks.blend'))
objs=sorted([o for o in bpy.context.scene.objects if o.type=='MESH'],key=lambda o:o['tier']);assert len(objs)==5
report={'passed':False,'blender':bpy.app.version_string,'models':[measure(o) for o in objs]}
assert len({o.data.materials[0].node_tree.nodes['GeneratedAtlas'].image.as_pointer() for o in objs})==5
assert all(not o.modifiers for o in objs)
for expected in report['models']:
 bpy.ops.wm.read_factory_settings(use_empty=True)
 bpy.ops.import_scene.fbx(filepath=str(OUT/'Models'/f"{expected['mesh']}.fbx"))
 imported=[o for o in bpy.context.scene.objects if o.type=='MESH'];assert len(imported)==1
 imported[0].name=expected['mesh'];got=measure(imported[0],False)
 assert got['triangles']==expected['triangles'] and got['uv_islands']==expected['uv_islands']
 assert max(abs(a-b) for a,b in zip(got['dimensions_cm'],expected['dimensions_cm']))<.01
 expected['fbx_reload_passed']=True
report['passed']=True
report['sha256']={p.relative_to(OUT).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in [OUT/'Backpacks.blend',*sorted((OUT/'Models').glob('*.fbx')),*sorted((OUT/'Textures').glob('*.png'))]}
(OUT/'source_validation.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print(json.dumps(report,indent=2))
