# Title shadow lighting

The title studio's `CharacterKeyLight` is a movable SpotLight. Its inner/outer
half-cone angles are 20/28 degrees and its source radius is 22 cm. Intensity,
color, position and attenuation radius retain the previous key-light values.
The light aims at the title body mesh origin plus 100 cm vertically, including
when the presentation actor or studio is moved. It does not aim at the
presentation actor's root, which is offset from the character.

The smaller projection concentrates shadow detail around the character. The
finite light source softens VSM penumbras. `EmptyWallLight` retains its fill
illumination but does not cast an additional hard self-shadow. These settings
belong to `ATunaSweeperTitleStudioActor`; no global VSM quality variables or
gameplay lights are changed.

The existing saved BP_TitleStudio and IntroMap load the updated native light
component and defaults without an asset migration or startup regeneration.

## Validation

Run `Tools/TitleMotions/verify_title_shadows.py` in a separate UE 5.7 editor with
`-ExecutePythonScript`. It runs the saved IntroMap in PIE for 12 seconds, checks
the effective light type/soft source/fill shadow state, and samples the head,
pelvis, hands and feet against the fully illuminated inner cone and range.
It saves an actual game viewport screenshot, then exits without saving assets.

Outputs are under `TunaSweeper/Saved/Automation/TitleShadows/`:

- `verification.json`: runtime coverage and effective settings.
- `TitleShadows_Final.png`: final rendered viewport.

The initial comparison also captured point-light shadows, no direct shadows,
hard spotlight shadows, and 12/22 cm sources in one paused pose. Use the game
viewport for judging VSM and final post-processing; SceneCapture2D produces a
different lighting result. Mesh normals and authored texture edges are not
smoothed by increasing the shadow source radius.
