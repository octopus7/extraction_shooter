# Hopper three-phase enemy

## Authorized outcome
Integrate the supplied HOPPER_03_ArmModules_v5 character into TunaSweeper UE 5.7: a rabbit boards and stays attached to the mech torso; interchangeable arms support weapon modules; destroyed mech releases the rabbit, which fires finite ammunition and then attacks in melee. Grounded mech walking uses foot-planting IK with heavy acceleration, turning and settling.

## Architecture
ATunaSweeperHopperEnemyCharacter derives from the existing enemy so faction, perception, quest kills, loot and final death remain compatible. Boarding and disembarking suppress standard attacks. Mech durability is separate from rabbit health, with overkill absorbed by the mech. Only final rabbit death grants kill rewards. A module actor owns arm visuals, muzzle and cooldown; left/right modules can be replaced with validated classes. Rabbit is parented to the cockpit during seated combat and detached for disembarkation.

Mech static pieces retain original materials and use world-rest geometry with joint transforms. A ground sampler feeds a pure biped gait solver: one planted foot, alternating swing, grounded foot orientation, reach clamps and body settling. Torso yaw follows the project's constant turn-rate convention. Pilot skeletal animations cover boarding, seat, disembarking, ground movement, shooting and melee.

## Integration and acceptance
A dedicated Hopper test map and placeable Blueprint provide an inspectable result without changing existing user maps or raid placement tables. Expose tuning as editor properties. No new user-facing UI strings unless registered in existing localization. Enemy phases and gait are per-encounter runtime state, matching ordinary enemy persistence. Authoring sources remain editable; remove one-off generation entry points after validating assets. Build UE5.7, run Hopper automation and relevant combat regressions, reload assets, inspect presentation, open the project. Commit all finished code/assets/docs/logs together. Do not push or merge unrelated work.
