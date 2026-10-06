# Captain pilot replacement

The active Hopper pilot uses `Captain_Outfit_Refined.glb`, supplied from `Blender/Captain_Outfit_Refined/models`. The input was read without modifying it. Its SHA-256 is recorded in `RigVerification.json`.

`CaptainPilot_UE.blend` is the editable, centimeter-scale source. It contains the original 7,850 triangles, body and outfit materials, an 18-bone deform rig, normalized skin weights, and seven baked clips. The standing rabbit is 70 cm tall. The existing Hopper capsule and three combat phases are retained. The new calf joints support seated, boarding, dismounting and walking poses. `hand_r` remains the weapon attachment bone.

The six packed images and the six PNG files in `Textures` are physically 512x512. Both body and outfit have BaseColor, Normal and metallic/roughness maps. UE uses sRGB only for BaseColor, normal-map compression with green-channel inversion for Normal, and linear masks with G=roughness/B=metallic for MR. The mech's existing atlases are separate assets.

## Unreal assets

- Mesh and skeleton: `/Game/Characters/Hopper/Captain/SK_Captain_Pilot`
- Animations: `/Game/Characters/Hopper/Captain/Animations/A_Captain_*`
- Textures and materials: `/Game/Characters/Hopper/Captain/Textures` and `Materials`
- Existing `DA_HopperVisual` references the new mesh and all seven clips. Its gun transform is fitted to the new right-hand aim pose.

The clip durations remain Idle 1.6 s, Boarding 2 s, Seated 2 s, Disembark 1.4 s, Walk 0.8 s, Fire 0.5 s and Melee 0.7 s. The runtime handles boarding/dismount translation and mech attachment.

## Export and verification

The Blender authoring frame faces -Y with Z up. Export the selected mesh and armature with -Y forward, Z up, FBX All scaling, scene units 0.01 and no leaf bones. UE skeletal import uses Convert Scene, Convert Scene Unit and Force Front X Axis.

UE 5.7's FBX animation import introduced a 90-degree root rotation even though the skeletal reference root was identity. The saved animation root tracks were baked to zero translation, identity rotation and unit scale using the editor animation data controller; all body motion remains on pelvis and limb tracks. If the FBX clips are reimported, restore this in-place root contract before use. The read-only verifier rejects rotated roots and checks actual foot motion along +X.

Run `Tools/Hopper/verify_captain.py` with the project's full UE editor and `-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities -ExecutePythonScript=<absolute script path>`. It checks the persisted visual-data references, skeleton, clip lengths and poses, forward gait, gun aim, six 512px PNG/UE textures and material connections. Results go to `Saved/Hopper/captain_verification.json`. `UEVerification.json` is the verified saved-asset snapshot for this change. `RigVerification.json` records source geometry, weights and posed bounds.

The Blender file was independently reopened to check all 18 bones, seven actions, six packed 512px images and every vertex's normalized weights. UE 5.7 editor build succeeded, followed by 54 Hopper/combat tests: 52 passed, two passed with existing warnings, zero failed. `Preview/UE_*.png` are captures of mounted/ranged/melee phases from the passing PIE test. The named pose previews are Blender renders using the same 512px images.

One-off rigging/import scripts are not required by runtime and are not part of the committed source.