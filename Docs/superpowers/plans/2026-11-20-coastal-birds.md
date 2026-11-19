# Coastal wildlife implementation plan

Implement the approved [coastal bird design](../specs/2026-11-20-coastal-birds-design.md) in UE 5.7. Keep independent art, runtime and placement work separate until editor integration.

1. Author a gull body, two wings, two legs, a palm, a shell and driftwood under `SourceArt/Environment/CoastalWildlife`. Verify object origins, dimensions and the shared palette after FBX export.
2. Build `ACoastalGull` in the CoastalWildlife runtime plugin. Use one shared player-distance check, disable each distant bird's tick, and resume local motion time on wake. Exercise walking, takeoff, flight, landing, and the inclusive 10,000 cm boundary.
3. Import the art under `/Game/Environment/CoastalWildlife`, assign the palette material and create `BP_CoastalGull`. Reload the packages to check mesh bounds, wing and leg alignment and BP parent.
4. Survey both faction beaches and the two widened forest water zones. Place dry walking and landing points, water-facing flights, and village shore props. Preserve the pending forest map edit separately. Check dry ground, shoreline clearance, map reload and player approach from offscreen.
5. Verify tests and saved placements, remove temporary generators from the change, append the request record and commit this task.
