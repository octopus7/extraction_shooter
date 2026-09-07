"""Check measured bounds and prove full-arc body/lid separation, without UE writes."""
from pathlib import Path
import json
OUT=Path(__file__).resolve().parents[2]/'TunaSweeper/SourceArt/Environment/LootContainerSet'
m=json.loads((OUT/'model_manifest.json').read_text())
report={'passed':False,'assemblies':[],'proof':'In hinge coordinates every lid vertex has y>=0 and z>=0; every body vertex has y>=0 and z<0. For physical +X angle 0..90 degrees lid z stays >=0. For 90..105 degrees, any lid point with z<0 has y<0, behind the body. Thus their interiors cannot intersect anywhere in the continuous requested arc.'}
for s in m['sets']:
    b=next(e for e in m['assets'] if e['name']=='SM_LC_'+s['key']+'_Body')['bounds_m']
    l=next(e for e in m['assets'] if e['name']=='SM_LC_'+s['key']+'_Lid')['bounds_m']
    hinge=s['hinge_m'];expected=s['dimensions_m']
    assert l[1]>=-1e-6 and l[2]>=-1e-6
    assert b[1]>=hinge[1]-1e-6 and b[5]<hinge[2]-.0005
    closed=[min(b[i],l[i]+hinge[i]) for i in range(3)]+[max(b[i+3],l[i+3]+hinge[i]) for i in range(3)]
    dims=[closed[i+3]-closed[i] for i in range(3)]
    assert max(abs(a-c) for a,c in zip(dims,expected))<1e-6
    assert abs(closed[2])<1e-6 and abs(closed[0]+closed[3])<1e-6 and abs(closed[1]+closed[4])<1e-6
    report['assemblies'].append({'kind':s['key'],'closed_dimensions_m':dims,'body_to_lid_vertical_gap_mm':(hinge[2]-b[5])*1000,'continuous_clearance_roll_degrees':[0,-105],'passed':True})
report['passed']=True
(OUT/'assembly_validation.json').write_text(json.dumps(report,indent=2))
print('LOOT_ASSEMBLY_VALIDATION_PASSED')
