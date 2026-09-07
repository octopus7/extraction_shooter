from pathlib import Path
import unreal, json, traceback
OUT=Path(__file__).resolve().parents[2]/'TunaSweeper/SourceArt/Environment/LootContainerSet'
def vec(v): return [v.x,v.y,v.z]
def rot(v): return [v.pitch,v.yaw,v.roll]
def obj(v): return v.get_path_name() if v else None
try:
    cls=unreal.EditorAssetLibrary.load_blueprint_class('/Game/Interaction/BP_LootContainer')
    actor=unreal.get_editor_subsystem(unreal.EditorActorSubsystem).spawn_actor_from_class(cls,unreal.Vector(0,0,-10000))
    d={}
    for k in ['body_mesh_override','lid_mesh_override','lid_pivot_relative_location','closed_lid_relative_rotation','open_lid_relative_rotation','open_animation_duration','close_animation_duration','container_definition_id']:
        v=actor.get_editor_property(k)
        d[k]=vec(v) if isinstance(v,unreal.Vector) else rot(v) if isinstance(v,unreal.Rotator) else obj(v) if isinstance(v,unreal.Object) else v
    d['components']=[]
    for c in actor.get_components_by_class(unreal.SceneComponent):
        e={'name':c.get_name(),'location':vec(c.get_editor_property('relative_location')),'scale':vec(c.get_editor_property('relative_scale3d')),'rotation':rot(c.get_editor_property('relative_rotation'))}
        if isinstance(c,unreal.StaticMeshComponent):e.update(mesh=obj(c.static_mesh),materials=[obj(m) for m in c.get_materials()],collision=str(c.get_collision_enabled()))
        d['components'].append(e)
    unreal.get_editor_subsystem(unreal.EditorActorSubsystem).destroy_actor(actor)
    (OUT/'existing_bp_audit.json').write_text(json.dumps(d,indent=2))
except Exception: unreal.log_error(traceback.format_exc())
unreal.SystemLibrary.quit_editor()
