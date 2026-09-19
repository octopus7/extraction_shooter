"""Read-only saved asset verification; never loads or saves a level."""
import json
from pathlib import Path
import unreal

DEST = '/Game/Environment/BarrierGate'
ROOT = Path(__file__).resolve().parents[2]

def verify():
    bp = DEST + '/BP_BarrierGate'
    assert unreal.EditorAssetLibrary.does_asset_exist(bp), 'Missing placeable BP_BarrierGate'
    cls = unreal.EditorAssetLibrary.load_blueprint_class(bp)
    assert cls, 'Barrier Blueprint failed to compile/load'
    defaults = unreal.get_default_object(cls)
    for name in ['housing_mesh', 'arm_mesh', 'red_lens_mesh', 'green_lens_mesh', 'indicator_material']:
        assert defaults.get_editor_property(name), 'Missing BP binding: ' + name
    assert abs(defaults.get_editor_property('open_angle') - 85.0) < .01
    mesh_editor = unreal.get_default_object(unreal.StaticMeshEditorSubsystem)
    meshes = []
    for name in ['SM_BarrierHousing', 'SM_BarrierArm', 'SM_BarrierRedLens', 'SM_BarrierGreenLens']:
        mesh = unreal.load_asset(DEST + '/Meshes/' + name)
        assert isinstance(mesh, unreal.StaticMesh), name
        assert mesh_editor.get_number_verts(mesh, 0) > 0
        assert mesh_editor.get_num_uv_channels(mesh, 0) >= 1
        assert mesh.get_material(0), name + ' has no material'
        meshes.append(name)
    arm = unreal.load_asset(DEST + '/Meshes/SM_BarrierArm')
    bounds = arm.get_bounds()
    assert abs(bounds.box_extent.x * 2 - 350) < 1, 'Arm must extend 350cm along local +X'
    assert abs(bounds.origin.x - 175) < 1, 'Arm origin must remain at the hinge'
    tex = unreal.load_asset(DEST + '/Textures/T_BarrierHousing_BaseColor')
    assert isinstance(tex, unreal.Texture2D)
    housing = unreal.load_asset(DEST + '/Meshes/SM_BarrierHousing')
    assert mesh_editor.get_simple_collision_count(housing) > 0, 'Housing must block movement'
    assert mesh_editor.get_simple_collision_count(arm) == 0, 'Arm uses its moving actor box collider'
    mat = unreal.load_asset(DEST + '/Materials/M_BarrierHousing')
    base = unreal.MaterialEditingLibrary.get_material_property_input_node(mat, unreal.MaterialProperty.MP_BASE_COLOR)
    assert isinstance(base, unreal.MaterialExpressionTextureSample), 'Housing must sample the dedicated atlas'
    assert base.get_editor_property('texture') == tex
    assert base.get_editor_property('const_coordinate') == 0, 'Housing uses authored UV0'
    for suffix, expected_x in [('RedLens', -9), ('GreenLens', 9)]:
        lens = unreal.load_asset(DEST + '/Meshes/SM_Barrier' + suffix)
        center = lens.get_bounds().origin
        assert abs(center.x-expected_x) < .1 and abs(center.y-26.8) < .1 and abs(center.z-79) < .1, ('LED must align with control housing', suffix, center)
    result = dict(passed=True, blueprint=bp, meshes=meshes, maps_created=0)
    output = ROOT / 'TunaSweeper/Saved/Automation/BarrierGate/assets.json'
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2), encoding='utf-8')
    unreal.log('BARRIER_GATE_VERIFY_PASSED ' + json.dumps(result))

verify()
