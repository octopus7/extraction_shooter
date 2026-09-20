# Luna Mk2 title motion reference

Generated with the built-in image_gen tool on 2026-09-20 and approved by the user. Authored motion assets and the editable Blender file are in the parent folder; UE assets are in /Game/Characters/Player/LunaMk2/Animations/Title/.

Authored clips: A, 4-second subtle breathing loop in the supplied leaning-forward hands-behind-back pose; B, 4-second breathing loop with shifted weight and a changed leg stance; C, 3-second rear three-quarter turn ending at A's first pose. Separate 1.6-second A-to-B and B-to-A transitions are included for later random selection. No title code, AnimBP, or map integration in this task.

## Generation prompt

Later arm correction (2026-09-21): the user rejected the crossed hands behind the back as looking restrained. All five current clips now use relaxed arms open diagonally downward beside the body, with softly bent elbows and wrists aligned with the forearms. The generated reference sheet below is historical; the OpenArms front/side/back previews show the current pose.

Historical prompt below. On 2026-09-21 the user corrected C using a side-view screenshot: mirror its facing direction, start in screen-left profile, and turn to A in the opposite direction. Current C uses a 90-degree side-to-front arc, replacing the original 140-degree rear view. A/B and their connecting clips remain unchanged. Current title integration is documented in `Docs/title_standalone_motions.md`.

Use case: stylized-concept.
Create one polished animation pose reference sheet, wide landscape, with four clearly separated equal panels, full body including feet in every panel, quiet warm light gray studio background and subtle floor contact shadows, no scenery or game UI. Input image is the character identity, clothing and PRIMARY A POSE reference, not an edit target.
Character: exactly the silver-haired, blue-eyed stylized anime maid from the supplied image, matching twin tails, headband, black short-sleeve maid dress and white apron, modest black stockings and simple black shoes, consistent face and proportions in all panels.
Panel labels only: "A / IDLE", "B / WEIGHT SHIFT", "C / START", "C / TO A".
A: reproduce the supplied image upper-body pose faithfully: torso gently leaning forward toward viewer, head lightly tilted toward viewer left, both hands held together behind lower back, shoulders relaxed, face looking at viewer. Extend the cropped lower body naturally: feet close but not touching, knees gently relaxed, balanced planted feet. This is a quiet breathing idle, not a T-pose or A-pose rig stance.
B: same hands behind back and gentle upper body lean, change the leg pose visibly yet subtly: weight supported on character's left leg, right foot shifted slightly forward and outward with relaxed bent right knee, natural asymmetrical hips. Keep both shoes in floor contact and same overall position and scale.
C START: full body rear three-quarter view, character facing away about 140 degrees from the A facing direction, clear back and side of head, relaxed posture, hands behind back visible, feet planted in a believable stance ready to turn.
C TO A: show the same character mid-turn at front three-quarter angle about 55 degrees, head gently leading shoulders and hips, one small pivoting step toward the A stance; no walking away.
Consistent locked camera and lighting across panels, clean anatomical readable pose silhouettes, beautiful faithful anime illustration, entirely wholesome, no exaggerated expression or dramatic gesture, no weapon, no arrows covering joints, no motion blur, no extra limbs. Show all shoes and entire hair with comfortable margins.
