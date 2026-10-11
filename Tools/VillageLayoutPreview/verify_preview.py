"""Read-only verification and rendered evidence of saved village preview decals."""
from pathlib import Path
import hashlib, json, time, traceback
import unreal
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'TunaSweeper/Saved/VillageLayoutPreview'
DEST='/Game/MainRaid/EditorPreviews/VillageLayout'
LAYOUT=json.loads((ROOT/'Docs/level_layout_snapshots/RaidVillage_20261012.json').read_text(encoding='utf-8'))
DECALS={row['label'].removeprefix('TS_Village_LayoutPreview_'):row['after'] for row in LAYOUT['actors'] if row['label'].startswith('TS_Village_LayoutPreview_')}
SPECS={name:tuple(data['location_cm'][:2]) for name,data in DECALS.items()}
def v(value): return [value.x,value.y,value.z]
def hashes():
    # Other region tasks may write their own assets concurrently.
    paths=[ROOT/'TunaSweeper/Content/MainRaid/RaidVillage.umap',ROOT/'TunaSweeper/Content/Materials/Landscape/M_LandScape.uasset']
    paths+=list((ROOT/'TunaSweeper/Content/MainRaid/EditorPreviews/VillageLayout').rglob('*.uasset'))
    return {str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}
class Verify:
    def __init__(self):
        OUT.mkdir(parents=True,exist_ok=True)
        self.levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
        self.actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
        self.before=hashes(); self.started=time.monotonic(); self.frames=0; self.state='load'; self.index=0
        self.report={'passed':False,'decals':[],'captures':[]}
        self.views=[('Overview',(0,0),6500)]+[(n,xy,900) for n,xy in SPECS.items()]
    def start(self):
        unreal.EditorPythonScripting.set_keep_python_script_alive(True)
        self.handle=unreal.register_slate_post_tick_callback(self.tick)
    def check(self):
        assert self.levels.load_level('/Game/MainRaid/RaidVillage')
        actors=self.actors.get_all_level_actors()
        decals=[a for a in actors if a.get_actor_label().startswith('TS_Village_LayoutPreview_')]
        assert len(decals)==3
        mat=unreal.load_asset(DEST+'/M_LocalLayoutPreview')
        assert mat.get_editor_property('material_domain')==unreal.MaterialDomain.MD_DEFERRED_DECAL
        landscape=next(a for a in actors if isinstance(a,unreal.Landscape))
        assert landscape.get_editor_property('landscape_material').get_path_name()=='/Game/Materials/Landscape/M_LandScape.M_LandScape'
        for name,(x,y) in SPECS.items():
            a=next(a for a in decals if a.get_actor_label()=='TS_Village_LayoutPreview_'+name)
            c=a.get_component_by_class(unreal.DecalComponent)
            location=a.get_actor_location(); size=c.get_editor_property('decal_size')
            assert abs(location.x-x)<1 and abs(location.y-y)<1
            expected_size=DECALS[name]['decal_size_cm']
            assert all(abs(actual-expected)<1 for actual,expected in zip(v(size),expected_size))
            assert abs(x)+size.y<=2500 and abs(y)+size.z<=2500,'Preview extends outside central staging area'
            assert a.get_editor_property('is_editor_only_actor') and a.get_editor_property('hidden')
            direction=a.get_actor_forward_vector()
            assert direction.z<-.999,'Projection must face down'
            assert c.get_editor_property('fade_screen_size')==0
            mi=c.get_editor_property('decal_material')
            assert mi.get_path_name()==DEST+f'/MI_Layout_{name}.MI_Layout_{name}'
            assert mi.get_editor_property('parent')==mat
            tex=unreal.MaterialEditingLibrary.get_material_instance_texture_parameter_value(mi,'PreviewTexture')
            assert tex.get_path_name()==DEST+f'/T_Layout_{name}.T_Layout_{name}'
            self.report['decals'].append({'name':name,'location_cm':v(location),'half_extents_cm':v(size),'direction':v(direction),'editor_only':True,'hidden_in_game':True})
        actual={a.get_name():a for a in actors}
        for row in LAYOUT['actors']:
            assert row['name'] in actual
            actor=actual[row['name']]
            coords=row['after']['location_cm']
            assert all(abs(aa-bb)<.01 for aa,bb in zip(v(actor.get_actor_location()),coords))
            assert all(abs(aa-bb)<.0001 for aa,bb in zip(v(actor.get_actor_scale3d()),row['after']['scale']))
            if row['category']=='central_test_actor':
                center,extent=actor.get_actor_bounds(False)
                assert abs(center.x)+extent.x<=2500.01 and abs(center.y)+extent.y<=2500.01,row['label']+' outside central 50m'
                for key in ('memo_id','placement_id'):
                    if key in row: assert actor.get_editor_property(key)==row[key]
        assert len(actors)==len(LAYOUT['actors'])
        self.report['preserved_snapshot_actors']=len(actors)
        self.report['central_staging_bounds_valid']=True
        self.report['landscape_material_unchanged']=True
        self.camera=self.actors.spawn_actor_from_class(unreal.CameraActor,unreal.Vector(0,0,24000),unreal.Rotator(pitch=-90,yaw=0,roll=0))
        self.camera.get_component_by_class(unreal.CameraComponent).set_editor_property('field_of_view',70.0)
        self.camera.get_component_by_class(unreal.CameraComponent).set_editor_property('aspect_ratio',1.0)
        self.camera.get_component_by_class(unreal.CameraComponent).set_editor_property('constrain_aspect_ratio',True)
        self.levels.editor_set_game_view(False)
        unreal.AutomationLibrary.set_editor_active_viewport_view_mode(unreal.ViewModeIndex.VMI_LIT)
        unreal.SystemLibrary.execute_console_command(landscape,'ShowFlag.BillboardSprites 0')
        unreal.SystemLibrary.execute_console_command(landscape,'ShowFlag.Decals 1')
        self.view()
    def view(self):
        name,(x,y),z=self.views[self.index]
        self.camera.set_actor_location(unreal.Vector(x,y,z),False,False)
        self.levels.pilot_level_actor(self.camera)
        self.levels.set_exact_camera_view(True)
        unreal.AutomationLibrary.finish_loading_before_screenshot()
        self.state='warmup'; self.frames=0; self.stage=time.monotonic()
    def tick(self,dt):
        if getattr(self,'busy',False): return
        self.busy=True
        try:
            assert time.monotonic()-self.started<600,'Capture timeout'
            self.frames+=1
            if self.state=='load' and self.frames>=3 and self.levels.get_viewport_config_keys(): self.check()
            elif self.state=='warmup':
                self.levels.editor_invalidate_viewports()
                if self.frames>=45 and time.monotonic()-self.stage>=8:
                    self.path=OUT/(self.views[self.index][0]+'.png')
                    self.task=unreal.AutomationLibrary.take_high_res_screenshot(1280,1280,str(self.path),camera=self.camera,delay=1.0,force_game_view=False)
                    self.state='capture'
            elif self.state=='capture':
                self.levels.editor_invalidate_viewports()
                assert self.task and self.task.is_valid_task()
                if self.task.is_task_done() and self.path.exists():
                    data=self.path.read_bytes()
                    assert len(data)>10000 and data[:8]==b'\x89PNG\r\n\x1a\n'
                    import struct
                    assert struct.unpack('>II',data[16:24])==(1280,1280),'Incorrect rendered image dimensions'
                    self.report['captures'].append(str(self.path))
                    self.index+=1
                    if self.index==len(self.views): self.finish()
                    else: self.view()
        except Exception: self.finish(traceback.format_exc())
        finally: self.busy=False
    def finish(self,error=None):
        unreal.unregister_slate_post_tick_callback(self.handle)
        after=hashes()
        self.report['changed_packages']=[p for p in set(self.before)|set(after) if self.before.get(p)!=after.get(p)]
        self.report['saved_packages_unchanged']=not self.report['changed_packages']
        self.report['passed']=error is None and self.report['saved_packages_unchanged'] and len(self.report['captures'])==4
        if error: self.report['error']=error; unreal.log_error(error)
        (OUT/'verification.json').write_text(json.dumps(self.report,indent=2))
        # Release the temporary piloted camera and let render resources drain
        # before quitting. None of these viewport changes are saved.
        self.levels.eject_pilot_level_actor()
        if getattr(self,'camera',None):
            self.actors.destroy_actor(self.camera)
            self.camera=None
        self.levels.load_level('/Engine/Maps/Entry')
        self.cleanup_frames=0
        self.cleanup_handle=unreal.register_slate_post_tick_callback(self.cleanup)
    def cleanup(self,dt):
        self.cleanup_frames+=1
        if self.cleanup_frames>=30:
            unreal.unregister_slate_post_tick_callback(self.cleanup_handle)
            unreal.EditorPythonScripting.set_keep_python_script_alive(False)
            unreal.SystemLibrary.quit_editor()
VERIFY=Verify()
VERIFY.start()
