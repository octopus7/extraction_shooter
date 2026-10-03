# Hopper source assets

`Hopper.blend` is the editable assembly source, containing the original textured mech split into 62 static components and the separately rigged Rabbit pilot. `HopperPilot_UE.blend` contains the exact centimeter-native pilot export rig, with positive unit object scales and the original mesh and bone handedness preserved. The GLB reference was read without modifying its source repository. Its SHA-256 is recorded in `HopperManifest.json`.

## Coordinates and parts

- Runtime assets use centimeters: UE +X = glTF +Z, UE +Y = glTF -X, and UE +Z = glTF +Y. This preserves readable source lettering. Original artist L nodes lie at UE +Y and R nodes at UE -Y; their names and matching joint data are preserved.
- Static components share the ground-level assembly origin. Runtime joints move these world-baked meshes around the pivots in the visual data asset. For FBX export from the original Blender frame, preserve mesh coordinates and winding, keep object scale positive, and use -Y forward / Z up. Import with Convert Scene, Convert Scene Unit, and Force Front X Axis enabled. Extra reflections cause mirrored lettering; negative object scales can also produce inside-out imported geometry.
- The original body, leg, and arm PBR texture atlases are preserved. Normal textures use a flipped green channel in Unreal.
- Leg pivots are measured estimates at the visible joint housings; the source legs did not contain a skeleton.
- The manifest includes 31 samples of the original cockpit opening animation for each of its 13 moving parts. These are delta transforms relative to the closed assembly.

## Rabbit pilot

The source Rabbit was a single mesh of disconnected anatomical islands. It now has a 16-bone rig. Rigid island weights preserve the original low-poly appearance, eyes, ears, clothing, and paws. The pilot skeleton includes `hand_r` for a weapon attachment.

All seven clips are in place at 30 fps:

| Clip | Seconds | Intended use |
| --- | ---: | --- |
| Idle | 1.6 | Standing loop |
| Boarding | 2.0 | Limb motion while the component moves into the seat |
| Seated | 2.0 | Seated loop |
| Disembark | 1.4 | Limb motion while the component exits |
| Walk | 0.8 | Walking loop |
| Fire | 0.5 | Aiming and recoil |
| Melee | 0.7 | Arm swing |

The root stays at zero; a pelvis offset keeps the lowest foot point at floor height. The standing pilot is approximately 70 cm tall; its walk reaches 72.11 cm. A capsule radius of 16 cm and half-height of 37 cm covers these poses. The seated mesh origin attaches at `(17, 0, 119.7806)` cm in the mech assembly.

In `Hopper.blend` the pilot is displayed at its seat location in the original meter-scale authoring frame. Use `HopperPilot_UE.blend` for skeletal re-export: its mesh and rig stay at zero with object scale 1, scene unit scale 0.01, and 16 bones. Export selected armature and mesh with -Y forward, Z up, Apply Scalings = FBX All, and no leaf bones. Import with Convert Scene, Convert Scene Unit, and Force Front X Axis enabled. Reference and gameplay poses use the same skeleton; there is no runtime regeneration.

## Verification

The `Preview` images show the assembled source and sample pilot poses. `ImportVerification.json` records imported Unreal counts, clip lengths, coordinate checks, DirectX 12 SM6 compilation for all 14 materials, and outward winding checks for all 28,346 static triangles. The UE_MechWalking, UE_PilotRanged, and UE_PilotMelee previews are captures from the passing in-editor gameplay test; the other previews show Blender source poses.

The existing standard pistol is reused at 0.45 scale. Its grip center is aligned to hand_r with the inverse reference hand rotation; the saved GunTransform remains editable in DA_HopperVisual.
