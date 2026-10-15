"""UE5.7 read-only asset/map verification for faction cats and Region2 robots.

Run in a coordinated fresh normal editor with -ExecutePythonScript. Only writes
Saved/Region2Work/asset_validation.json. Temporary actor instances in a new unsaved
level exercise native construction; no packages or maps are saved. The caller
owns editor shutdown and can observe REGION2_VERIFY_DONE/report completion.
"""
import hashlib,json,math,time,traceback
from pathlib import Path
import unreal as u
ROOT=Path(__file__).resolve().parents[2];WORK=ROOT/'TunaSweeper/Saved/Region2Work'
DEST='/Game/Characters/Region2Robots';CATS='/Game/Characters/FactionCats'
MAP='/Game/MainRaid/RaidVillage';REPORT=WORK/'asset_validation.json'
REGION2_VERIFY_DONE=False
CAT_SPECS=[('MarineCat','marine_cat'),('PirateCat','pirate_cat')]
ROBOT_SPECS=[('BP_DeliveryRobot_Marine','DeliveryRobot','MarineBlue',20,'enemy.marine_delivery',False),('BP_DeliveryRobot_Pirate','DeliveryRobot','PirateRed',21,'enemy.pirate_delivery',False),('BP_RabbitTrike','RabbitTrike','RabbitAmber',22,'enemy.rabbit_trike',True)]
NPC_LABELS={'Village_MarineCat':'provider.marine_cat','Village_PirateCat':'provider.pirate_cat'}
ENEMY_LABELS={'Village_MarineDelivery_01':'enemy.marine_delivery','Village_MarineDelivery_02':'enemy.marine_delivery','Village_PirateDelivery_01':'enemy.pirate_delivery','Village_PirateDelivery_02':'enemy.pirate_delivery','Village_RabbitTrike_01':'enemy.rabbit_trike','Village_RabbitTrike_02':'enemy.rabbit_trike'}
def xyz(v):return [v.x,v.y,v.z]
def close(a,b,t=.05):assert len(a)==len(b) and max(abs(x-y) for x,y in zip(a,b))<t,(a,b)
def protect():
    files=[ROOT/'TunaSweeper/Content/MainRaid/RaidVillage.umap',ROOT/'TunaSweeper/Content/Data/MainRuntimeDefaults/EnemySpawns.json']
    for folder in ('FactionCats','Region2Robots'):files+=list((ROOT/'TunaSweeper/Content/Characters'/folder).rglob('*.uasset'))
    return {p.relative_to(ROOT).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in files if p.exists()}
def mesh_bounds(mesh):
    b=mesh.get_bounds();return [b.origin.x-b.box_extent.x,b.origin.y-b.box_extent.y,b.origin.z-b.box_extent.z,b.origin.x+b.box_extent.x,b.origin.y+b.box_extent.y,b.origin.z+b.box_extent.z]
def hit_info(result):
    if result is None:return None
    hit=result if isinstance(result,u.HitResult) else next((v for v in result if isinstance(v,u.HitResult)),None) if isinstance(result,tuple) else None
    return hit.to_tuple() if hit and hit.to_tuple()[0] else None
def nav_project(world,point):
    result=u.NavigationSystemV1.project_point_to_navigation(world,point,None,None,u.Vector(90,90,220))
    if isinstance(result,u.Vector):return result
    if isinstance(result,tuple):
        if any(isinstance(v,bool) and not v for v in result):return None
        return next((v for v in result if isinstance(v,u.Vector)),None)
    return None

