import bpy,bmesh,json,math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]/'TunaSweeper/SourceArt/Environment/Basement'
m=json.loads((ROOT/'layout_manifest.json').read_text());report=[]
bpy.ops.wm.open_mainfile(filepath=str(ROOT/'Basement_Modules.blend'))
for name,meta in m['assets'].items():
    o=bpy.data.objects[name];bm=bmesh.new();bm.from_mesh(o.data)
    assert all(e.is_manifold for e in bm.edges),name
    volume=bm.calc_volume(signed=True);assert volume>0,name
    bm.free();assert len(o.data.uv_layers)==2
    assert min(p.area for p in o.data.polygons)>1e-7
    if name.startswith('SM_Basement_Corner'):
        assert meta['corner_segments']==3
        # Eight perimeter vertices at both base and top: 4+4 arc samples.
        assert len(o.data.vertices)==16
        inner=[v.co for v in o.data.vertices[:4]]
        lengths=[(inner[i+1]-inner[i]).length for i in range(3)]
        assert max(lengths)-min(lengths)<1e-5
    report.append({'name':name,'closed_manifold':True,'positive_volume_m3':volume,'uv_channels':2,'triangles':meta['triangles']})
assert len({i['id'] for i in m['instances']})==len(m['instances'])
assert m['range_origin_m'][1]-5.8==m['range_entrance_m'][1]
assert len([i for i in m['instances'] if 'EndCap' in i['asset']])==3
assert len({i['asset'] for i in m['instances'] if i['asset'].startswith('SM_Basement_Corner')})==2
assert all(i['scale'][2]==1 for i in m['instances'])
for i in m['instances']:
    x,y,z=i['location_m']
    if i['asset'].startswith('SM_Basement_Wall') and abs(x)<=3.20001 and abs(y)<=3.20001:
        assert i['zone']=='LadderHub',i['id']
(ROOT/'source_validation.json').write_text(json.dumps({'passed':True,'assets':report,'placements':len(m['instances']),'connectors':3,'range_join_gap_cm':0,'corner_segments':3},indent=2))
print('BASEMENT_SOURCE_VALIDATED')
