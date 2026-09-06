# Tuna Warp Transition

Reusable UE 5.7 player-camera transition for TunaSweeper. It combines:

- a circular radial UV warp whose inside pinches inward and outside stretches outward;
- a three-sample outward streak pass;
- a fully covered midpoint where the actual teleport is performed;
- a world-normal, scene-depth, player-position arrival rim wave;
- an optional short shadowless point-light flash.

## Human-facing use

The normal project-wide tuning location is:

- `/TunaWarpTransition/Profiles/DA_WarpTransition_Default`
- open `Style > Timing` and edit `Close Duration` (departure/cover time) and `Open Duration` (arrival/rim fade time).

`BP_TunaSweeperGameInstance` inherits this asset through its `TunaSweeper > Warp Transition > Warp Transition Profile` setting. Runtime-created components automatically read that GameInstance profile. To make one Pawn behave differently, add `Tuna Warp Transition` to that Pawn Blueprint and set its `Transition Profile`; an explicitly assigned component profile takes priority over GameInstance.

Call `Play Warp Transition` with the actor and destination transform. Blueprint events are exposed for Started, Midpoint, and Finished. C++ callers that need the teleport result can use `PlayWarpTransitionNative`.

The editable material instances are:

- `/TunaWarpTransition/Materials/MI_PP_TunaWarpRadial_Default`
- `/TunaWarpTransition/Materials/MI_PP_TunaWarpArrivalRim_Default`

Duplicate these instances for a different visual style and assign the copies on the component. The parent materials under `/TunaWarpTransition/Generated/Internal` are committed assets and can be edited directly.

Startup generation was retired on 2026-09-06. The default profile and materials remain in plugin Content and are cooked with the plugin. Edit these saved assets directly; restore missing files from Git/LFS. The editor module identity remains available without startup mutations.

## Rendering limits

The rim pass is designed for deferred SM5/SM6 opaque and masked scene geometry. Translucent particles, UI, sky, and some unlit surfaces do not provide equivalent GBuffer normals. Use a separate VFX treatment or Custom Stencil selection when those surfaces need special handling.
