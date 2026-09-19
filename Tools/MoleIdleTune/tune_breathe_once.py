"""One-off UE editor asset adjustment. Remove after committing with its output."""
import math
import unreal

clip = unreal.load_asset('/Game/Characters/NPC/Mole/A_Mole_Idle_Breathe')
model = clip.get_editor_property('data_model_interface')
controller = clip.get_editor_property('controller')
count = model.get_number_of_keys()
assert count == 121
names = [str(n).lower() for n in model.get_bone_track_names()]
before = {n: [unreal.AnimationLibrary.get_bone_pose_for_time(clip, n, i / 30.0, False)
              for i in range(count)] for n in names}


def q(qt):
    return [qt.x, qt.y, qt.z, qt.w]


def multiply(a, b):
    x, y, z, w = a
    X, Y, Z, W = b
    return [w*X+x*W+y*Z-z*Y, w*Y-x*Z+y*W+z*X,
            w*Z+x*Y-y*X+z*W, w*W-x*X-y*Y-z*Z]


def relative(a, b):
    return multiply([-a[0], -a[1], -a[2], a[3]], b)


def angle(a, b):
    delta = relative(a, b)
    return 2 * math.atan2(math.sqrt(sum(v*v for v in delta[:3])), abs(delta[3]))


arms = ['upper_arm_l', 'upper_arm_r', 'forearm_l', 'forearm_r']
for bone in arms:
    keys = before[bone]
    base = q(keys[0].rotation)
    if bone.startswith('upper_arm'):
        peak = max((q(k.rotation) for k in keys), key=lambda value: angle(base, value))
        old_angle = angle(base, peak)
        assert math.radians(.05) < old_angle < math.radians(.5), 'Already tuned or unexpected source'
        delta = relative(base, peak)
        length = math.sqrt(sum(v*v for v in delta[:3]))
        axis = [v / length for v in delta[:3]]
        amplitude = math.radians(3.0)
    else:
        axis = [1.0, 0.0, 0.0]
        amplitude = math.radians(1.0)
    rotations = []
    for frame in range(count):
        # Same four-second breathing cycle; zero velocity at both loop endpoints.
        breath = .5 - .5 * math.cos(2 * math.pi * frame / (count - 1))
        half = .5 * amplitude * breath
        delta = [v * math.sin(half) for v in axis] + [math.cos(half)]
        rotations.append(unreal.Quat(*multiply(base, delta)))
    assert controller.set_bone_track_keys(bone, [k.translation for k in keys], rotations,
                                          [k.scale3d for k in keys], False)

for bone in names:
    after = [unreal.AnimationLibrary.get_bone_pose_for_time(clip, bone, i / 30.0, False)
             for i in range(count)]
    for old, new in zip(before[bone], after):
        assert (old.translation-new.translation).length() < .00001
        assert (old.scale3d-new.scale3d).length() < .00001
        if bone not in arms:
            assert angle(q(old.rotation), q(new.rotation)) < .00001
    assert angle(q(after[0].rotation), q(after[-1].rotation)) < .00001
    if bone in arms:
        span = max(math.degrees(angle(q(after[0].rotation), q(k.rotation))) for k in after)
        expected = 3.0 if bone.startswith('upper_arm') else 1.0
        assert abs(span-expected) < .01
        print('TUNED_ARM_DEGREES', bone, span)
assert clip.get_editor_property('force_root_lock')
assert unreal.EditorAssetLibrary.save_loaded_asset(clip, False)
print('SAVED', clip.get_path_name())
