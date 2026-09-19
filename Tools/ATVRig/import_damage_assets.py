"""One-off import of ATV debris and smoke variants; remove after the asset commit."""
import unreal
from pathlib import Path

root = Path('D:/github/extraction_shooter/TunaSweeper/SourceArt/Vehicles/ATV/Debris')
destination = '/Game/Meshes/Props/ATV/Debris'
tools = unreal.AssetToolsHelpers.get_asset_tools()
material = unreal.load_asset('/Game/Meshes/Props/ATV/M_ATV')
for source in sorted(root.glob('*.fbx')):
    task = unreal.AssetImportTask()
    task.filename = str(source)
    task.destination_path = destination
    task.destination_name = source.stem
    task.automated = True
    task.replace_existing = True
    task.save = False
    options = unreal.FbxImportUI()
    options.import_mesh = True
    options.import_as_skeletal = False
    options.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
    options.automated_import_should_detect_type = False
    options.import_materials = False
    options.import_textures = False
    options.static_mesh_import_data.combine_meshes = True
    options.static_mesh_import_data.auto_generate_collision = True
    options.static_mesh_import_data.normal_import_method = unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS
    options.static_mesh_import_data.convert_scene = True
    options.static_mesh_import_data.convert_scene_unit = True
    options.static_mesh_import_data.force_front_x_axis = False
    options.static_mesh_import_data.import_uniform_scale = 1.0
    task.options = options
    tools.import_asset_tasks([task])
    mesh = unreal.load_asset(destination + '/' + source.stem)
    assert isinstance(mesh, unreal.StaticMesh), source
    for index, slot in enumerate(mesh.get_editor_property('static_materials')):
        name = str(slot.material_slot_name)
        resolved = unreal.load_asset('/Game/Meshes/Props/ATV/' + name)
        mesh.set_material(index, resolved or material)
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    unreal.log('ATV_DEBRIS_IMPORTED ' + mesh.get_path_name() + ' bounds=' + str(mesh.get_bounds()))

for stage in ('Light', 'Heavy'):
    source = '/Game/Effects/NS_ExplosiveBarrel_Smoke' + stage
    target = '/Game/Effects/ATV/NS_ATV_DamageSmoke' + stage
    asset = unreal.EditorAssetLibrary.duplicate_asset(source, target)
    assert isinstance(asset, unreal.NiagaraSystem), target
    unreal.EditorAssetLibrary.save_loaded_asset(asset)
unreal.log('ATV_DAMAGE_ASSETS_IMPORTED')
