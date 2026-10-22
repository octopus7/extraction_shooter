// Custom material expression source, embedded in M_ShorelineReference.
// Shoreline algorithm adapted from norio/three-stylized-water (MIT).
// Copyright (c) 2025 MatrixRex; Copyright (c) 2026 norio.
// Full notice: Resources/Reference/LICENSE.txt. UE coordinates are centimeters, Z up.
// UE FBX import mirrors source Y. Data UV: (-4096,-4096) UE XY maps to (0,0).
// Noise has independent world XY UVs; source EXR V is negative source Y.
float time = lerp(T * AnimationSpeed, PreviewTime, saturate(FreezeTime));
float2 dataUV = P.xy / max(MaskWorldSize, 1.0) + 0.5;
float4 shoreData = Texture2DSample(WaterData, WaterDataSampler, dataUV);
float depthCm = max(shoreData.g * MaskDepthRange, 0.0);
float distanceCm = (shoreData.r - 0.5) * 2000.0;
float coverage = smoothstep(-max(fwidth(distanceCm), 0.5), max(fwidth(distanceCm), 0.5), distanceCm);
float shallow = saturate(exp(-depthCm / max(ShoreDepthCm, 0.01)));
float center = smoothstep(0.5, 0.757, shallow);
float phase = (shallow - time * PhaseSpeed) * max(BandAmount, 0.01);
float pos = frac(phase);
float neg = frac(-phase);
float edge = -0.5 * BandThickness;
float bandA = smoothstep(edge, edge + 1.0, neg);
float bandB = smoothstep(edge, edge + 1.0, pos);
float positiveSpeed = PhaseSpeed > 0.0 ? 1.0 : 0.0;
float band = lerp(bandB, bandA, positiveSpeed);
float2 noiseUV = P.xy * (DissolveScale * 0.001) + float2(0.02, 0.03) * time;
float noise = Texture2DSample(Dissolve, DissolveSampler, noiseUV).r;
float dissolved = band * (1.0 - noise * DissolveStrength);
float expanded = lerp(dissolved, band, shallow * GradientDissolve);
float aa = clamp(fwidth(expanded) * 0.65, 0.001, 0.035);
float front = smoothstep(0.5 - aa, 0.5 + aa, expanded);
float directionPhase = lerp(neg, pos, positiveSpeed);
float trail = lerp(front, directionPhase * front, TrailFade);
// Reuse the dissolve texture with independent rotation/scale to erode the tail.
// Protect the bright leading portion and the scalloped contour; no new foam is added.
float2 tailUV = float2(0.8 * noiseUV.x - 0.6 * noiseUV.y,
                      0.6 * noiseUV.x + 0.8 * noiseUV.y)
              * TailBreakupScale + float2(0.371, 0.619);
float tailNoise = Texture2DSample(Dissolve, DissolveSampler, tailUV).r;
float holes = smoothstep(0.075, 0.18, tailNoise);
float tailOnly = (1.0 - smoothstep(0.45, 0.80, directionPhase))
               * smoothstep(0.52, 0.68, expanded);
float foamMask = saturate(center + trail - 1.0);
foamMask = max(0.0, foamMask - 0.45 * saturate(TailBreakupStrength) * tailOnly * holes);
float foam = foamMask * coverage * saturate(FoamStrength);

// Color and opacity are intentionally independent, matching the reference layers.
float deepFade = smoothstep(OpaqueDepthStartCm, max(OpaqueDepthEndCm, OpaqueDepthStartCm + 1.0), depthCm);
float colorShallow = exp(-depthCm / max(ColorDepthCm, 0.01)) * (1.0 - deepFade);
float3 waterColor = lerp(DeepColor, ShallowColor, colorShallow);
float baseAlpha = lerp(DeepOpacity, ShallowOpacity, colorShallow);
// The isolated island has a finite seabed mesh. Hide its distant rectangular edge.
baseAlpha = lerp(baseAlpha, 1.0, deepFade);
float foamBlend = foam * saturate(FoamOpacity);
float3 color = lerp(waterColor, FoamColor, foamBlend);
float alpha = baseAlpha + (1.0 - baseAlpha) * foamBlend;
float shoreFade = smoothstep(0.0, max(ShoreFadeWidth, 0.001), 1.0 - exp(-depthCm / 30.0));
alpha *= shoreFade * coverage;

// This study leaves refraction, reflection, geometry waves and all-over foam off.
// DebugMask is a render-test output; normal review materials leave it at zero.
if (DebugMask > 0.5)
{
    return float4(foam.xxx, 1.0);
}
return float4(color, saturate(alpha));
