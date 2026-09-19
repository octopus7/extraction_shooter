# ATV detached parts

Three rigid fragments are exported from `Blender/SKM_ATV.blend`: `wheel_FL`, `wheel_RR`, and `handlebar`. The existing skeletal mesh, skeleton and source Blender files are unchanged.

Each static FBX keeps the original UVs/material slots and is centered on its named bone in the reference pose. Export uses the same centimeter convention and `-Y` forward / `Z` up conversion as the original rig. Import as a static mesh with scene/unit conversion on, front-X conversion off, scale 1, and generated simple collision. Assign existing `M_ATV` and `M_ATV_DarkSteel` by material slot name.

At destruction, the runtime spawns each mesh at that bone's evaluated world transform and hides the original bone. Fragments collide with world-static terrain only, ignoring pawns, vehicles, projectiles and each other. The chassis stays a simulated body after its Chaos vehicle simulation is removed.

`TunaSweeper.Vehicle.DamageAndDestruction` compares fragment bounds against the source skeletal mesh's rigid weighted vertices and verifies physics/cleanup. All three fragments have matching bounds (0.000cm measured error) and pass the collision, detachment and cleanup assertions. The test as a whole still fails its blocked-exit rider-coordinate assertion. `-ATVDamagePreview` saves healthy, light-smoke, heavy-smoke and destroyed previews to `Saved/ATVRigWork` when rendering is enabled; final smoke visibility review remains pending.

Read-only saved-asset verification: `Tools/ATVRig/verify_damage_assets.py`. The one-off export/import generators are removed immediately after the asset commit under the project workflow.
