# Demo water intake source assets

The user's `screen_reference.png` supersedes the cylindrical screen in the initial
concept. The new screen is a flat vertical bar grate in tapered concrete sluice
abutments, with a maintenance walkway, stairs, rails and screw hoist. A compact
pump, control cabinet and repair valve sit on the bank beside the gate.

## Rebuild

Run from any directory with Blender 4.5 LTS:

```powershell
& 'C:\Program Files\Blender Foundation\Blender 4.5\blender.exe' --background --factory-startup --python-exit-code 1 --python 'D:\github\extraction_shooter\Tools\WaterIntakeDemo\build_water_intake.py'
```

Output: `TunaSweeper/SourceArt/Environment/WaterIntakeDemo/`.
The script creates a new scene and never opens or overwrites the previous intake
models. `WaterIntakeDemo.blend` packs the generated texture and retains a review
camera. `PREVIEW_ONLY_NOT_EXPORTED` contains the studio ground, lights and camera;
these are absent from the FBX files. No water surface or terrain is included.

## Meshes

| Name suffix after `SM_IntakeDemo_` | Contents / use |
| --- | --- |
| `GateFrame` | Concrete sluice, stairs, walkway, rails, guides and fixed screw hoist |
| `Screen` | Flat vertical-bar screen; separate from the concrete frame |
| `ScreenDebris` | Removable branches and leaves on the upstream face |
| `Pump` | Pump, motor, cabinet, connected piping, valve body and permanent stem |
| `PumpFoundation` | Concrete pump pad and discharge anchor |
| `ValveHandle_Broken` | Damaged orange wheel hub and stubs |
| `ValveHandle_Repaired` | Complete orange replacement wheel |

All meshes except the two handles share the gate-centered origin. Place those
components at the same transform with unit scale. The handles have local pivots
at the valve spindle: Blender assembly location `(-3.15, 1.56, 1.88)` meters.
Only one handle should be visible at a time. The saved preview shows the repaired
handle and debris; the other handle remains a separate hidden object.
The original Blender coordinates, dimensions, triangle counts, material slots and
convex collision counts are recorded in `model_manifest.json`. UE placement values
must be read from the import verification report, which measures the FBX axis
conversion rather than assuming it.

The gate opening is approximately 2.6 m wide and 1.8 m tall between frame members;
the top of the hoist is 3.61 m above the shared origin. This is a visual game prop,
not a hydraulically specified installation. Gate animation and gameplay are not
implemented by these assets.

## ImageGen texture

`Textures/T_WaterIntakeDemo_Atlas_BaseColor.png` is the original unmodified built-in
ImageGen output. The exact prompt is `texture_atlas.prompt.txt`.
Its 4 x 2 material regions are blue paint, steel, concrete, ivory paint, orange
paint, foliage, wood and rubber. Each UV island stays inside its tile with a 9%
inset to avoid neighboring colors bleeding into mipmaps. Five materials share
this sRGB base-color texture with scalar roughness and metallic values; no normal,
roughness or other texture is synthesized by a script.

## Validation

The builder checks finite vertices, UV coverage within 0–1, absence of degenerate
triangles and a 120,000-triangle ceiling. Explicit convex collision boxes are
exported for the fixed structures and the screen, preserving the gate opening.
Debris and interchangeable handles have no simple collision. Four Cycles renders
cover assembly, front, back and clean-screen/broken-valve states.

No Blueprint, level, quest authoring data or existing intake asset is changed.
