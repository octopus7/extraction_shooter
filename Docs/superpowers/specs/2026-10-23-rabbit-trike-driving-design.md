# Rabbit trike driving

The user approved implementing the proposed drive, stop, aim, fire behavior. The current AI faces its target during repositioning while character movement travels sideways; the wheels only respond to longitudinal travel.

Use the existing navmesh and combat state machine. A character movement component constrains the trike's grounded travel to its forward axis, turns toward the requested path direction at the combat profile's angular limit, and slows for sharp turns. A stopped trike may pivot to acquire its next heading so short paths and narrow spaces remain usable. This is game steering rather than a vehicle physics simulation.

During travel, target-facing must not compete with steering. Firing requires the trike to be stationary, have no active move request or movement input, and face the target within the existing attack tolerance. Once stopped, the existing aim/fire sequence resumes. Front-wheel steering follows actual yaw and forward travel; all wheels continue to roll from actual displacement.

## Approaches

- Selected: steer the existing character movement, then stop and aim. Fits the fixed body-mounted gun and preserves navigation/combat integration.
- Separate rotating gun/body: supports continuous drive-by fire but requires model changes beyond this request.
- Full vehicle physics: substantially more navigation and collision work than this enemy needs.

## Constraints

- UE 5.7; preserve bounded profile yaw speed and attack facing tolerance.
- Enable only for bTricycle; delivery robots and other enemies retain their behavior.
- No new player-facing text, assets, persistent state, or spawn data.
- Preserve unrelated pending shoreline edits and logs. One task commit after validation and task logging.
- Validate native and saved Blueprint construction, navigation velocity and acceleration input, large heading changes, stopping/firing, wheel steering, and existing enemy combat tests.
