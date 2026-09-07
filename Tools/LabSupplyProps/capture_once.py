"""One-off UE screenshot driver. Deleted immediately after verified asset commit."""
from pathlib import Path
import unreal,json,traceback
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'TunaSweeper/SourceArt/Environment/LabSupplyProps'
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert levels.load_level('/Game/Environment/LabSupplyProps/Maps/L_LabSupplyProps')
bylabel={a.get_actor_label():a for a in actors.get_all_level_actors()}
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
for command in ('r.Shadow.Virtual.SMRT.RayCountLocal 16','r.Shadow.Virtual.SMRT.SamplesPerRayLocal 16'):
    unreal.SystemLibrary.execute_console_command(world,command)
names=['LSP_Lab_GameCamera','LSP_Warehouse_GameCamera','LSP_Lab_ObliqueCamera','LSP_Warehouse_ObliqueCamera']
queue=list(names);task=None;ticks=0;busy=False;report={'passed':False,'captures':[]}
def tick(dt):
    global ticks,busy,task
    if busy:return
    ticks+=1
    if ticks<90:return
    busy=True
    try:
        assert ticks<2400,'capture timeout'
        if task and not task.is_task_done():return
        if queue:
            name=queue.pop(0)
            group='Warehouse' if 'Warehouse' in name else 'Lab'
            for label,actor in bylabel.items():
                if label.startswith('LSP_') and not isinstance(actor,unreal.CameraActor):actor.set_is_temporarily_hidden_in_editor(not label.startswith('LSP_'+group+'_'))
            task=unreal.AutomationLibrary.take_high_res_screenshot(1600,1100,str(OUT/'Previews'/('UE_'+name+'.png')),camera=bylabel[name],delay=5.0)
            assert task;report['captures'].append(name)
        else:
            assert all((OUT/'Previews'/('UE_'+name+'.png')).is_file() for name in names)
            report['passed']=True;unreal.unregister_slate_post_tick_callback(handle);(OUT/'unreal_capture_validation.json').write_text(json.dumps(report,indent=2));unreal.SystemLibrary.quit_editor()
    except Exception:
        report['error']=traceback.format_exc();unreal.log_error(report['error']);unreal.unregister_slate_post_tick_callback(handle);(OUT/'unreal_capture_validation.json').write_text(json.dumps(report,indent=2));unreal.SystemLibrary.quit_editor()
    finally:busy=False
handle=unreal.register_slate_post_tick_callback(tick)
