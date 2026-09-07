# Extraction marker props

Three reusable visual props identify an extraction point: a portable beacon, an inclined direction sign and the existing modular-interior emergency light. They add no extraction condition, timer, transition or gameplay feature. Source art lives in `TunaSweeper/SourceArt/Environment/ExtractionMarkers`; new UE assets live under `/Game/Interaction/ExtractionMarkers`. Existing game maps, Blueprints, strings and authoring data are unchanged.

## Existing extraction behavior

The baseline is `TunaSweeper/Source/TunaSweeper/Private/Interaction/TunaSweeperExtractionPointActor.cpp` and its corresponding public header. `ExtractionArea` has no collision or overlap events. Extraction eligibility uses the pawn's horizontal distance from the actor origin; the displayed procedural ring is also noncolliding. Keep the extraction actor's origin and its central approach area clear.

Native fallback settings are radius 300cm, hold time 4s, ring width 4.8cm, ring height 3cm and 96 segments. The ring has 192 triangles and green linear color `(0.05, 0.95, 0.24, 0.82)`. These are native defaults, not an assertion about every gameplay instance. The spawn subsystem accepts JSON overrides for radius, hold time, ring width and particle system, and defaults to the native extraction class. Historical gameplay instructions changed radius to 200cm and ring width to 2cm; the public gameplay-spawn JSON arrays were empty when audited. The dedicated review scene explicitly uses radius **300cm**.

`/Game/Interaction/BP_ExtractionPoint` and both `/Game/FX/NS_ExtractionSmoke` and `/Game/Effects/NS_ExtractionSmokeSignal` exist. The latest implementation logs identify `/Game/FX/NS_ExtractionSmoke` as the gameplay smoke asset; the native particle-system property itself is unset until configured. An active Niagara system suppresses the actor's 18 fallback smoke sprites. The ten green spark meshes have their own enable setting. No original smoke asset, source parameter, ring, map marker or extraction logic is edited by this prop set. The dedicated UE sample uses the existing BP and smoke; visual proof and persisted properties are recorded by the validation outputs.

The map overlay already supports a green inverted triangle, `green_inverted_triangle`, in linear color `(0.22, 0.96, 0.34, 1)`. Green symbol blocks on these props follow that established extraction palette. No game text or translation key is added. The existing top-down camera mode is arm 1500cm, pitch -88 degrees, yaw 0 and FOV 70; the source constructor's older -60-degree camera pose is not the active top-down-mode contract. Review this set from the actual steep top-down angle as well as from oblique views.

## Geometry and assembly contract

Source units are meters and UE units are centimeters. Local +X is north and the direction of the sign arrow, +Y is east, +Z is up. Exported object transforms are identity, with pivot at local origin; keep placement scale `(1,1,1)`. The source script defines the handedness conversion for FBX, and the source/FBX/UE validations check signed bounds and symbol orientation. Do not add a second axis or scale correction during import.

| Prop | Contract | UE mesh |
| --- | --- | --- |
| Portable beacon | Main body 60x60x55cm; the carrying handle extends approximately 8cm on one side. Ground pivot, low base and short body; optional simple box collision is available for deliberate placement. | `/Game/Interaction/ExtractionMarkers/Meshes/SM_EM_Beacon` |
| Direction sign | Broad face 90cm along Y and 65cm slope run toward +X, rising 20 degrees toward +X; feet establish its ground footprint. The large arrow points +X. | `/Game/Interaction/ExtractionMarkers/Meshes/SM_EM_DirectionSign` |
| Emergency light | Existing closed box, bounds X=0..40cm, Y=-12..0cm, Z=0..20cm; rear/lower/left origin. Front -Y and top +Z emit. Original geometry and material assignments are retained. | `/Game/Environment/ModularInteriorExpansion/Meshes/SM_MIE_EmergencyLight` |

The inclined sign reuses the prior sign-body approach while giving the symbol a broad upward-facing surface. It does not rely on the prior vertical sign's thin top edge. The emergency light remains an independent mesh suitable for an attachment or freestanding placement. Its source FBX is included as `Models/SM_MIE_EmergencyLight.fbx` alongside the new prop FBXs for reproducible source review; there is **no duplicate new UE emergency-light asset**.

All decorative mesh components in the sample use `NoCollision` and do not affect navigation. An optional beacon box does not imply that collision should be enabled at an extraction point. If blocking collision is deliberately enabled later, keep the whole box outside the usable radius and away from the approach path; do not use complex geometry as simple collision.

## Suggested integration

Place separate static-mesh components or actors relative to the existing extraction point. No changes to its gameplay BP or map are needed to inspect the new assets. The table is a suggested layout in centimeters, with `R` equal to the actual extraction radius and all rotations initially zero. The dedicated sample and manifest provide the final reviewed transforms.

