# Wall Coping generation source

`wall_coping_parameters.json` is the editable source for the deterministic Unreal
`FMeshDescription` generator in `TunaSweeperWallCopingSetup.cpp`.

Run the UE 5.7 editor with:

`-TunaSweeperWallCopingSetup -TunaSweeperWallCopingSetupQuit`

The generated block uses local X for its repeat length, local Y for wall width,
and local Z for up. Its pivot is the bottom center (`X=0, Y=0, Z=0`), which lets a
designer place the spline directly on the wall top and change height without
introducing an offset. End caps are generated from the same cross-section, so
neighboring blocks at zero gap remain closed even though the end bevel creates a
readable mortar-like seam.