class Verify:
    def __init__(self):
        self.baseline=protect();self.report={'passed':False,'engine':u.SystemLibrary.get_engine_version(),'asset_checks':[],'blueprints':[]};self.temp=[]
        self.actors=u.get_editor_subsystem(u.EditorActorSubsystem);self.levels=u.get_editor_subsystem(u.LevelEditorSubsystem);self.editor=u.get_editor_subsystem(u.UnrealEditorSubsystem)
        self.sm=u.get_editor_subsystem(u.StaticMeshEditorSubsystem) or u.get_default_object(u.StaticMeshEditorSubsystem)
        self.start=time.monotonic();self.stage='preload';self.ticks=0;self.busy=False;self.handle=None
    def begin(self):
        WORK.mkdir(parents=True,exist_ok=True)
        if REPORT.exists():REPORT.unlink()
        u.EditorPythonScripting.set_keep_python_script_alive(True);self.handle=u.register_slate_post_tick_callback(self.tick)
    def require_clean_editor(self):
        # World switches are safe only in a clean dedicated editor. Never discard
        # a live user's PIE session or unsaved map/content when this is run later.
        assert not self.levels.is_in_play_in_editor(),'Stop PIE before running this read-only verifier'
        assert not u.EditorLoadingAndSavingUtils.get_dirty_map_packages(),'Unsaved map packages: use a clean dedicated editor'
        assert not u.EditorLoadingAndSavingUtils.get_dirty_content_packages(),'Unsaved content packages: use a clean dedicated editor'
    def preload(self):
        self.require_clean_editor()
        bodymi=u.load_asset(DEST+'/Materials/MI_RobotBodyAtlas');assert bodymi
        atlas=u.MaterialEditingLibrary.get_material_instance_texture_parameter_value(bodymi,'BodyAtlas')
        palette=u.load_asset(CATS+'/T_FactionCatsPalette');assert atlas and palette
        self.textures=[(atlas,[1024,1024]),(palette,[128,8])]
        self.texture_started=time.monotonic();self.stage='textures'
    def textures_ready(self):
        # UE asynchronously compiles newly loaded textures. GetSize initially
        # returns the 32x32 fallback texture; allow editor ticks to finish it.
        sizes=[{'texture':tex.get_path_name(),'actual':[tex.blueprint_get_size_x(),tex.blueprint_get_size_y()],'expected':expected} for tex,expected in self.textures]
        if all(row['actual']==row['expected'] for row in sizes):
            self.report['texture_dimensions']=sizes;self.stage='assets'
        else:
            assert time.monotonic()-self.texture_started<60,'Texture dimensions did not become ready: '+json.dumps(sizes)
    def assets(self):
        self.require_clean_editor()
        lib=u.MaterialEditingLibrary
        body=u.load_asset(DEST+'/Materials/M_RobotBodyAtlas');eyes=u.load_asset(DEST+'/Materials/M_RobotEyes')
        bodymi=u.load_asset(DEST+'/Materials/MI_RobotBodyAtlas');assert body and eyes and bodymi
        assert bodymi.get_editor_property('parent')==body
        assert lib.get_material_property_input_node(body,u.MaterialProperty.MP_EMISSIVE_COLOR) is None
        assert lib.get_material_property_input_node(eyes,u.MaterialProperty.MP_EMISSIVE_COLOR) is not None
        atlas=lib.get_material_instance_texture_parameter_value(bodymi,'BodyAtlas');assert atlas
        assert atlas.blueprint_get_size_x()==atlas.blueprint_get_size_y()==1024
        self.eyes={}
        for suffix,color in [('MarineBlue',(.015,.4,1)),('PirateRed',(1,.025,.015)),('RabbitAmber',(1,.22,.01))]:
            mi=u.load_asset(DEST+'/Materials/MI_RobotEyes_'+suffix);assert mi and mi.get_editor_property('parent')==eyes
            actual=lib.get_material_instance_vector_parameter_value(mi,'EyeColor');close([actual.r,actual.g,actual.b],color,.0001)
            assert abs(lib.get_material_instance_scalar_parameter_value(mi,'EyeStrength')-3)<.001
            self.eyes[suffix]=mi
        axis=json.loads((ROOT/'TunaSweeper/SourceArt/Characters/Region2Robots/axis_validation.json').read_text())['assets']
        for entry in axis:
            name=entry['mesh'];mesh=u.load_asset(DEST+'/Meshes/'+name);assert mesh
            close(mesh_bounds(mesh),entry['corrected_ue_bounds_cm'],.2)
            slots=list(mesh.static_materials);assert [str(s.material_slot_name) for s in slots]==(['BodyAtlas','Eyes'] if name.endswith('_Body') else ['BodyAtlas'])
            assert slots[0].material_interface==bodymi and self.sm.get_num_uv_channels(mesh,0)>=1
            if len(slots)==2:assert slots[1].material_interface.get_editor_property('parent')==eyes
            self.report['asset_checks'].append({'mesh':mesh.get_path_name(),'bounds_cm':mesh_bounds(mesh),'slots':[str(s.material_slot_name) for s in slots],'uv_channels':self.sm.get_num_uv_channels(mesh,0)})
        catmanifest=json.loads((ROOT/'TunaSweeper/SourceArt/Characters/FactionCats/model_manifest.json').read_text())
        catmat=u.load_asset(CATS+'/M_FactionCats');palette=u.load_asset(CATS+'/T_FactionCatsPalette');assert catmat and palette
        assert palette.get_editor_property('filter')==u.TextureFilter.TF_NEAREST
        assert palette.get_editor_property('mip_gen_settings')==u.TextureMipGenSettings.TMGS_NO_MIPMAPS
        assert [palette.blueprint_get_size_x(),palette.blueprint_get_size_y()]==[128,8]
        assert lib.get_material_property_input_node(catmat,u.MaterialProperty.MP_BASE_COLOR).texture==palette
        for entry in catmanifest['assets']:
            mesh=u.load_asset(CATS+'/'+entry['name']);assert mesh
            bounds=entry['bounds_cm'];expected=[bounds[0][0],-bounds[1][1],bounds[2][0],bounds[0][1],-bounds[1][0],bounds[2][1]]
            close(mesh_bounds(mesh),expected)
            assert self.sm.get_num_uv_channels(mesh,0)==1 and len(mesh.static_materials)==1 and mesh.static_materials[0].material_interface==catmat
        for name,ident in CAT_SPECS:
            cls=u.EditorAssetLibrary.load_blueprint_class(CATS+'/BP_'+name);assert cls
            cdo=u.get_default_object(cls);assert isinstance(cdo,u.TunaSweeperFacilityNpcActor) and not isinstance(cdo,u.TunaSweeperQuestPresenceNpcActor)
            assert str(cdo.get_editor_property('npc_id'))=='npc.'+ident
            assert str(cdo.get_editor_property('quest_provider_id'))=='provider.'+ident
            assert cdo.get_editor_property('body_mesh').static_mesh==u.load_asset(CATS+'/SM_'+name)
            assert cdo.get_editor_property('head_mesh').static_mesh is None
            marker=cdo.get_editor_property('quest_marker_component')
            assert all(not isinstance(m,u.MaterialInstanceDynamic) for m in marker.get_materials()),'NPC template contains serialized dynamic marker material'
            close(xyz(cdo.get_editor_property('body_mesh').get_editor_property('relative_scale3d')),[1,1,1])
        # UE5.7 may defer PostLoad shader jobs until rendering asks for them.
        # GetStatistics submits missing jobs and waits for compilation without
        # editing the graph/package, avoiding false success with fallback shaders.
        self.report['material_shader_statistics']=[]
        for material in [catmat,body,eyes,bodymi]+list(self.eyes.values()):
            stats=lib.get_statistics(material)
            assert stats.num_pixel_shader_instructions>0 and stats.num_vertex_shader_instructions>0,('Missing compiled shaders',material.get_path_name())
            self.report['material_shader_statistics'].append({'material':material.get_path_name(),'pixel_instructions':stats.num_pixel_shader_instructions,'vertex_instructions':stats.num_vertex_shader_instructions})
        # Temporary instances test OnConstruction-dependent meshes and wheel layout.
        u.EditorLoadingAndSavingUtils.new_blank_map(False)
        for bpname,model,color,faction,enemyid,trike in ROBOT_SPECS:
            cls=u.EditorAssetLibrary.load_blueprint_class(DEST+'/'+bpname);assert cls
            cdo=u.get_default_object(cls);assert isinstance(cdo,u.TunaSweeperWheeledEnemyCharacter)
            assert bool(cdo.get_editor_property('tricycle'))==trike
            assert str(cdo.get_editor_property('enemy_id'))==enemyid
            assert cdo.get_editor_property('faction_component').get_faction_id()==faction
            actor=self.actors.spawn_actor_from_class(cls,u.Vector(0,0,1000));assert actor;self.temp.append(actor)
            bodycomp=actor.get_editor_property('robot_body');assert bodycomp.static_mesh==u.load_asset(DEST+'/Meshes/SM_'+model+'_Body')
            assert bodycomp.get_material(0)==bodymi and bodycomp.get_material(1)==self.eyes[color]
            hh=78 if trike else 60;radius=30 if trike else 32
            close(xyz(bodycomp.get_editor_property('relative_location')),[0,0,-hh])
            expected=[(-65 if trike else 0,-46 if trike else -48,radius-hh),(-65 if trike else 0,46 if trike else 48,radius-hh),(77,0,radius-hh)]
            visible=0
            for prop,pos in zip(('left_wheel','right_wheel','front_wheel'),expected):
                comp=actor.get_editor_property(prop);assert comp.static_mesh==u.load_asset(DEST+'/Meshes/SM_'+model+'_Wheel')
                close(xyz(comp.get_editor_property('relative_location')),pos)
                assert comp.get_num_materials()==1 and comp.get_material(0)==bodymi
                assert comp.get_collision_enabled()==u.CollisionEnabled.NO_COLLISION
                if comp.get_editor_property('visible') and not comp.get_editor_property('hidden_in_game'):visible+=1
            assert visible==(3 if trike else 2)
            assert bodycomp.get_collision_enabled()==u.CollisionEnabled.NO_COLLISION
            self.report['blueprints'].append({'bp':bpname,'enemy_id':enemyid,'faction_id':faction,'visible_wheels':visible,'eye_material':self.eyes[color].get_path_name(),'body_nonemissive':True})
            assert self.actors.destroy_actor(actor);self.temp.remove(actor)
        assert self.levels.load_level(MAP);self.stage='map';self.ticks=0
    def map(self):
        world=self.editor.get_editor_world();actors=list(self.actors.get_all_level_actors());labels={a.get_actor_label():a for a in actors}
        assert set(NPC_LABELS).issubset(labels) and set(ENEMY_LABELS).issubset(labels)
        starts=[a for a in actors if isinstance(a,u.PlayerStart)];assert starts
        start=next((a for a in starts if a.get_actor_label()=='TS_Village_PlayerStart'),starts[0]).get_actor_location()
        positions=[labels[label].get_actor_location() for label in NPC_LABELS]
        assert math.hypot(positions[0].x-positions[1].x,positions[0].y-positions[1].y)>=500
        for label,provider in NPC_LABELS.items():assert str(labels[label].get_editor_property('quest_provider_id'))==provider
        rows=json.loads((ROOT/'TunaSweeper/Content/Data/MainRuntimeDefaults/EnemySpawns.json').read_text())
        entries=[];byid={}
        for a in actors:
            if isinstance(a,u.TunaSweeperRaidPlacementAnchor):byid.setdefault(int(a.get_editor_property('placement_id')),[]).append(a)
        landscape=[a for a in actors if isinstance(a,u.LandscapeProxy)];assert landscape
        ignored=[a for a in actors if a not in landscape]
        for label,profile in ENEMY_LABELS.items():
            a=labels[label];pid=int(a.get_editor_property('placement_id'));assert a.get_editor_property('anchor_kind')==u.TunaSweeperRaidPlacementAnchorKind.ENEMY
            assert 0<pid<1000 and len(byid[pid])==1 and not a.get_editor_property('allow_duplicate_placement_id')
            matches=[r for r in rows if r.get('level_name')=='RaidVillage' and r.get('placement_id')==pid]
            assert len(matches)==1 and matches[0]['profile_id']==profile and matches[0]['spawn_chance']==10000
            assert not any(k in matches[0] for k in ('location','rotation','scale'))
            p=a.get_actor_location()
            assert all(math.hypot(p.x-q.x,p.y-q.y)>1600 for q in positions+[start])
            hit=None
            for dx,dy in [(0,0),(.1,0),(0,.1)]:
                hit=hit_info(u.SystemLibrary.line_trace_single(world,u.Vector(p.x+dx,p.y+dy,15000),u.Vector(p.x+dx,p.y+dy,-5000),u.TraceTypeQuery.TRACE_TYPE_QUERY1,False,ignored,u.DrawDebugTrace.NONE,True))
                if hit:break
            assert hit is not None
            hh=78 if profile=='enemy.rabbit_trike' else 60
            assert abs(p.z-hh-hit[5].z-2)<.5,(label,'not grounded')
            entries.append({'label':label,'placement_id':pid,'profile_id':profile,'position_cm':xyz(p)})
        volumes=[a for a in actors if isinstance(a,u.NavMeshBoundsVolume)];assert volumes,'No navigation bounds'
        for entry in entries:
            p=entry['position_cm'];covered=False
            for nav in volumes:
                c,e=nav.get_actor_bounds(False)
                if abs(p[0]-c.x)<e.x and abs(p[1]-c.y)<e.y and abs(p[2]-c.z)<e.z:covered=True
            assert covered,('spawn outside navigation bounds',entry['label'])
        recast=[a for a in actors if isinstance(a,u.RecastNavMesh)];assert recast
        for nav in recast:
            if nav.get_actor_label()=='Village_Region2_Recast':
                assert nav.get_editor_property('runtime_generation')==u.RuntimeGenerationType.DYNAMIC
                assert nav.get_editor_property('force_rebuild_on_load')
        # Retain all three station/arrival systems added by the earlier map task.
        stations=[a for a in actors if isinstance(a,u.LoopRailStation)];assert len(stations)==3
        arrivals=[a for a in actors if isinstance(a,u.TunaSweeperQuestArrivalTrigger)];assert len(arrivals)==3
        assert {str(a.get_editor_property('location_id')) for a in arrivals}=={'village.station.north','village.station.east','village.station.west'}
        for arrival in arrivals:
            assert arrival.get_attach_parent_actor() in stations
            assert arrival.get_editor_property('vehicle_condition')==u.TunaSweeperArrivalVehicleCondition.NONE
            assert arrival.get_editor_property('arrival_area').get_collision_enabled()==u.CollisionEnabled.NO_COLLISION
        self.report.update(map=MAP,placements=entries,station_count=len(stations),arrival_actor_count=len(arrivals),navigation_bounds_count=len(volumes),navigation_recast_count=len(recast))
        # Real character instances supply the actual movement agent's navigation
        # properties. They exist only in this unsaved verification world.
        bp_by_profile={spec[4]:spec[0] for spec in ROBOT_SPECS}
        self.nav_pending=[];self.nav_paths=[]
        for entry in entries:
            cls=u.EditorAssetLibrary.load_blueprint_class(DEST+'/'+bp_by_profile[entry['profile_id']])
            agent=self.actors.spawn_actor_from_class(cls,u.Vector(*entry['position_cm']));assert agent
            agent.set_actor_label('QA_Navigation_'+entry['label']);self.temp.append(agent)
            self.nav_pending.append((entry,agent))
        self.nav_started=time.monotonic();self.stage='navigation';self.ticks=0
    def navigation(self):
        world=self.editor.get_editor_world();remaining=[]
        for entry,agent in self.nav_pending:
            point=agent.get_actor_location();start=nav_project(world,point)
            found=None
            if start and math.hypot(start.x-point.x,start.y-point.y)<95:
                for distance in (150,220):
                    for dx,dy in ((distance,0),(-distance,0),(0,distance),(0,-distance)):
                        target=nav_project(world,start+u.Vector(dx,dy,0))
                        if target is None or math.hypot(target.x-start.x,target.y-start.y)<100:continue
                        path=u.NavigationSystemV1.find_path_to_location_synchronously(world,start,target,agent,None)
                        if path and path.is_valid() and not path.is_partial() and 100<=path.get_path_length()<1000:
                            found={'label':entry['label'],'profile_id':entry['profile_id'],'projected_start_cm':xyz(start),'reachable_target_cm':xyz(target),'path_length_cm':path.get_path_length(),'agent_class':agent.get_class().get_path_name(),'partial':False}
                            break
                    if found:break
            if found:self.nav_paths.append(found)
            else:remaining.append((entry,agent))
        self.nav_pending=remaining
        if not remaining:
            assert len(self.nav_paths)==6
            self.report['navigation_local_paths']=self.nav_paths
            self.report['navigation_projection_and_agent_paths_passed']=True
            self.finish()
        else:
            assert time.monotonic()-self.nav_started<120,'Local encounter navigation did not become ready: '+', '.join(entry['label'] for entry,agent in remaining)
    def tick(self,dt):
        if self.busy:return
        self.busy=True;self.ticks+=1
        try:
            assert time.monotonic()-self.start<240,'Asset verification timeout'
            if self.stage=='preload':self.preload()
            elif self.stage=='textures':self.textures_ready()
            elif self.stage=='assets':self.assets()
            elif self.stage=='map' and self.ticks>=60:self.map()
            elif self.stage=='navigation' and self.ticks%15==0:self.navigation()
        except Exception:self.finish(traceback.format_exc())
        finally:self.busy=False
    def finish(self,error=None):
        global REGION2_VERIFY_DONE
        for actor in self.temp:
            if actor:self.actors.destroy_actor(actor)
        self.temp=[]
        unchanged=protect()==self.baseline
        self.report.update(passed=not error and unchanged,packages_unchanged=unchanged,elapsed_seconds=round(time.monotonic()-self.start,2))
        if error:self.report['error']=error;u.log_error(error)
        REPORT.write_text(json.dumps(self.report,indent=2),encoding='utf-8')
        if self.handle:u.unregister_slate_post_tick_callback(self.handle);self.handle=None
        REGION2_VERIFY_DONE=True
        u.log('REGION2_ASSET_VERIFY_'+('PASS' if self.report['passed'] else 'FAIL'))

verification=Verify();verification.begin()
