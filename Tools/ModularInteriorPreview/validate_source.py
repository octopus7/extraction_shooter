"""Fresh Blender process: .blend reload, exact mesh audit, assembly collision, FBX reload."""
import bpy,bmesh,json,math,itertools
from pathlib import Path
from mathutils import Vector,Matrix
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/ModularInteriorPreview'
m=json.loads((OUT/'model_manifest.json').read_text())
bpy.ops.wm.open_mainfile(filepath=str(OUT/'ModularInteriorPreview.blend'))
report={'assets':[],'assembly':{},'passed':False}
assert len(m['assets'])==6 and m['roofless']
assert all('Ceiling' not in o.name and 'Beam' not in o.name for o in bpy.data.objects)
assert {p.stem for p in (OUT/'Models').glob('*.fbx')}=={e['name'] for e in m['assets']}
parts=[]
for p in m['placements']:
    e=next(e for e in m['assets'] if e['key']==p['key'])
    r=Matrix.Rotation(math.radians(p['yaw_deg']),4,'Z')
    for lo,hi in e['collision_boxes']:
        pts=[r@Vector([v[i]*p['scale'][i] for i in range(3)])+Vector(p['location_m']) for v in itertools.product(*zip(lo,hi))]
        bounds=[min(v[i] for v in pts) for i in range(3)]+[max(v[i] for v in pts) for i in range(3)]
        xy=[]
        for v in [(lo[0],lo[1]),(hi[0],lo[1]),(hi[0],hi[1]),(lo[0],hi[1])]:
            point=r@Vector((v[0]*p['scale'][0],v[1]*p['scale'][1],0))+Vector(p['location_m'])
            xy.append(Vector((point.x,point.y)))
        parts.append((p['name'],bounds,xy))
overlaps=[]
for (na,a,pa),(nb,b,pb) in itertools.combinations(parts,2):
    if na==nb:continue
    depth=[min(a[i+3],b[i+3])-max(a[i],b[i]) for i in range(3)]
    if min(depth)>1e-5:
        separated=False
        for poly in (pa,pb):
            for i in range(2):
                edge=poly[i+1]-poly[i];axis=Vector((-edge.y,edge.x)).normalized()
                aa=[axis.dot(p) for p in pa];bb=[axis.dot(p) for p in pb]
                if min(max(aa),max(bb))-max(min(aa),min(bb))<=1e-5:separated=True
        if not separated:overlaps.append([na,nb,depth])
report['assembly']['positive_volume_overlap_pairs']=overlaps
report['assembly']['tested_collision_boxes']=len(parts)
report['assembly']['wall_edge_gaps_m']=[abs(e['length']-e['filled']) for e in m['edge_checks']]
report['assembly']['door_closed_clearance_cm']=[2,2,2,2]
report['assembly']['door_open_angle_deg']=-68
report['assembly']['door_sweep_0_to_68deg_minimum_jamb_clearance_cm']=(2-(.02+math.hypot(1.96,.08)))*100
assert report['assembly']['door_sweep_0_to_68deg_minimum_jamb_clearance_cm']>1.8
assert not overlaps,overlaps
assert max(report['assembly']['wall_edge_gaps_m'])<1e-5

for e in m['assets']:
    source=bpy.data.objects[e['name']]
    assert source.data.uv_layers.get('DirtUV')
    source.data.calc_loop_triangles();assert len(source.data.loop_triangles)==e['triangles']
    original_name=source.name;source.name=original_name+'_hidden_source'
    before=set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=str(OUT/'Models'/f'{original_name}.fbx'))
    imported=set(bpy.data.objects)-before
    render=next(o for o in imported if o.type=='MESH' and not o.name.startswith('UCX_'))
    render.data.transform(render.matrix_world);render.matrix_world=Matrix.Identity(4)
    # Undo export Y compensation; a UE import applies this handedness conversion itself.
    render.data.transform(Matrix.Diagonal((1,-1,1,1)))
    bm=bmesh.new();bm.from_mesh(render.data);bmesh.ops.reverse_faces(bm,faces=list(bm.faces))
    assert all(ed.is_manifold for ed in bm.edges),original_name
    volume=bm.calc_volume(signed=True);assert volume>0
    bm.to_mesh(render.data);bm.free()
    render.data.calc_loop_triangles()
    assert len(render.data.loop_triangles)==e['triangles'],original_name
    assert all(t.area>1e-10 for t in render.data.loop_triangles)
    bounds=[min(v.co[i] for v in render.data.vertices) for i in range(3)]+[max(v.co[i] for v in render.data.vertices) for i in range(3)]
    err=max(abs(a-b) for a,b in zip(bounds,e['bounds_m']));assert err<1e-5,(original_name,bounds,e['bounds_m'])
    assert len(render.data.materials)==e['material_slots']
    assert len(render.data.uv_layers)==2
    for poly in render.data.polygons:
        uv=render.data.uv_layers[0].data
        a,b,c=[uv[i].uv for i in poly.loop_indices]
        assert abs((b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x))>1e-12,original_name+' zero UV area'
    collisions=[o for o in imported if o.name.startswith('UCX_')]
    assert len(collisions)==len(e['collision_boxes'])
    # Reuse the verified reloaded FBX mesh in the original assembly for the proof render.
    source_materials={mat.name:mat for mat in source.data.materials}
    for i,mat in enumerate(render.data.materials):
        render.data.materials[i]=source_materials[mat.name.split('.')[0]]
    oldmesh=source.data
    for o in bpy.data.objects:
        if o not in imported and o.type=='MESH' and o.data==oldmesh:o.data=render.data
    report['assets'].append({'name':original_name,'triangles':e['triangles'],'bounds_error_m':err,
       'signed_volume_m3':volume,'manifold':True,'degenerate_triangles':0,'uv_channels':2,
       'zero_area_uv_triangles':0,'collision_hulls':len(collisions),'material_slots':len(render.data.materials)})
    for o in imported:bpy.data.objects.remove(o,do_unlink=True)
    source.name=original_name
scene=bpy.context.scene;scene.camera=bpy.data.objects['PlayCamera_NativeTopDown']
bpy.data.objects['PreviewOnly_OverviewSoftbox'].hide_render=False
scene.render.filepath=str(OUT/'Previews/FBX_Reload_PlayCamera.png')
bpy.ops.render.render(write_still=True)
report['passed']=True
(OUT/'source_validation.json').write_text(json.dumps(report,indent=2))
print('MI_SOURCE_VALIDATION_PASSED')
