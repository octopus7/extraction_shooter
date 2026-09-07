# Tuna Weapon Collection

Six additive weapon assets for TunaSweeper: Standard and Premium variants for SMG, AR, and Pistol. Standard models use a contemporary tactical language; Premium models keep the same believable firearm construction while moving the core silhouette and modular equipment modestly into the near future.

## Unreal assets

- Destination: `/Game/Weapons/TunaWeaponCollection`
- Showcase: `/Game/Weapons/TunaWeaponCollection/Maps/L_TunaWeaponCollection_Showcase`
- Forward/right/up: `+X / +Y / +Z`
- Required sockets: `MuzzleSocket`, `LaserSightSocket`, `ShellEjectionSocket`
- Every mesh has UV0, UV1, three simple collision hulls, identity import scale, and semantic material slots.

## Editable colors

The master material is `M_TunaWeapon_Master`. Each semantic surface is a separate `MaterialInstanceConstant` under the collection's `Materials` folder. Open any instance and override its `Tint` vector parameter to recolor that surface without changing the atlas or mesh. `Metallic`, `Roughness`, and `EmissionStrength` are also exposed per instance.

The shared atlas is `T_TunaWeaponCollection_Atlas`. Its 16 cells were generated as a single ImageGen texture and are assigned through atlas-safe UV islands. The atlas color is multiplied by each material instance's `Tint`.

## Editable source

- `TunaWeaponCollection_SMG.blend`
- `TunaWeaponCollection_AR.blend`
- `TunaWeaponCollection_Pistol.blend`
- `Models/`: FBX exports
- `References/`: selected ImageGen references, discarded direction iterations, and exact prompts
- `Textures/`: generated atlas and exact prompt
- `Manifests/`: per-family metadata; `model_manifest.json` is the merged UE contract
- `Previews/`: model review angles and the six-weapon comparison sheet

Run the retained checks with Blender 4.5 using `Tools/WeaponModels/verify_fbx.py`. After the one-off Unreal importer is removed, `Tools/WeaponModels/verify_unreal.py` remains as the read-only fresh-process reload validator.
