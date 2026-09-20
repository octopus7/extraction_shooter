# Title background depth of field

The title background is a color/composition layer. Its fine tree, crate and
landscape details are intentionally defocused, while the character's face stays
in focus.

`ATunaSweeperTitleStudioActor` sets DOF only on its presentation camera:

- `BackdropFStop`: 2.8 by default, editable on the studio.
- Sensor width: 36 mm.
- Focus distance: the head socket projected along the camera forward direction.

Focus is updated with the title camera, including main/submenu transitions.
No gameplay camera or global post-processing quality setting is changed.
Normal engine DOF quality settings still apply.

`M_TitleMatteLake` remains Unlit and Translucent. It now renders Before DOF and
enables Output Depth and Velocity, so the backdrop participates in depth-aware
blur. Its existing screen projection, exposure correction and peripheral mip
blur are retained. The source image and texture are unchanged.

## Verification

Run `Tools/TitleMotions/verify_title_dof.py` with `-ExecutePythonScript` in a
separate UE 5.7 editor. It loads the saved material, checks the main/submenu focus
distance during PIE, captures both views and exits without saving assets.
Results live in `TunaSweeper/Saved/Automation/TitleDOF/`:

- `verification.json`
- `TitleDOF_Final.png`
- `TitleDOF_Submenu.png`

F/4, F/2.8 and F/1.8 were compared in a transient fixed-pose test. F/2.8 removes
background detail while preserving the character. Keep the camera ticking when
comparing post-process settings; pausing the entire world can leave the cached
camera view unchanged.
