"""Read-only source UCX aperture and sampled walkway checks for MoleControlV2.

Only layout_validation.json is written. Checks are conservative AABB clearance,
not a runtime climbing, physics or navigation test. The arched door is deliberately
closed in this variant, so no path through that door is declared passable.
"""
from pathlib import Path
from itertools import product
import hashlib
import json
import math

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'TunaSweeper/SourceArt/Environment/MoleControlV2'
LEGACY_SOURCE = ROOT / 'TunaSweeper/SourceArt/Environment/FacilityRooms'
CAPSULE_RADIUS_CM = 35.0
SAMPLE_INTERVAL_CM = 10.0
BODY_Z_RANGE_CM = (35.0, 180.0)
EPSILON = .01


def load_assets():
    merged = SOURCE / 'model_manifest.json'
    paths = [merged] if merged.exists() else [SOURCE/'Manifests'/f'{n}.json' for n in ('Shell','Console','Decor')]
    assets = {}; inputs = []
    for path in paths:
        if not path.is_file():
            continue
        data = json.loads(path.read_text(encoding='utf-8'))
        rows = data.get('assets', [])
        assert all('collision_specs_cm' in row for row in rows), str(path)
        inputs.append(path)
        for row in rows:
            assert row['name'] not in assets, ('Duplicate mesh', row['name'])
            assets[row['name']] = row
    assert assets, 'No new model manifests available'
    legacy = LEGACY_SOURCE / 'model_manifest.json'
    if not legacy.exists():
        legacy = LEGACY_SOURCE / 'Manifests/Architecture.json'
    data = json.loads(legacy.read_text(encoding='utf-8'))
    ladder = next(row for row in data['assets'] if row['name'] == 'SM_FacilityRooms_Ladder300')
    assets[ladder['name']] = ladder; inputs.append(legacy)
    return assets, inputs


def digest(paths):
    return {p.relative_to(ROOT).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in sorted(set(paths))}


def local_bounds(spec):
    return [spec['center'][i]-spec['size'][i]/2 for i in range(3)] + [
        spec['center'][i]+spec['size'][i]/2 for i in range(3)]


def world_bounds(spec, placement):
    angle = math.radians(placement.get('yaw_deg',0)); cosine,sine = math.cos(angle),math.sin(angle)
    translation = placement['location_cm']; scale = placement.get('scale',[1,1,1]); corners=[]
    for signs in product((-1,1),repeat=3):
        p=[(spec['center'][i]+signs[i]*spec['size'][i]/2)*scale[i] for i in range(3)]
        x,y,z=p[0],-p[1],p[2]
        corners.append([translation[0]+cosine*x-sine*y, translation[1]+sine*x+cosine*y, translation[2]+z])
    return [min(p[i] for p in corners) for i in range(3)] + [max(p[i] for p in corners) for i in range(3)]


def central_gap(specs, axis, other_coordinates):
    intervals=[]
    for spec in specs:
        bounds=local_bounds(spec)
        if all(bounds[a]-EPSILON<=value<=bounds[a+3]+EPSILON for a,value in other_coordinates.items()):
            intervals.append((bounds[axis],bounds[axis+3]))
    if any(low+EPSILON<0<high-EPSILON for low,high in intervals):
        return 0.0
    left=[high for low,high in intervals if high<=EPSILON]
    right=[low for low,high in intervals if low>=-EPSILON]
    assert left and right, ('Missing opening sides',axis,intervals)
    return min(right)-max(left)


def verify_openings(assets):
    results=[]
    hatch=assets['SM_MoleControlV2_OakHatch200']
    width=central_gap(hatch['collision_specs_cm'],0,{1:0,2:10})
    depth=central_gap(hatch['collision_specs_cm'],1,{0:0,2:10})
    results.append({'name':hatch['name'],'expected_width_depth_cm':[90,90],
                    'collision_width_depth_cm':[width,depth],
                    'passed':abs(width-90)<.1 and abs(depth-90)<.1})
    window=assets['SM_MoleControlV2_StoneWindowWall200']
    shell=[];muntins=[];blockers=[]
    for index,spec in enumerate(window['collision_specs_cm']):
        bounds=local_bounds(spec)
        ox=min(bounds[3],50)-max(bounds[0],-50)
        oz=min(bounds[5],210)-max(bounds[2],100)
        if ox>.1 and oz>.1:
            vertical=(spec['size'][0]<=8 and abs(spec['center'][0])<=4
                      and bounds[2]>=99.9 and bounds[5]<=210.1)
            horizontal=(spec['size'][2]<=8 and abs(spec['center'][2]-155)<=4
                        and bounds[0]>=-50.1 and bounds[3]<=50.1)
            (muntins if vertical or horizontal else blockers).append({'index':index,'bounds_cm':bounds})
        else:
            shell.append(spec)
    width=central_gap(shell,0,{1:0,2:127})
    crossing=[local_bounds(s) for s in shell if local_bounds(s)[0]<=20<=local_bounds(s)[3]
              and local_bounds(s)[1]<=0<=local_bounds(s)[4]]
    bottom=max(b[5] for b in crossing if b[5]<155)
    top=min(b[2] for b in crossing if b[2]>155)
    results.append({'name':window['name'],'expected_width_height_cm':[100,110],
                    'expected_z_range_cm':[100,210],'collision_width_height_cm':[width,top-bottom],
                    'collision_z_range_cm':[bottom,top],'allowed_muntins':muntins,
                    'interior_blocking_boxes':blockers,
                    'passed':not blockers and abs(width-100)<.1 and abs(bottom-100)<.1 and abs(top-210)<.1})
    return results


