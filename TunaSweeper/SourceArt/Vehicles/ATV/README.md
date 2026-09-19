# ATV skeletal asset

The existing `Blender/SM_ATV.blend` and `/Game/Meshes/Props/ATV/SM_ATV` remain unchanged. The authored source is `Blender/SKM_ATV.blend`; the engine asset is `/Game/Meshes/Props/ATV/SKM_ATV` with `SKM_ATV_Skeleton`.

The original connected mesh was separated into chassis, four wheel assemblies, and the handlebar cluster while retaining its exterior UVs and atlas material. Cut interior faces were capped. The fused, partially occluded suspension connections were replaced by articulated upper/lower arms, knuckles, shock cylinders/pistons, and visible coils. The source vehicle's asymmetric exterior proportions were retained.

## Units and axes

- Dimensions: approximately 178.943 × 120.228 × 127.621 cm (length × width × height), matching the existing UE static mesh dimensions after orienting its front to +X.
- Ground is Z=0 in the new asset. The old static-mesh pivot is not reused.
- UE: +X forward (round headlights), +Y right, +Z up; identity root/wheel reference rotations and scale 1.
- Blender authoring uses centimeters (`unit_scale=0.01`), +X forward, +Y left, +Z up. The exported FBX converts the lateral axis for UE.
- FBX export: `-Y` forward / `Z` up, primary bone `X`, secondary bone `-Y`, no leaf bones, no object scale, no baked space transform. The supplied FBX already contains the textures.

## Bones and rigid parts

35 bones; 50 editable mesh objects in the Blender source. Each mesh vertex has exactly one weight of 1. The FBX imports as one Skeletal Mesh with four material slots.

| Bones | Purpose |
| --- | --- |
| `root` | Chassis and vehicle origin |
| `wheel_FL`, `wheel_FR`, `wheel_RL`, `wheel_RR` | Wheel hubs; direct children of root for Chaos wheel setup |
| `handlebar` | Steering handlebar and gauge cluster |
| `lower_arm_*`, `upper_arm_*` | Suspension links |
| `knuckle_*` | Upright/hub support; follows wheel translation/steering but not tire spin |
| `shock_upper_*`, `shock_lower_*` | Rigid shock cylinder and sliding piston |
| `spring_*` | Visible coil compression/extension |
| `seat`, `grip_l`, `grip_r`, `foot_l`, `foot_r` | Attachment/IK reference bones |

Root and wheel positions are recorded in `unreal_validation.json`. These are the measured source-specific hub centers, not a perfectly symmetric axle layout. Wheel collision radii and physical travel must be tuned against the actual mesh during vehicle setup.

## Inspection animation

`ATV_RigCheck` is embedded in the Blender file and exported separately as `ATV_RigCheck.fbx`; UE asset: `/Game/Meshes/Props/ATV/AN_ATV_RigCheck`.

The 4-second, 30 fps clip turns the front wheels/handlebar through ±24 degrees, rotates the tires twice, and alternates wheel travel through ±7 cm. Shock cylinders and pistons remain rigid, coils change length, and link endpoints follow the wheel hubs. The small link scaling used in this visual inspection clip is not a rigid-link mechanical solver.

This is a baked asset-check animation, **not** runtime Chaos simulation. A future vehicle AnimBP/Control Rig must drive the four wheel bones from Chaos and drive the additional suspension/handlebar bones from the same wheel state. No driving Pawn, player mounting code, X-key UI, Physics Asset, or runtime vehicle animation graph is introduced by this asset task.

## Materials

`M_ATV` remains the existing atlas material. New solid mechanical materials are `M_ATV_DarkSteel`, `M_ATV_Chrome`, and `M_ATV_ShockGold`, all in the ATV content folder. No existing material/texture asset was modified.

## Validation

- `blender_validation.json`: source hash preservation, rigid weights, root/wheel axes, posed-wheel rigidity, and fresh FBX import bounds comparison.
- `unreal_validation.json`: fresh UE 5.7 process reload, all 35 bones, root/wheel scale and orientation, hub positions, ±7 cm wheel travel, front-only steering, rigid shock bodies, coil scaling, and resolved materials.
- `Previews`: Blender-rendered bind and articulated poses; these are not screenshots of runtime Chaos driving.

Read-only checks remain in `Tools/ATVRig/verify_blender.py` and `Tools/ATVRig/verify_unreal.py`. One-off build/import generators are removed immediately after their asset commit, per project policy. For later edits, use the saved Blender source and reimport its FBX with skeletal import, normals import, scene/unit conversion on, front-X conversion off, scale 1, and animation import off. Reimport the separate inspection clip only when intentionally updating it.