| Prop | Relative location | Placement intent |
| --- | --- | --- |
| Beacon | `(0, R+90, 0)` | One side of the circle; leave the smoke origin and center accessible. |
| Direction sign | `(-110, R+90, 0)` | Broad upward face and arrow pointing local +X. |
| Emergency light | `(-45, R+135, 0)` | Independent position beside the pair; its rear-origin bounds must be considered if attached instead. |

For another approach direction, rotate the whole decorative group about the extraction origin, preserving clearances, and rotate the sign arrow with it. Do not move or scale the extraction actor just to reposition props. The props do not drive Niagara, radius color, map markers, hold progress or level transitions. They contain no dynamic light actors; emission provides the visible signal. Daylight and dark-environment previews compare emission on/off before any additional illumination is considered.

## Reuse and surface provenance

Only relevant asset dependencies are selectively restored from the completed modular-interior expansion at commit `8bb53750`. The source lineage is original `4f8caf5d`, visual/UV correction `7c7c9b8c`, imported UE assets `481528e5`, then generator cleanup `8bb53750`. No unrelated module, map or source change from that task is integrated.

The reused emergency light has **12 triangles and 2 material slots**: `/Game/Environment/ModularInteriorPreview/Materials/M_MI_Steel` and `/Game/Environment/ModularInteriorExpansion/Materials/M_MI_ExpansionAmber`. Its source is `TunaSweeper/SourceArt/Environment/ModularInteriorExpansion/Models/SM_MIE_EmergencyLight.fbx`. The prior `SM_MIE_ZoneSign` is the thin closed sign-body reference, with 12 triangles, dimensions 100x2x50cm and one service material. Its exact `ZONE 01` typography is not used as an extraction message.

The gray-metal treatment reuses the existing 2048-square base atlas, `/Game/Environment/ModularInteriorPreview/Textures/T_MI_Atlas`, and shared 1024-square linear dirt mask, `/Game/Environment/ModularInteriorPreview/Textures/T_MI_DirtMask`. Their source PNGs are under `TunaSweeper/SourceArt/Environment/ModularInteriorPreview/Textures`. The new 1024-square marker atlas is generated with ImageGen; the concept reference is also generated with ImageGen. Exact prompts are preserved in `concept.prompt.txt` and `texture.prompt.txt`; unmodified outputs remain in the source-art References/Textures folders. Concept images are design references; images in Previews are actual model/UE renders, with the renderer identified in their captions or names.

Basic wear is part of the base-color texture. Additional dirt uses the shared mask rather than stacked broad decals. Ordinary surfaces are Opaque. Surface materials have two reachable texture reads: atlas and dirt mask. The new green emission material has zero texture reads and an emission-strength parameter; the reused amber material likewise uses constant color/emission. No new normal or roughness texture is required. Reused dirt convention: UV0 atlas, UV1 `DirtUV` with 4m coverage, generated lightmap UV2; custom primitive data 0 controls strength (0.25 on new surfaces; existing steel remains 0.65 default), 1 scale (1 default), 2/3 offsets. Larger scale produces smaller stains.

## Verification and reproduction

The final source manifest and validation reports are the authority for measured triangles, slots, bounds, UV channels, collision shapes and texture dimensions. Measured source and FBX counts: beacon 132 triangles/2 slots, sign 60 triangles/1 slot, reused light 12 triangles/2 slots; total 204 triangles/5 slots. Beacon has one optional simple box; sign/light have none. Material texture-read counts describe the authored graphs, not total frame GPU work. No FPS, batching or draw-call claim is made.

The reproducible Blender script, `.blend`, per-model FBXs, original images, textures and multi-view previews are stored in the task's dedicated directories. Validation covers dimensions and signed axes, pivots/transforms, UVs, normals, nondegenerate triangles, closed-part geometry, material assignment, assembly clearances, FBX reload and rendered orientation. UE validation imports and saves the new assets, then reloads them in a fresh process and verifies the persisted meshes, materials, collision configuration and dedicated sample scene. Bright daylight, actual top-down, per-prop and emission on/off images support visual review.


Run Blender 4.5 with --factory-startup -b --python-exit-code 1 --python Tools/ExtractionMarkers/build_markers.py, then validate_source.py in a fresh process. EM_SKIP_RENDER=1 skips rendering; EM_RENDER_FILTER selects comma-separated preview names. Run build_gallery.py with Python. Read-only UE audits use run_unreal.ps1 -Script verify_unreal.py and -Script verify_scene_driver.py -FullEditor. The dedicated scene is /Game/Interaction/ExtractionMarkers/Maps/L_ExtractionMarkers. Reports live beside ExtractionMarkers.blend. Source and UE-import commits are separate. If a one-off editor generator is used, its generated assets and generator are first validated and committed together; the next commit removes the generator, its entry points and generator-only dependencies, followed by revalidation. No startup regeneration path remains. Remote push is not part of this task.

