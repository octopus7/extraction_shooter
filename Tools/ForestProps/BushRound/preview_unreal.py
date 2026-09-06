"""Reload uassets in UE 5.7 with an RHI; capture an unsaved review world."""
import unreal,time,json,traceback
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/ForestProps/BushRound'
world=unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
mesh=unreal.load_asset('/Game/Nature/ForestProps/BushRound/SM_BushRound')
assert mesh
material=unreal.load_asset('/Game/Nature/ForestProps/BushRound/M_BushRound')
unreal.MaterialEditingLibrary.recompile_material(material)
bush=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(0,0,0))
bush.static_mesh_component.set_static_mesh(mesh)
bush.static_mesh_component.set_material(0,material)
bush.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
floor=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(0,0,-5))
floor.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'))
floor.static_mesh_component.set_material(0,unreal.load_asset('/Engine/BasicShapes/BasicShapeMaterial'))
floor.set_actor_scale3d(unreal.Vector(200,200,.1))
light=actors.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,200),unreal.Rotator(-48,-35,0))
light.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
light.light_component.set_editor_property('intensity',8.0)
fill=actors.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,200),unreal.Rotator(-35,145,0))
fill.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
fill.light_component.set_editor_property('intensity',3.0)
fill.light_component.set_editor_property('cast_shadows',False)
sky=actors.spawn_actor_from_class(unreal.SkyLight,unreal.Vector(0,0,200))
sky.light_component.set_editor_property('intensity',.8)
capture=actors.spawn_actor_from_class(unreal.SceneCapture2D,unreal.Vector(190,-260,190))
capture.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(capture.get_actor_location(),unreal.Vector(0,0,33)),False)
cap=capture.get_component_by_class(unreal.SceneCaptureComponent2D)
rt=unreal.RenderingLibrary.create_render_target2d(world,1200,1000,unreal.TextureRenderTargetFormat.RTF_RGBA8)
cap.set_editor_property('texture_target',rt)
cap.set_editor_property('projection_type',unreal.CameraProjectionMode.ORTHOGRAPHIC)
cap.set_editor_property('ortho_width',205)
cap.set_editor_property('capture_source',unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
cap.set_editor_property('capture_every_frame',True)
pp=cap.get_editor_property('post_process_settings')
pp.set_editor_property('override_auto_exposure_method',True);pp.set_editor_property('auto_exposure_method',unreal.AutoExposureMethod.AEM_MANUAL)
pp.set_editor_property('override_auto_exposure_bias',True);pp.set_editor_property('auto_exposure_bias',1.8)
pp.set_editor_property('override_auto_exposure_apply_physical_camera_exposure',True);pp.set_editor_property('auto_exposure_apply_physical_camera_exposure',False)
cap.set_editor_property('post_process_settings',pp)
start=time.monotonic();stage=0
def tick(dt):
    global stage
    try:
        elapsed=time.monotonic()-start
        if stage==0 and elapsed>45:
            stage=1
        elif stage==1 and elapsed>50:
            unreal.RenderingLibrary.export_render_target(world,rt,str(OUT/'Previews'),'BushRound_UE_Reload.png')
            assert (OUT/'Previews/BushRound_UE_Reload.png').exists()
            (OUT/'unreal_display_validation.json').write_text(json.dumps({'engine':unreal.SystemLibrary.get_engine_version(),'mesh':mesh.get_path_name(),'material':material.get_path_name(),'used_textures':[t.get_path_name() for t in unreal.MaterialEditingLibrary.get_used_textures(material)],'preview':'Previews/BushRound_UE_Reload.png','world_saved':False,'real_rhi_capture':True},indent=2))
            unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.quit_editor()
    except Exception:
        (OUT/'unreal_display_error.txt').write_text(traceback.format_exc())
        unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.quit_editor()
handle=unreal.register_slate_post_tick_callback(tick)
