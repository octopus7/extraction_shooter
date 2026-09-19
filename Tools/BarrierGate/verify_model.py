"""Read-only Blender checks for topology, nonoverlapping housing UVs and arm UV area."""
import bpy, json
import numpy as np
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
SOURCE=ROOT/'TunaSweeper/SourceArt/Environment/BarrierGate'
bpy.ops.wm.open_mainfile(filepath=str(SOURCE/'BarrierGate.blend'))
report={'passed':False,'meshes':[]}
for name in ['SM_BarrierHousing','SM_BarrierArm','SM_BarrierRedLens','SM_BarrierGreenLens']:
    obj=bpy.data.objects[name]
    assert obj.type=='MESH' and len(obj.data.materials)>0
    obj.data.calc_loop_triangles()
    uv=obj.data.uv_layers.active
    assert uv
    uv_area=0
    coverage=np.zeros((1024,1024),np.uint16)
    density=[]
    for tri in obj.data.loop_triangles:
        coords=np.array([uv.data[i].uv[:] for i in tri.loops],dtype=float)
        a,b,c=coords
        area=abs(np.cross(b-a,c-a))/2
        assert area>1e-12,(name,'degenerate UV',tri.index)
        uv_area+=area
        if name=='SM_BarrierHousing':
            assert coords.min()>=-1e-5 and coords.max()<=1.00001
            density.append((area/tri.area)**.5*1024)
            coords*=1024
            lo=np.maximum(np.floor(coords.min(axis=0)).astype(int),0)
            hi=np.minimum(np.ceil(coords.max(axis=0)).astype(int),1024)
            if np.any(hi<=lo):continue
            y,x=np.mgrid[lo[1]:hi[1],lo[0]:hi[0]]
            p=np.stack((x+.5,y+.5),axis=-1)
            a,b,c=coords
            den=np.cross(b-a,c-a)
            s=np.cross(p-a,c-a)/den
            t=np.cross(b-a,p-a)/den
            mask=(s>1e-5)&(t>1e-5)&(s+t<1-1e-5)
            coverage[lo[1]:hi[1],lo[0]:hi[0]]+=mask.astype(np.uint16)
    if name=='SM_BarrierHousing':
        overlap=int(np.count_nonzero(coverage>1))
        assert overlap==0,('overlapping housing UV pixels',overlap)
        assert 0<uv_area<1
        report['housing_uv']={'overlap_pixels_1024':overlap,'occupied_uv_area':float(uv_area),
            'median_texels_per_meter':float(np.median(density)),
            'density_p05':float(np.percentile(density,5)),'density_p95':float(np.percentile(density,95))}
    report['meshes'].append({'name':name,'triangles':len(obj.data.loop_triangles),'uv_area':float(uv_area)})
arm=bpy.data.objects['SM_BarrierArm']
assert abs(arm.dimensions.x-3.5)<.0001
assert abs(min(v.co.x for v in arm.data.vertices))<.0001
for name, x in [('SM_BarrierRedLens',-.09),('SM_BarrierGreenLens',.09)]:
    lens=bpy.data.objects[name]
    assert lens.location.length<.00001, 'Lens export origin must be the cabinet origin'
    center=np.mean(np.array([v.co[:] for v in lens.data.vertices]),axis=0)
    assert np.linalg.norm(center-np.array([x,-.268,.79]))<.0001, (name,'lens mounting alignment')
report['passed']=True
target=ROOT/'TunaSweeper/Saved/Automation/BarrierGate/model.json'
target.parent.mkdir(parents=True,exist_ok=True)
target.write_text(json.dumps(report,indent=2),encoding='utf-8')
print('BARRIER_MODEL_VERIFY_PASSED '+json.dumps(report))
