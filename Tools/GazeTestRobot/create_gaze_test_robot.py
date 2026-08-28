import math
from pathlib import Path

import bpy
from mathutils import Vector


SCRIPT_DIR = Path(__file__).resolve().parent
REPOSITORY_ROOT = SCRIPT_DIR.parents[1]
BLEND_PATH = REPOSITORY_ROOT / "Blender" / "SKM_GazeTestRobot.blend"
FBX_PATH = REPOSITORY_ROOT / "Blender" / "SKM_GazeTestRobot.fbx"
PREVIEW_PATH = SCRIPT_DIR / "SKM_GazeTestRobot_preview.png"


def clear_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    for datablocks in (
        bpy.data.meshes,
        bpy.data.curves,
        bpy.data.armatures,
        bpy.data.materials,
    ):
        for datablock in list(datablocks):
            if datablock.users == 0:
                datablocks.remove(datablock)


def make_material(name, color, metallic=0.0, roughness=0.5):
    material = bpy.data.materials.new(name)
    material.diffuse_color = (*color, 1.0)
    material.use_nodes = True
    principled = material.node_tree.nodes.get("Principled BSDF")
    principled.inputs["Base Color"].default_value = (*color, 1.0)
    principled.inputs["Metallic"].default_value = metallic
    principled.inputs["Roughness"].default_value = roughness
    return material


def bind_rigidly(mesh_object, armature, bone_name):
    mesh_object.parent = armature
    modifier = mesh_object.modifiers.new(name="Armature", type="ARMATURE")
    modifier.object = armature
    group = mesh_object.vertex_groups.new(name=bone_name)
    group.add(range(len(mesh_object.data.vertices)), 1.0, "REPLACE")


def add_box(name, location, dimensions, material, armature, bone_name, bevel=0.03):
    bpy.ops.mesh.primitive_cube_add(location=location)
    obj = bpy.context.object
    obj.name = name
    obj.dimensions = dimensions
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    bevel_modifier = obj.modifiers.new(name="SoftEdges", type="BEVEL")
    bevel_modifier.width = bevel
    bevel_modifier.segments = 2
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.modifier_apply(modifier=bevel_modifier.name)
    obj.data.materials.append(material)
    bind_rigidly(obj, armature, bone_name)
    return obj


def add_eye(name, x, armature, eye_material, pupil_material):
    eye_center = (x, -0.30, 1.75)
    bpy.ops.mesh.primitive_uv_sphere_add(
        segments=32,
        ring_count=16,
        radius=0.13,
        location=eye_center,
    )
    eye = bpy.context.object
    eye.name = f"{name}_globe"
    eye.data.materials.append(eye_material)
    bpy.ops.object.shade_smooth()
    bind_rigidly(eye, armature, name)

    bpy.ops.mesh.primitive_cylinder_add(
        vertices=32,
        radius=0.062,
        depth=0.018,
        location=(x, -0.426, 1.75),
        rotation=(math.radians(90.0), 0.0, 0.0),
    )
    pupil = bpy.context.object
    pupil.name = f"{name}_pupil"
    pupil.data.materials.append(pupil_material)
    bevel_modifier = pupil.modifiers.new(name="PupilBevel", type="BEVEL")
    bevel_modifier.width = 0.008
    bevel_modifier.segments = 2
    bpy.context.view_layer.objects.active = pupil
    bpy.ops.object.modifier_apply(modifier=bevel_modifier.name)
    bind_rigidly(pupil, armature, name)


def create_armature():
    armature_data = bpy.data.armatures.new("SK_GazeTestRobot")
    # Unreal's FBX importer removes an armature object named exactly "Armature"
    # instead of promoting it to an extra skeleton bone. The authored hierarchy
    # therefore starts directly at the requested root bone.
    armature = bpy.data.objects.new("Armature", armature_data)
    bpy.context.collection.objects.link(armature)
    bpy.context.view_layer.objects.active = armature
    armature.select_set(True)
    bpy.ops.object.mode_set(mode="EDIT")

    root = armature_data.edit_bones.new("root")
    root.head = (0.0, 0.0, 0.0)
    root.tail = (0.0, 0.0, 0.20)

    body = armature_data.edit_bones.new("body")
    body.head = (0.0, 0.0, 0.20)
    body.tail = (0.0, 0.0, 1.30)
    body.parent = root

    neck = armature_data.edit_bones.new("neck")
    neck.head = (0.0, 0.0, 1.30)
    neck.tail = (0.0, 0.0, 1.50)
    neck.parent = body

    head = armature_data.edit_bones.new("head")
    head.head = (0.0, 0.0, 1.50)
    head.tail = (0.0, 0.0, 1.95)
    head.parent = neck

    for bone_name, x in (("left_eye", -0.20), ("right_eye", 0.20)):
        eye = armature_data.edit_bones.new(bone_name)
        eye.head = (x, -0.30, 1.75)
        eye.tail = (x, -0.50, 1.75)
        eye.parent = head
        eye.use_connect = False

    bpy.ops.object.mode_set(mode="OBJECT")
    armature.show_in_front = True
    armature.data.display_type = "OCTAHEDRAL"
    return armature


