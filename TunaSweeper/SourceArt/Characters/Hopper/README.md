# Hopper source assets

`Hopper.blend` assembles the original 62 static mech components with the seated Captain. The rig, mesh, materials, and seated action are linked from [the Captain authoring file](Captain/CaptainPilot_UE.blend). The source GLB checksum and assembly pivots are recorded in `HopperManifest.json`.

## Coordinates and parts

- Runtime assets use centimeters: UE +X = glTF +Z, UE +Y = glTF -X, and UE +Z = glTF +Y. This preserves readable source lettering. Original artist L nodes lie at UE +Y and R nodes at UE -Y; their names and matching joint data are preserved.
- Static components share the ground-level assembly origin. Runtime joints move these world-baked meshes around the pivots in the visual data asset. For FBX export from the original Blender frame, preserve mesh coordinates and winding, keep object scale positive, and use -Y forward / Z up. Import with Convert Scene, Convert Scene Unit, and Force Front X Axis enabled. Extra reflections cause mirrored lettering; negative object scales can also produce inside-out imported geometry.
- The original body, leg, and arm PBR texture atlases are preserved. Normal textures use a flipped green channel in Unreal.
- Leg pivots are measured estimates at the visible joint housings; the source legs did not contain a skeleton.
- The manifest includes 31 samples of the original cockpit opening animation for each of its 13 moving parts. These are delta transforms relative to the closed assembly.

## Captain pilot

The Captain has an 18-bone rig with calf joints and a `hand_r` attachment. The assembly instance uses the centimeter-native source at 0.01 scale in the meter-scale mech scene. See [Captain authoring and export](Captain/README.md) for material, rig, and FBX settings.

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

The standing pilot is approximately 70 cm tall. The gameplay capsule has a 16 cm radius and 37 cm half-height. The seated mesh origin attaches at `(17, 0, 119.7806)` cm in Unreal coordinates.

## Verification

The [assembly preview](Preview/Hopper_Assembly.png) shows the Captain in the closed cockpit. [Pilot poses and gameplay captures](Captain/Preview) show the current pilot. `ImportVerification.json` preserves the initial import measurements, including outward winding checks for 28,346 static triangles. [Captain verification](Captain/UEVerification.json) records the current pilot, rig, textures, and animation checks.

The existing standard pistol is reused at 0.45 scale. Its grip center is aligned to hand_r with the inverse reference hand rotation; the saved GunTransform remains editable in DA_HopperVisual.
