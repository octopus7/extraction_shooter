import unreal, json
from pathlib import Path
out = Path('D:/github/extraction_shooter/TunaSweeper/Saved/TitleMotions')
out.mkdir(parents=True, exist_ok=True)
base = '/Game/Characters/Player/LunaMk2/'
report = {}
for name in ['SKM_LunaMk2', 'SKM_LunaFace', 'Skirt/SKM_LunaMk2_TitleSkirt']:
    mesh = unreal.load_asset(base + name)
    assert mesh, name
    skeleton = mesh.get_editor_property('skeleton')
    pose = unreal.AnimPoseExtensions.get_reference_pose(skeleton)
    names = unreal.AnimPoseExtensions.get_bone_names(pose)
    bones = {}
    for bone in names:
        tr = unreal.AnimPoseExtensions.get_bone_pose(pose, bone, unreal.AnimPoseSpaces.WORLD)
        bones[str(bone)] = {'location': list(tr.translation.to_tuple()), 'rotation': [tr.rotation.x, tr.rotation.y, tr.rotation.z, tr.rotation.w]}
    report[name] = {'skeleton': skeleton.get_path_name(), 'bones': bones}
    task = unreal.AssetExportTask()
    task.object = mesh
    task.filename = str(out / (mesh.get_name() + '.fbx'))
    task.automated = True
    task.prompt = False
    task.replace_identical = True
    task.exporter = unreal.SkeletalMeshExporterFBX()
    task.options = unreal.FbxExportOption()
    assert unreal.Exporter.run_asset_export_task(task), name
(out / 'ue_rigs.json').write_text(json.dumps(report, indent=2))
unreal.log('TITLE_MOTION_INSPECTION_OK')