def circle_aabb_distance(point,bounds):
    nearest=[min(max(point[i],bounds[i]),bounds[i+3]) for i in (0,1)]
    return math.hypot(point[0]-nearest[0],point[1]-nearest[1])


def check_level(level,assets):
    colliders=[]
    placements=level['placements']
    assert len({p['label'] for p in placements})==len(placements), 'Duplicate placement labels'
    for placement in placements:
        name=placement['mesh']
        assert name in assets,('Missing placement mesh',name)
        for index,spec in enumerate(assets[name]['collision_specs_cm']):
            colliders.append({'label':placement['label'],'mesh':name,'collision_index':index,
                              'bounds_cm':world_bounds(spec,placement)})
    z0,z1=[level['floor_z_cm']+z for z in BODY_Z_RANGE_CM]
    body=[b for b in colliders if b['bounds_cm'][2]<z1-EPSILON and b['bounds_cm'][5]>z0+EPSILON]
    segments=[]
    checks=level.get('walkway_checks',level.get('walkways',[]))
    assert checks,('No walkway checks',level['name'])
    for index,segment in enumerate(checks):
        x0,y0,x1,y1=segment
        steps=max(1,math.ceil(math.hypot(x1-x0,y1-y0)/SAMPLE_INTERVAL_CM));hits={};minimum=None
        for step in range(steps+1):
            point=[x0+(x1-x0)*step/steps,y0+(y1-y0)*step/steps]
            for collider in body:
                clearance=circle_aabb_distance(point,collider['bounds_cm'])-CAPSULE_RADIUS_CM
                minimum=clearance if minimum is None else min(minimum,clearance)
                if clearance < -EPSILON:
                    key=(collider['label'],collider['collision_index'])
                    if key not in hits:
                        hits[key]=dict(collider,first_blocked_point_cm=point,last_blocked_point_cm=point,
                                       blocked_samples=0,minimum_clearance_cm=clearance)
                    hit=hits[key];hit['blocked_samples']+=1;hit['last_blocked_point_cm']=point
                    hit['minimum_clearance_cm']=min(hit['minimum_clearance_cm'],clearance)
        segments.append({'index':index,'segment_cm':segment,'samples':steps+1,'minimum_clearance_cm':minimum,
                         'blocked_colliders':list(hits.values()),'passed':not hits})
    return {'name':level['name'],'placement_count':len(placements),'collision_box_count':len(colliders),
            'body_z_range_cm':[z0,z1],'walkway_checks':segments,
            'passed':all(s['passed'] for s in segments)}


def main():
    assets,paths=load_assets();layout_path=SOURCE/'level_layout.json'
    layout=json.loads(layout_path.read_text(encoding='utf-8'));paths.append(layout_path);before=digest(paths)
    report={'verification':'source UCX AABB transformed into UE; conservative sampled capsule clearance',
            'capsule_radius_cm':CAPSULE_RADIUS_CM,'sample_interval_cm':SAMPLE_INTERVAL_CM,
            'body_height_above_floor_cm':list(BODY_Z_RANGE_CM),
            'axis_contract':'Blender X/-Y/Z -> UE X/+Y/Z, then yaw/location',
            'closed_arched_door_note':'No traversal through the intentionally closed door is asserted',
            'opening_checks':[],'levels':[],'errors':[],'passed':False}
    try:
        report['opening_checks']=verify_openings(assets)
    except Exception as error:
        report['errors'].append({'check':'openings','error':str(error)})
    for level in layout['levels']:
        try:
            report['levels'].append(check_level(level,assets))
        except Exception as error:
            report['errors'].append({'check':level['name'],'error':str(error)})
    after=digest(paths);report['source_hashes_before']=before;report['source_hashes_after']=after
    report['source_files_unchanged']=before==after
    report['passed']=(not report['errors'] and before==after
                      and all(o['passed'] for o in report['opening_checks'])
                      and all(level['passed'] for level in report['levels']))
    (SOURCE/'layout_validation.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print('MOLE_V2_LAYOUT_VALIDATION',json.dumps({'passed':report['passed'],'errors':report['errors'],
          'openings':report['opening_checks'],'levels':[{'name':level['name'],'passed':level['passed'],
          'blocked_segments':[{'index':s['index'],'colliders':sorted({b['label'] for b in s['blocked_colliders']})}
                              for s in level['walkway_checks'] if not s['passed']]} for level in report['levels']]}))
    assert report['passed'],'MoleControlV2 layout validation failed; see layout_validation.json'


if __name__=='__main__':
    main()
