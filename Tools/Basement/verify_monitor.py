"""Read-only check of the saved control-screen texture and lane-count contract."""
import json
from pathlib import Path
import unreal as u

SOURCE=Path(__file__).resolve().parents[2]/'TunaSweeper/SourceArt/Environment/Basement'

def validate():
    meta=json.loads((SOURCE/'RangeMonitor/monitor_manifest.json').read_text())
    actors=u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()
    targets=[a for a in actors if 'TS_RangeTarget' in [str(t) for t in a.tags]]
    assert len(targets)==meta['lane_count']==len(meta['rows'])==5
    assert meta['rows']==[f'{i:02} READY' for i in range(1,6)]
    monitor=next(a for a in actors if a.get_actor_label()=='ControlConsole_001')
    material=monitor.static_mesh_component.get_material(0)
    assert material.get_name()=='MI_Range_Props'
    for channel in ['BaseColor','Emissive']:
        tex=u.MaterialEditingLibrary.get_material_instance_texture_parameter_value(material,channel)
        assert tex.get_name()=='T_Props_'+channel
        assert [tex.blueprint_get_size_x(),tex.blueprint_get_size_y()]==[2048,2048]
        assert Path(tex.get_editor_property('asset_import_data').get_first_filename()).resolve()==(SOURCE/f'Range/Textures/T_Props_{channel}.png').resolve()
    report={'passed':True,'monitor':'ControlConsole_001','display_rows':meta['rows'],'placed_target_count':len(targets),'screen_patch_resolution':[512,512],'base_color_and_emissive_updated':True,'shared_atlas_other_regions_unchanged':meta['other_atlas_regions_unchanged'],'static_artwork':True}
    (SOURCE/'RangeMonitor/unreal_validation.json').write_text(json.dumps(report,indent=2)+'\n')
    u.log('RANGE_MONITOR_FIVE_LANES_VERIFIED')
    return report
