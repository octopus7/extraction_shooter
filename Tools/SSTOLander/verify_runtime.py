"""Read-only PIE test of saved-map ladder transfers and the existing dissolve tick."""
import hashlib
import json
from pathlib import Path
import time
import traceback
import unreal

ROOT=Path(__file__).resolve().parents[2]
REPORT=ROOT/'TunaSweeper/Saved/SSTOLander_20261003/runtime.json'
def packages():
    folder=ROOT/'TunaSweeper/Content/MainRaid'
    return {str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in list(folder.glob('*.umap'))+list((folder/'SSTO').rglob('*.uasset'))}
def array(v):return [v.x,v.y,v.z]

class Runtime:
    def __init__(self):
        self.before=packages();self.started=time.monotonic();self.state='load';self.frames=0;self.busy=False
        self.report={'passed':False,'round_trips':0,'native_reveal_tick':False}
        self.levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
        self.editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
        unreal.EditorPythonScripting.set_keep_python_script_alive(True)
        self.handle=unreal.register_slate_post_tick_callback(self.tick)
    def transition(self,state):self.state=state;self.frames=0
    def tick(self,_dt):
        if self.busy:return
        self.busy=True;self.frames+=1
        try:
            assert time.monotonic()-self.started<240,'Runtime validation timed out'
            if self.state=='load':
                assert self.levels.load_level('/Game/MainRaid/RaidPlains')
                self.transition('settle')
            elif self.state=='settle' and self.frames>=30:
                self.levels.editor_play_simulate();self.transition('pie')
            elif self.state=='pie' and self.frames>=30:
                world=self.editor.get_game_world();assert world
                self.world=world
                self.player=unreal.GameplayStatics.get_player_pawn(world,0);assert self.player,'PIE player missing'
                endpoints=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.load_class(None,'/Script/TunaSweeper.TunaSweeperLadderTransferActor'))
                assert len(endpoints)==2
                self.entry=next(a for a in endpoints if a.get_actor_label()=='TS_Plains_SSTO_Entry')
                self.exit=next(a for a in endpoints if a.get_actor_label()=='TS_Plains_SSTO_Exit')
                # Simulate-in-editor starts with a spectator. Possess a real gameplay character.
                controller=unreal.GameplayStatics.get_player_controller(world,0)
                assert controller
                floor=self.entry.get_arrival_point().get_world_location()
                cls=unreal.load_class(None,'/Script/TunaSweeper.TunaSweeperTopDownCharacter')
                controller.call_method('EnableCheats')
                unreal.SystemLibrary.execute_console_command(world,'summon /Script/TunaSweeper.TunaSweeperTopDownCharacter',controller)
                characters=unreal.GameplayStatics.get_all_actors_of_class(world,cls)
                assert characters,'Gameplay character summon failed'
                self.player=characters[-1]
                assert unreal.TunaSweeperSimulationTestLibrary.configure_simulation_player(controller,self.player),'Simulation player setup failed'
                half=self.player.get_editor_property('capsule_component').get_scaled_capsule_half_height()
                floor=self.entry.get_arrival_point().get_world_location()
                assert self.player.set_actor_location(floor+unreal.Vector(0,0,half+2),False,False)
                self.report['player_debug']={'class':self.player.get_class().get_path_name(),'location':array(self.player.get_actor_location()),'entry_floor':array(floor),'capsule_half_height':half,'capsule_radius':self.player.get_editor_property('capsule_component').get_scaled_capsule_radius(),'controller_pawn_matches':unreal.GameplayStatics.get_player_pawn(world,0)==self.player,'spectator_only':controller.get_editor_property('player_state').is_only_a_spectator(),'dead':self.player.is_dead(),'mounted':self.player.is_mounted_in_vehicle()}
                assert self.entry.can_transfer_player(self.player),'Saved entry destination is blocked'
                assert self.entry.request_interaction(self.player),'Actual interaction entry failed'
                self.report['entered_location_cm']=array(self.player.get_actor_location())
                self.transition('inside')
            elif self.state=='inside' and self.frames>=12:
                ship=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(self.world,unreal.Actor) if a.get_actor_label()=='TS_Plains_SSTOLander')
                shell=ship.get_components_by_class(unreal.ChildActorComponent)[0].get_editor_property('child_actor')
                settings=unreal.load_asset('/Game/Effects/DA_OcclusionRevealSettings')
                expected_start=self.player.get_actor_location().z+settings.get_editor_property('vertical_reveal_start_above_player_cm')
                expected_fade=settings.get_editor_property('vertical_reveal_fade_height_cm')
                samples=[]
                for mesh in shell.get_components_by_class(unreal.StaticMeshComponent):
                    for material in mesh.get_materials():
                        assert isinstance(material,unreal.MaterialInstanceDynamic)
                        active=material.get_scalar_parameter_value('VerticalRevealActive')
                        start=material.get_scalar_parameter_value('VerticalRevealStartZ')
                        fade=material.get_scalar_parameter_value('VerticalRevealFadeHeightCm')
                        assert active==1.0 and abs(start-expected_start)<.1 and abs(fade-expected_fade)<.01
                        samples.append({'active':active,'start_z_cm':start,'fade_cm':fade})
                assert samples
                self.report['native_reveal_tick']=True;self.report['reveal_materials']=samples
                assert self.exit.request_interaction(self.player),'Actual interaction exit failed'
                self.report['exited_location_cm']=array(self.player.get_actor_location())
                assert self.entry.request_interaction(self.player),'Second entry failed'
                assert self.exit.request_interaction(self.player),'Second exit failed'
                self.report['round_trips']=2
                assert unreal.GameplayStatics.get_game_instance(self.world),'Game instance lost during transfer'
                self.levels.editor_request_end_play();self.transition('stop')
            elif self.state=='stop' and self.frames>=20:self.finish()
        except Exception:self.finish(traceback.format_exc())
        finally:self.busy=False
    def finish(self,error=None):
        unreal.unregister_slate_post_tick_callback(self.handle)
        if self.editor.get_game_world():self.levels.editor_request_end_play()
        self.report['asset_packages_unchanged']=self.before==packages()
        self.report['passed']=error is None and self.report['asset_packages_unchanged']
        if error:self.report['error']=error;unreal.log_error(error)
        REPORT.parent.mkdir(parents=True,exist_ok=True)
        REPORT.write_text(json.dumps(self.report,indent=2),encoding='utf-8')
        unreal.log('SSTO_RUNTIME_RESULT '+str(self.report['passed']))
        unreal.EditorPythonScripting.set_keep_python_script_alive(False);unreal.SystemLibrary.quit_editor()

RUNTIME=Runtime()
