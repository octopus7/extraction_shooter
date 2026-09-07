# Four-way noise emitter

Source art: `TunaSweeper/SourceArt/Props/NoiseEmitter/`.

## Inspection and modeling

`build_model.py` reconstructs the original JSON geometry in Blender 4.5. Box
extents, horn basis vectors, 8 radial sides, back caps and open mouths follow
`TunaSweeperPeriodicNoiseEmitterActor.cpp`. The original runtime duplicates face
winding for two-sided rendering; the reconstruction uses Blender's two-sided
surface display. Studio lighting is illustrative, not an Unreal capture.
Original front, side and oblique renders were inspected before ImageGen and
modeling. Dimensions: 304 x 304 x 206 cm, horn axes Z154, 32cm square post,
68cm hub and four 118cm horns with 18/52cm radii.

The built-in ImageGen tool generated `reference.png` from `original_oblique.png`.
The exact prompt is in `reference_prompt.txt`. Its small front view omits the
front/back horns and mislabels the diameter as across-flats; neither error was
copied. Original geometry controls dimensions and cardinal directions. The final
model adopts thick octagonal rims, dark interiors/diaphragms, charcoal collars,
chamfered ivory pillar/hub, footplate and a simple service panel.

`author_model.py` creates editable `NoiseEmitter.blend`, body/horn FBXs and an
assembly FBX. The assembly has **800 triangles** (176 + 4 x 156), 416 geometric
vertices and four flat-color materials. Constituent shells have zero nonmanifold
edges and degenerate faces; body pieces intentionally intersect. UVs are
unwrapped, normals flat. Units: cm, +Z up; assembly pivot: ground center. Reusable
horn: +X, narrow-end pivot. Imported bounds confirm axes and scale. Final front,
side, top and oblique renders were inspected.

## Unreal integration

- `/Game/Meshes/Props/NoiseEmitter/SM_NoiseEmitter_Body` and `SM_NoiseEmitter_Horn`
  are CPU-readable source static meshes.
- `/Game/Interaction/BP_PeriodicNoiseEmitter` supports direct placement.
- The existing actor and derived test BPs use these defaults for
  `mesh.test_noise_quad_horn`. Construction/BeginPlay extract material sections
  into the procedural component, preserving the existing per-vertex pulse.
- Custom JSON IDs/paths and missing mesh assets retain the original fallback.
  No placement data, noise values, AI hearing, collision, shadows or save data
  changed. Hard UObject references allow cooking to follow the source meshes.

## Verification

Run Blender 4.5 with `--python Tools/NoiseEmitter/build_model.py`, generate and
inspect the ImageGen reference, then run `author_model.py`. Build UE 5.7 editor,
run `Tools/NoiseEmitter/run_import.ps1`, then `-VerifyOnly` in a fresh process.
Memory DDC avoids dependence on the local Zen service.

Import and reload asset reports passed (bounds, UVs, CPU access, no collision,
BP noise defaults, source references and 14 constructed sections). Both engine
process exits were 1 due to the pre-existing Niagara typed-element startup
ensure documented in `Docs/demo_build_implementation_audit_2026-09-02.md`.
The wrapper preserves this nonzero exit status.

UE 5.7.4 editor build succeeded. Automation test
`TunaSweeper.NoiseEmitter.AuthoredMeshAndPulse` passed, process exit 0. It checks
800 triangles, original bounds/pivot, original emitted event, fixed body,
deforming horns, pulse completion/restoration and JSON fallback.
`runtime_validation.json` records the result. Full logs are local under Saved.
The editor was launched. Its pre-existing one-shot setup rewrote unrelated
tracked Content assets. Automatic approval review blocked restoration even
after log matching; these unrelated edits are excluded from the noise commit
and remain pending user approval to restore. Previews are Blender renders.
