# Shared canopy rim implementation plan

> Execute inline with the executing-plans and verification-before-completion skills. The user approved the shared mask design and replacement of the old rim. Keep all implementation, validation, cleanup and logs in one task commit.

**Goal:** Replace the card-pivot ellipsoid tint with soft saturated paint derived from the visible wind-deformed canopy silhouette.

**Architecture:** Reuse CustomDepth/stencil with one ID per tree and shared view-local RDG buffers. Downsample into a mask at ceil(view width/4) by ceil(view height/4), close small leaf holes, compute distance to the closed silhouette, and composite only onto visible leaf pixels. All components in a tree share one ID; shadow proxies and trunks do not participate. No SceneCapture actor or per-tree render target.

**Tech stack:** UE 5.7, C++, global shaders/RDG, an early-loading runtime rendering module. Existing leaf WPO/opacity provides wind and card density. Existing full-resolution CustomDepth remains the source; the auxiliary masks and distance calculation are quarter resolution per axis.

**Constraints:** Preserve the editable clumps, tree gradient, bark, bushes and user-edited RaidMap. No demo/channel-specific logic, UI or saved gameplay data. Retire old ellipsoid CPD, shader nodes, controls and previews. Add adjustable screen-space width (pixels), strength, color and brightness. IDs 16–255 are reserved for foliage, leaving existing 1–3 outlines alone. Exhaustion disables the extra effect safely, rather than merging trees.

**Limits:** A single visible ID/depth layer does not reconstruct occluded crowns. Morphological closing suppresses small holes, not arbitrarily large holes; expose its radius globally. This is a soft interior paint, not an external outline. No GPU timing claims without measurements.

## Review focus

- Disabled/destroyed/duplicated trees release IDs; world teardown and PIE do not leave render-thread UObject references.
- Overlapping trees with different colors remain separated; foreground geometry and non-foliage stencils are unchanged.
- Nonzero view origins, odd dimensions and scene captures use correct screen/depth coordinates.
- Wind/density/scale affect both visible leaves and their mask, including zero density.
- Existing gradient and independent per-tree controls still work; no stale shader generator or old radial fallback remains.

## Work

- [x] Add a failing shared-stencil contract test and run it against the old implementation.
- [x] Restore the pre-rim leaf material; remove radial mapping and integrate actor registration with a world-owned shared renderer.
- [x] Add quarter-resolution mask, gap closing, distance and depth-aware composite shaders with BP controls.
- [x] Run editor build and automation, inspect off/on/wind/overlap/occluder images, and correct failures.
- [x] Fresh code review, remove any one-off scripts/generators, revalidate, update source-art documentation and request log, commit only this task and open the project editor.

## Validation / review ledger

- Baseline: 4e2b0644. SharedMask RED in CanopySharedRed.log: old leaves did not render custom depth and lacked distinct IDs.
- GREEN: Development Editor build; CanopySharedFinalQA.log has 6 Success / 0 Fail. Real-RHI captures: outer paint 28,335 top / 25,919 oblique changed leaf pixels, central 0 changed; overlapping colors 20,087 red / 24,099 blue pixels; occluder 0 of 124,552 gray pixels changed with 39,840 leaf pixels painted; wind mask changed 3,219 pixels.
- Independent review: review_shared_mask, no P1/P2/P3 findings. Restored leaf material blob matches 8092b579 exactly.
- Ruling: a GPU test with nonzero viewport origin is not added in this task; engine CopyFromSlice and viewport mapping were inspected, odd-size allocation is tested. A split viewport still needs visual acceptance; incorrect mapping would show misaligned paint there.
- Ruling: packaged cook, long-running PIE/streaming stress, target GPU timings and artistic/TSR acceptance are outside this editor implementation check. Early shader registration and world/ID lifecycle were reviewed and basic lifecycle is covered; packaging/platform or long-session issues remain possible until those environments are exercised.
- No temporary generators or startup asset regeneration were created. Test and capture code is permanent regression coverage.