def create_robot():
    clear_scene()
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.length_unit = "METERS"
    scene.unit_settings.scale_length = 1.0

    body_material = make_material("M_GazeRobot_Body", (0.12, 0.30, 0.38), metallic=0.65, roughness=0.32)
    head_material = make_material("M_GazeRobot_Head", (0.24, 0.52, 0.58), metallic=0.50, roughness=0.28)
    neck_material = make_material("M_GazeRobot_Neck", (0.08, 0.12, 0.15), metallic=0.75, roughness=0.40)
    eye_material = make_material("M_GazeRobot_Eye", (0.72, 0.94, 0.92), metallic=0.10, roughness=0.18)
    pupil_material = make_material("M_GazeRobot_Pupil", (0.01, 0.02, 0.025), metallic=0.05, roughness=0.22)

    armature = create_armature()
    add_box("body_mesh", (0.0, 0.0, 0.75), (0.72, 0.48, 1.00), body_material, armature, "body", 0.055)
    add_box("neck_mesh", (0.0, 0.0, 1.38), (0.24, 0.24, 0.24), neck_material, armature, "neck", 0.035)
    add_box("head_mesh", (0.0, 0.0, 1.75), (0.82, 0.56, 0.56), head_material, armature, "head", 0.065)
    add_eye("left_eye", -0.20, armature, eye_material, pupil_material)
    add_eye("right_eye", 0.20, armature, eye_material, pupil_material)

    head_front_y = -0.28
    eye_center_y = -0.30
    eye_radius = 0.13
    embedded_depth = head_front_y - (eye_center_y + eye_radius)
    exposed_depth = (eye_center_y - eye_radius) - head_front_y
    assert embedded_depth < -0.05, "Eye globes must overlap the head volume"
    assert exposed_depth < -0.05, "Eye globes must remain visibly exposed"

    bpy.ops.object.select_all(action="DESELECT")
    for obj in bpy.context.scene.objects:
        if obj.type in {"ARMATURE", "MESH"}:
            obj.select_set(True)
    bpy.context.view_layer.objects.active = armature

    BLEND_PATH.parent.mkdir(parents=True, exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(BLEND_PATH))
    bpy.ops.export_scene.fbx(
        filepath=str(FBX_PATH),
        use_selection=True,
        object_types={"ARMATURE", "MESH"},
        apply_scale_options="FBX_SCALE_UNITS",
        use_space_transform=True,
        bake_space_transform=False,
        axis_forward="-Y",
        axis_up="Z",
        add_leaf_bones=False,
        use_armature_deform_only=True,
        bake_anim=False,
        mesh_smooth_type="FACE",
        path_mode="AUTO",
    )
    print(f"Created {BLEND_PATH}")
    print(f"Created {FBX_PATH}")

    bpy.ops.object.select_all(action="DESELECT")
    bpy.ops.object.camera_add(location=(2.8, -4.8, 2.35))
    camera = bpy.context.object
    camera.name = "PreviewCamera"
    camera.rotation_euler = (math.radians(78.0), 0.0, math.radians(30.0))
    direction = Vector((0.0, -0.05, 1.15)) - camera.location
    camera.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()
    scene.camera = camera

    bpy.ops.object.light_add(type="AREA", location=(-2.5, -3.0, 4.5))
    key_light = bpy.context.object
    key_light.data.energy = 900.0
    key_light.data.shape = "DISK"
    key_light.data.size = 4.0
    key_light.rotation_euler = (
        Vector((0.0, 0.0, 1.2)) - key_light.location
    ).to_track_quat("-Z", "Y").to_euler()

    bpy.ops.object.light_add(type="AREA", location=(3.0, 0.5, 2.5))
    fill_light = bpy.context.object
    fill_light.data.energy = 550.0
    fill_light.data.size = 3.0
    fill_light.rotation_euler = (
        Vector((0.0, 0.0, 1.3)) - fill_light.location
    ).to_track_quat("-Z", "Y").to_euler()

    scene.render.engine = "BLENDER_EEVEE_NEXT"
    scene.render.resolution_x = 640
    scene.render.resolution_y = 640
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.filepath = str(PREVIEW_PATH)
    scene.world.color = (0.025, 0.035, 0.045)
    bpy.ops.render.render(write_still=True)
    print(f"Rendered {PREVIEW_PATH}")


if __name__ == "__main__":
    create_robot()
