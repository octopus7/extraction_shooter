"""One-off export of existing rigid ATV parts; remove after the asset commit."""
import bpy
import json
from pathlib import Path
from mathutils import Matrix

root = Path('D:/github/extraction_shooter')
out = root / 'TunaSweeper/SourceArt/Vehicles/ATV/Debris'
out.mkdir(parents=True, exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=str(root / 'Blender/SKM_ATV.blend'))
bpy.context.scene.frame_set(1)
arm = bpy.data.objects['Armature']
report = []
for bone in ('wheel_FL', 'wheel_RR', 'handlebar'):
    sources = [o for o in bpy.context.scene.objects if o.type == 'MESH' and len(o.vertex_groups) == 1 and o.vertex_groups[0].name == bone]
    assert len(sources) == 1, (bone, [o.name for o in sources])
    source = sources[0]
    mesh = source.data.copy()
    # UE reference rotations are identity; use source vehicle axes around the hub.
    pivot = arm.matrix_world @ arm.data.bones[bone].head_local
    mesh.transform(Matrix.Translation(-pivot) @ source.matrix_world)
    part = bpy.data.objects.new('SM_ATV_Debris_' + bone, mesh)
    bpy.context.scene.collection.objects.link(part)
    bpy.ops.object.select_all(action='DESELECT')
    part.select_set(True)
    bpy.context.view_layer.objects.active = part
    path = out / (part.name + '.fbx')
    bpy.ops.export_scene.fbx(filepath=str(path), use_selection=True, object_types={'MESH'},
        global_scale=1, apply_unit_scale=True, apply_scale_options='FBX_SCALE_NONE',
        axis_forward='-Y', axis_up='Z', use_mesh_modifiers=False, bake_anim=False,
        mesh_smooth_type='FACE', path_mode='AUTO')
    report.append({'bone': bone, 'file': path.name, 'vertices': len(mesh.vertices),
        'pivot_blender_cm': list(pivot), 'materials': [m.name for m in mesh.materials]})
    bpy.data.objects.remove(part, do_unlink=True)
(out/'manifest.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
print('ATV_DAMAGE_PARTS_EXPORTED', json.dumps(report))
