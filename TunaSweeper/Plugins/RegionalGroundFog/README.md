# Regional Ground Fog

`RegionalGroundFog` is a UE 5.7 project plugin for low-lying, locally bounded fog.

- Add `RegionalGroundFogActor` to a level only where fog is wanted.
- Edit its `Fog Nodes` array to make one organic region from multiple softly fading spheres.
- The actor owns its `Local Fog Volume` components and its optional drifting fog cards; it never edits an existing `Exponential Height Fog`, level actor, material, Niagara asset, or renderer setting.
- Select the actor to see cyan inner (full-density) and blue outer (fade-out) node radii in the editor.

The density texture and translucent material are committed under `/RegionalGroundFog/`. Startup generation was retired on 2026-09-06. Edit saved assets directly or restore missing files from Git/LFS; source art remains at `Resources/SourceArt/T_RegionalGroundFogDensity_01.png`.
