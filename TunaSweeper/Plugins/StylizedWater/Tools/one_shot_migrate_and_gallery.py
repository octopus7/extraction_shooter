# ONE_SHOT_WATER_ASSET_GENERATOR: migration and saved review map; delete after the asset commit.
import unreal, json, os
out=os.path.join(unreal.Paths.project_saved_dir(),"WaterRebuild")
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
results=[]
for level,suffix in [("/Game/Maps/DemoRaidMap","Demo"),("/Game/MainRaid/RaidMap","Raid")]:
    world=unreal.EditorLoadingAndSavingUtils.load_map(level)
    assert world
    old=[a for a in actors.get_all_level_actors() if "StylizedWater" in a.get_class().get_path_name()]
    for i,a in enumerate(old):
        label=a.get_actor_label()
        size=a.get_editor_property("surface_size")
        height=a.get_editor_property("water_level_offset")
        transform=a.get_actor_transform()
        new=actors.convert_actors([a],unreal.StylizedWaterBodyActor,"")
        if not new: new=[candidate for candidate in actors.get_all_level_actors() if candidate.get_class()==unreal.StylizedWaterBodyActor.static_class() and candidate.get_actor_label()==label]
        assert len(new)==1,(level,label)
        b=new[0]
        if "River" in label: b.apply_flowing_river_preset()
        elif "Lake" in label: b.apply_calm_lake_preset()
        else: b.apply_gentle_beach_preset()
        b.set_actor_transform(transform,False,False)
        b.set_actor_label(label)
        b.set_editor_property("surface_size",size)
        b.set_editor_property("water_level_offset",height)
        b.set_editor_property("enable_sky_parallax",True)
        package="/StylizedWater/MaskWater/Masks/T_Mask"+suffix+str(i)
        mask=unreal.GenerateMaskWaterAssetsCommandlet.bake_actor_mask(b,package)
        assert mask,package
        b.set_editor_property("boundary_mask",mask)
        b.fit_surface_to_terrain()
        b.rebuild_surface()
        results.append(dict(level=level,label=label,mask=package,fit=b.get_editor_property("last_terrain_fit")))
    if old: assert unreal.EditorLoadingAndSavingUtils.save_map(world,level)
with open(os.path.join(out,"migration.json"),"w",encoding="utf-8") as f: json.dump(results,f,indent=2)
world=unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
ground=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(0,0,-220))
ground.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cube"))
ground.static_mesh_component.set_material(0,unreal.load_asset("/StylizedWater/Review/M_ReviewGround"))
ground.set_actor_scale3d(unreal.Vector(500,500,1))
ground.set_actor_label("Review ground")
for i,(name,preset) in enumerate([("Calm Lake","apply_calm_lake_preset"),("Gentle Beach","apply_gentle_beach_preset"),("Flowing River","apply_flowing_river_preset")]):
    b=actors.spawn_actor_from_class(unreal.StylizedWaterBodyActor,unreal.Vector(i*8000,0,0))
    getattr(b,preset)()
    b.set_actor_label(name)
    b.set_editor_property("enable_sky_parallax",True)
    b.rebuild_surface()
cam=actors.spawn_actor_from_class(unreal.CameraActor,unreal.Vector(0,-7200,2700),unreal.Rotator(-20,90,0))
cam.set_actor_label("Lake review camera")
assert unreal.EditorLoadingAndSavingUtils.save_map(world,"/StylizedWater/Review/WaterMaskReview")
unreal.log("WATER_MIGRATION_COMPLETE "+str(len(results)))
