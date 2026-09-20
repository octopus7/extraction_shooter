"""Read-only checks for title/player animation separation and saved level binding."""
import unreal
base='/Game/Characters/Player/LunaMk2/Animations/'
title=unreal.load_asset(base+'Title/ABP_LunaMk2_Title')
assert title,'Missing title-only animation blueprint'
title_class=unreal.EditorAssetLibrary.load_blueprint_class(base+'Title/ABP_LunaMk2_Title')
player_class=unreal.EditorAssetLibrary.load_blueprint_class(base+'ABP_LunaMk2')
assert title_class and player_class and title_class!=player_class
for path,name,expected in [('/Game/Characters/Player/BP_TunaSweeperPlayerCharacter','CharacterMesh0',player_class),('/Game/UI/Title/BP_TitlePresentationActor','BodyMesh',title_class)]:
    cdo=unreal.get_default_object(unreal.EditorAssetLibrary.load_blueprint_class(path))
    mesh=next(c for c in cdo.get_components_by_class(unreal.SkeletalMeshComponent) if c.get_name()==name)
    assert mesh.get_editor_property('anim_class')==expected,(path,'wrong animation class')
unreal.EditorLoadingAndSavingUtils.load_map('/Game/Maps/IntroMap')
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
titles=[a for a in actors if isinstance(a,unreal.TunaSweeperTitlePresentationActor)]
assert titles,'Missing title actor in IntroMap'
for a in titles:
    body=next(c for c in a.get_components_by_class(unreal.SkeletalMeshComponent) if c.get_name()=='BodyMesh')
    assert body.get_editor_property('anim_class')==title_class
unreal.log('TITLE_ABP_BINDINGS_PASSED')
