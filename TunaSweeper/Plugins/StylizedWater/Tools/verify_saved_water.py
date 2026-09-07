"""Read-only saved-water verification. Run via UE Python; writes only Saved/WaterRebuild."""
import unreal, json, os
assert not hasattr(unreal,"GenerateMaskWaterAssetsCommandlet"),"One-shot generator is still registered"
reg=unreal.AssetRegistryHelpers.get_asset_registry()
reg.search_all_assets(True)
opts=unreal.AssetRegistryDependencyOptions(True,True,True,True,True)
legacy_names=["BP_StylizedWaterBody_Internal","M_StylizedWaterSurface","M_StylizedWaterShoreOverlay","MI_StylizedWater_CalmAnime","MI_StylizedWater_ShoreOverlay","T_WaterDepthGradient"]
for name in legacy_names:
    assert not unreal.EditorAssetLibrary.does_asset_exist("/StylizedWater/Generated/Internal/"+name),name
base="/StylizedWater/MaskWater/M_WaterMask"
base_deps=[str(p) for p in reg.get_dependencies(base,opts)]
assert not any("/SkyParallax/" in p for p in base_deps),base_deps
saved=reg.get_assets_by_path("/StylizedWater",recursive=True)
assert len(saved)==10,len(saved)
loaded=[]
for item in saved:
    path=str(item.package_name)
    asset=unreal.load_asset(path)
    assert asset,path
    loaded.append(path)
actor_sub=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
rows=[]
for level in ["/Game/Maps/DemoRaidMap","/Game/MainRaid/RaidMap"]:
    deps=[str(p) for p in reg.get_dependencies(level,opts)]
    assert not any("/StylizedWater/Generated/" in p for p in deps),level
    world=unreal.EditorLoadingAndSavingUtils.load_map(level)
    assert world,level
    water=[a for a in actor_sub.get_all_level_actors() if a.get_class()==unreal.StylizedWaterBodyActor.static_class()]
    assert len(water)==1,(level,len(water))
    for a in water:
        assert not any(c.get_name()=="ShoreOverlay" for c in a.get_components_by_class(unreal.ActorComponent))
        mask=a.get_editor_property("boundary_mask")
        assert mask and "/MaskWater/Masks/" in mask.get_path_name()
        sections=a.get_editor_property("water_surface").get_num_sections()
        assert sections==1
        rows.append(dict(level=level,actor=a.get_actor_label(),mask=mask.get_path_name(),surface_sections=sections,terrain_fit=a.get_editor_property("last_terrain_fit"),components=[c.get_name() for c in a.get_components_by_class(unreal.ActorComponent)]))
out=os.path.join(unreal.Paths.project_saved_dir(),"WaterRebuild")
os.makedirs(out,exist_ok=True)
with open(os.path.join(out,"saved_verification.json"),"w",encoding="utf-8") as f:
    json.dump(dict(loaded_assets=loaded,base_dependencies=base_deps,maps=rows,legacy_assets_absent=True),f,indent=2)
unreal.log("WATER_SAVED_VERIFY_PASS assets="+str(len(loaded))+" maps="+str(len(rows)))
