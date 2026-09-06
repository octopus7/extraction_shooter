"""Reload persisted asset and render it in a transient world, without saving a map.
Run with UE Python commandlet, -AllowCommandletRendering and a hardware RHI.
"""
import unreal, json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/ForestProps/GrassLongCurved'
DEST='/Game/Nature/ForestProps/GrassLongCurved'
world=unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
mesh=unreal.load_asset(DEST+'/SM_GrassLongCurved');assert mesh
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem) or unreal.get_default_object(unreal.EditorActorSubsystem)
def spawn_mesh(mesh,location,scale=(1,1,1)):
    actor=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*location))
    component=actor.static_mesh_component;component.set_static_mesh(mesh);component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    actor.set_actor_scale3d(unreal.Vector(*scale));return actor
plant=spawn_mesh(mesh,(0,0,0))
ground=spawn_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'),(0,0,-3),(20,20,.05))
for rotation,intensity in [((-48,-35,0),3.0),((-35,135,0),1.2)]:
    light=actors.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,300),unreal.Rotator(*rotation))
    light.light_component.set_mobility(unreal.ComponentMobility.MOVABLE);light.light_component.set_intensity(intensity)
capture=actors.spawn_actor_from_class(unreal.SceneCapture2D,unreal.Vector(170,-240,175))
component=capture.get_component_by_class(unreal.SceneCaptureComponent2D)
component.set_editor_property('projection_type',unreal.CameraProjectionMode.ORTHOGRAPHIC)
component.set_editor_property('ortho_width',170.0)
component.set_editor_property('capture_source',unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
component.set_editor_property('capture_every_frame',False)
component.set_editor_property('capture_on_movement',False)
settings=component.get_editor_property('post_process_settings')
for key,val in {'override_auto_exposure_method':True,'auto_exposure_method':unreal.AutoExposureMethod.AEM_MANUAL,'override_auto_exposure_bias':True,'auto_exposure_bias':0.0,'override_auto_exposure_apply_physical_camera_exposure':True,'auto_exposure_apply_physical_camera_exposure':False}.items():settings.set_editor_property(key,val)
component.set_editor_property('post_process_settings',settings)
target=unreal.RenderingLibrary.create_render_target2d(world,1024,1024,unreal.TextureRenderTargetFormat.RTF_RGBA8)
component.set_editor_property('texture_target',target)
views=[('UE_Hero',(170,-240,175),(0,0,38),170),('UE_Back',(-180,240,160),(0,0,38),170)]
for name,location,lookat,width in views:
    capture.set_actor_location(unreal.Vector(*location),False,False)
    capture.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(unreal.Vector(*location),unreal.Vector(*lookat)),False)
    component.set_editor_property('ortho_width',float(width))
    component.capture_scene()
    unreal.RenderingLibrary.export_render_target(world,target,str(OUT/'Previews'),name+'.png')
    assert (OUT/'Previews'/(name+'.png')).exists()
(OUT/'unreal_render_validation.json').write_text(json.dumps({'mesh':mesh.get_path_name(),'engine':unreal.SystemLibrary.get_engine_version(),'views':[v[0] for v in views],'world':'transient unsaved blank map','saved_existing_map':False,'rhi':'hardware rendering; exported SceneCapture2D','passed':True},indent=2),encoding='utf-8')
unreal.log('GRASS_LONG_RENDER_PASSED')
