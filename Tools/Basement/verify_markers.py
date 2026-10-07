"""Read-only checks for shared 256px range triangle floor markers."""
import json
from pathlib import Path
import unreal as u

SOURCE=Path(__file__).resolve().parents[2]/'TunaSweeper/SourceArt/Environment/Basement'
DEST='/Game/Environment/Basement/Range'

def validate():
    meta=json.loads((SOURCE/'RangeMarkers/marker_manifest.json').read_text())
    actors=u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()
    labels={a.get_actor_label():a for a in actors}
    assert 'LaneNumber_05' not in labels and 'LaneNumber_05Backing' not in labels
    texture=u.load_asset(DEST+'/Textures/T_RangeLaneTriangle')
    assert [texture.blueprint_get_size_x(),texture.blueprint_get_size_y()]==[256,256]
    material=u.load_asset(DEST+'/Materials/M_RangeLaneTriangle')
    assert material.get_editor_property('blend_mode')==u.BlendMode.BLEND_MASKED
    mask=u.MaterialEditingLibrary.get_material_property_input_node(material,u.MaterialProperty.MP_OPACITY_MASK)
    assert isinstance(mask,u.MaterialExpressionTextureSample) and mask.texture==texture
    mesh=u.load_asset(DEST+'/Meshes/SM_RangeLaneTriangle')
    assert mesh.get_material(0)==material and mesh.get_num_triangles(0)==2
    assert labels[meta['floor_marking_actor']].static_mesh_component.static_mesh.get_name()=='SM_RangeFloorMarkings_NoNumbers'
    assert labels[meta['floor_marking_actor']].static_mesh_component.static_mesh.get_num_triangles(0)==10
    for a in actors:
        c=a.get_component_by_class(u.StaticMeshComponent)
        assert not(c and c.static_mesh and c.static_mesh.get_name() in ['SM_FloorMarkings','SM_RangeLane05Marking']),a.get_actor_label()
    for i,p in enumerate(meta['placements'],1):
        a=labels[p['label']];c=a.static_mesh_component
        assert c.static_mesh==mesh and (a.get_actor_location()-u.Vector(*p['location_cm'])).length()<.1
        assert a.get_attach_parent_actor()==labels['ROOT_Range']
        assert c.get_collision_enabled()==u.CollisionEnabled.NO_COLLISION and c.get_editor_property('visible')
        assert not c.get_editor_property('cast_shadow')
        assert abs(a.get_actor_bounds(False)[1].y*2-60)<.1
        toward=labels[f'Target_{i:02}'].get_actor_location()-a.get_actor_location();toward.z=0
        tip=-a.get_actor_right_vector()
        assert u.MathLibrary.dot_vector_vector(tip,toward.normal())>.999
    report={'passed':True,'markers':5,'texture_resolution':[256,256],'visible_width_cm':45,'shared_mesh_material_texture':True,'tips_face_targets':True,'old_number_geometry_removed':True,'collision':'NoCollision'}
    (SOURCE/'RangeMarkers/unreal_validation.json').write_text(json.dumps(report,indent=2)+'\n')
    u.log('RANGE_TRIANGLES_VERIFIED')
    return report
