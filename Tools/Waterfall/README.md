# UV waterfall test

Open `/Game/FX/Waterfall/L_WaterfallTest` in UE 5.7 and play in the selected viewport. The review camera uses fixed exposure. The surrounding cliff is a simple blockout, not finished environment art.

- `SM_Waterfall_UV` / `MI_Waterfall_UV`: a UV plane with the dedicated animated translucent curtain material. In the test level, roll is 90 degrees and scale is `(2.8, 4.4, 1)` (280 cm wide, 440 cm tall). Positive V points down. Two irregular generated flow-mask layers scroll down at approximately 0.49 and 0.45 curtain heights per second instead of evenly spaced procedural lines.
- `NS_Waterfall_Stream_UV`: continuous downward droplets distributed across the curtain's top.
- `NS_Waterfall_Splash_Mist`: continuous ballistic droplets (600/sec, 4.5 x 8 cm) and rising, fading mist (120/sec, 120 x 90 cm) across the base. Mist opacity is 0.26. These veil the impact center; the water and foam surfaces remain visible and have no center cutout.
- `M_Waterfall_Foam`: soft outward waves broken up by the generated foam mask, with uneven coverage instead of crisp continuous rings. Spray and mist have their own radial, lifetime-faded materials.

Generated PNG sources and exact built-in image-generation prompts are in `TunaSweeper/SourceArt/FX/Waterfall/README.md`. Imported `T_Waterfall_Flow` and `T_Waterfall_FoamBreakup` use linear mask compression and wrap addressing. `Shaders/Curtain.hlsl` and `Shaders/Foam.hlsl` retain the source of the saved materials' Custom expressions; they are not runtime includes or startup generators.

Niagara uses CPU, local-space simulation. Emitter widths and velocities are authored in centimeters for this test size; resize/reposition the emitter distributions when changing the waterfall dimensions. No third-party waterfall/footstep assets or generator plugin are needed by the saved effects.

## Verification

With the test level open and PIE stopped, run `Tools/Waterfall/verify_unreal.py` using the editor's Python console (supply the absolute file path). This loads assets, verifies texture references, linear masks, visible foam, vertical placement and UV direction, starts each auto-activated system independently, advances ten seconds, and reads actual particle counts, widths and velocities through transient Niagara simulation caches. It checks intensified spray/mist populations and rejects generator-plugin package dependencies. It does not save or regenerate assets.

Expected marker: `WATERFALL_ASSET_CHECKS_PASSED`. Separately inspect PIE for animated water, outward rings, falling droplets, splash and mist. An active component alone is not proof of visible particles.
