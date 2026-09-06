"""One-off, explicitly authorized BP mesh connection; GARAGE_BP_VERIFY_ONLY=1 audits only."""
import hashlib
import json
import os
from pathlib import Path
import unreal

ROOT=Path(__file__).resolve().parents[2]
BP='/Game/Environment/Bunker/GarageDoor/BP_RaidBunkerGarageDoor'
KIT='/Game/Environment/Bunker/GarageDoorConcrete'
OUT=ROOT/'TunaSweeper/SourceArt/Environment/GarageDoorConcrete'
VERIFY=os.environ.get('GARAGE_BP_VERIFY_ONLY')=='1'
mapping={
    'frame_top_mesh':'FrameTop','frame_left_mesh':'FrameLeft','frame_right_mesh':'FrameRight',
    'canopy_rail_left_mesh':'CanopyRailLeft','canopy_rail_right_mesh':'CanopyRailRight',
    'temporary_wall_left_mesh':'TemporaryWallLeft','temporary_wall_right_mesh':'TemporaryWallRight',
    'temporary_roof_mesh':'TemporaryRoof','led_bar_mesh':'LEDBar',
    'lower_panel_mesh':'LowerEmbeddedPanel','motion_indicator_mesh':'MotionIndicator'}
mesh_paths={k:f'{KIT}/Meshes/SM_GarageConcrete_{v}' for k,v in mapping.items()}
meshes={k:unreal.load_asset(v) for k,v in mesh_paths.items()}
assert all(isinstance(v,unreal.StaticMesh) for v in meshes.values())
upper=unreal.load_asset(KIT+'/Meshes/SM_GarageConcrete_UpperPanel')
assert isinstance(upper,unreal.StaticMesh)

def protected():
    paths=list((ROOT/'TunaSweeper/Content/Environment/Bunker/GarageDoor').rglob('*.uasset'))
    paths+=list((ROOT/'TunaSweeper/Content/Environment/Bunker/GarageDoorConcrete').rglob('*.uasset'))
    return {str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest()
            for p in paths if p.name!='BP_RaidBunkerGarageDoor.uasset'}

before=protected()
bp=unreal.load_asset(BP)
cls=unreal.EditorAssetLibrary.load_blueprint_class(BP)
cdo=unreal.get_default_object(cls)
geometry=json.loads((OUT/'existing_blueprint_layout.json').read_text(encoding='utf-8'))
preserve=[k for k in geometry if 'mesh' not in k and 'material' not in k]
def settings(actor):
    result={}
    for k in preserve:
        v=actor.get_editor_property(k)
        result[k]=list(v) if isinstance(v,unreal.Array) else v
    return result
original_settings=settings(cdo)
if not VERIFY:
    for k,v in meshes.items():cdo.set_editor_property(k,v)
    cdo.set_editor_property('upper_panel_meshes',[upper]*4)
    cdo.set_editor_property('use_authored_mesh_materials',True)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    assert unreal.EditorAssetLibrary.save_loaded_asset(bp,only_if_is_dirty=False)

cls=unreal.EditorAssetLibrary.load_blueprint_class(BP)
cdo=unreal.get_default_object(cls)
for k,v in meshes.items():assert cdo.get_editor_property(k)==v,k
assert list(cdo.get_editor_property('upper_panel_meshes'))==[upper]*4
assert cdo.get_editor_property('use_authored_mesh_materials')
assert settings(cdo)==original_settings,'Door geometry/motion settings changed'

# Construct an instance to verify native construction applies the saved defaults.
world=unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
assert world
actor=unreal.EditorLevelLibrary.spawn_actor_from_class(cls,unreal.Vector(0,0,0))
assert actor
components=actor.get_components_by_class(unreal.StaticMeshComponent)
component_report=[]
for c in components:
    mesh=c.static_mesh
    if not mesh:continue
    assert mesh.get_path_name().startswith(KIT+'/Meshes/'),(c.get_name(),mesh.get_path_name())
    mats=[c.get_material(i) for i in range(c.get_num_materials())]
    assert all(m and m.get_path_name().startswith(KIT+'/Materials/') for m in mats),c.get_name()
    component_report.append({'component':c.get_name(),'mesh':mesh.get_path_name(),
        'materials':[m.get_path_name() for m in mats]})
assert len(component_report)==16, len(component_report)
for name in ['OpeningIndicator','ClosingIndicator']:
    c=next(c for c in components if c.get_name()==name)
    assert c.static_mesh==meshes['motion_indicator_mesh']
unreal.EditorLevelLibrary.destroy_actor(actor)
assert before==protected(),'An unrelated garage asset changed'
report={'passed':True,'mode':'fresh-process reload' if VERIFY else 'connect and construct',
    'blueprint':BP,'mesh_properties':mesh_paths,'upper_panel_meshes':[upper.get_path_name()]*4,
    'use_authored_mesh_materials':True,'preserved_settings':settings(cdo),
    'constructed_components':component_report,'other_garage_assets_unchanged':True}
report_path=OUT/('blueprint_reload_validation.json' if VERIFY else 'blueprint_connection_validation.json')
report_path.write_text(json.dumps(report,indent=2),encoding='utf-8')
unreal.log('GARAGE_BP_CONNECTION_PASSED '+str(report_path))
