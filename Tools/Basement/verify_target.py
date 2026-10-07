"""Read-only range target Blueprint asset verification; no asset generation."""
import unreal as u,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
SOURCE=ROOT/'TunaSweeper/SourceArt/Environment/Basement'
BP_PATH='/Game/Environment/Basement/Range/Blueprints/BP_RangePracticeTarget'

def verify_asset():
    bp=u.load_asset(BP_PATH)
    assert isinstance(bp,u.Blueprint),'Missing BP_RangePracticeTarget'
    cls=bp.generated_class();cdo=u.get_default_object(cls)
    parent=u.load_class(None,'/Script/TunaSweeper.TunaSweeperShootingPracticeDummyActor')
    assert isinstance(cdo,u.TunaSweeperShootingPracticeDummyActor)
    entries={}
    spec=json.loads((SOURCE/'RangeTarget/mesh_manifest.json').read_text())
    for prop,suffix in [('body_mesh','Body'),('head_mesh','Head'),('headshot_plate_mesh','Plate')]:
        c=cdo.get_editor_property(prop)
        assert c.static_mesh.get_name()=='SM_RangePracticeTarget_'+suffix,(prop,c.static_mesh)
        assert c.get_editor_property('relative_location').length()<.001,prop
        assert (c.get_editor_property('relative_scale3d')-u.Vector(1,1,1)).length()<.001,prop
        assert c.get_editor_property('visible') and not c.get_editor_property('hidden_in_game')
        assert c.get_collision_enabled()==u.CollisionEnabled.QUERY_AND_PHYSICS
        assert c.static_mesh.get_num_triangles(0)==spec['parts'][suffix]['triangles']
        assert c.static_mesh.get_editor_property('body_setup').get_editor_property('collision_trace_flag')==u.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE
        assert abs(c.static_mesh.get_bounding_box().max.z-spec['parts'][suffix]['bounds_m'][1][2]*100)<.1,'Unexpected head/body height'
        expected={'Body':['MI_Range_Robot'],'Head':['M_Range_SensorGlass','M_Range_SensorLight','MI_Range_Robot'],'Plate':['M_Range_SensorHousing']}[suffix]
        assert [c.get_material(i).get_name() for i in range(c.get_num_materials())]==expected
        entries[prop]=c.static_mesh.get_path_name()
    assert cdo.get_editor_property('max_health')==100
    assert cdo.get_editor_property('minimum_health')==1
    assert cdo.get_editor_property('health_recovery_seconds')==2
    w=cdo.get_editor_property('health_bar_widget_component')
    assert w.get_editor_property('widget_class')
    assert w.get_editor_property('relative_location').z==180
    ears=spec['ears'];assert ears['count']==2 and abs(ears['height_m']/ears['original_height_m']-.5)<.0001
    return {'blueprint':BP_PATH,'parent':parent.get_path_name(),'components':entries,'health':100,'minimum_health':1,'recovery_seconds':2,'ear_height_cm':ears['height_m']*100,'ear_height_scale':.5,'health_bar_height_cm':180}

if __name__=='__main__':
    result=verify_asset();result['passed']=True
    (SOURCE/'target_asset_validation.json').write_text(json.dumps(result,indent=2))
    u.log('RANGE_TARGET_ASSET_VERIFIED')
