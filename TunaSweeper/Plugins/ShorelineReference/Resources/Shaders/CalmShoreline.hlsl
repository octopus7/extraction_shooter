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
// The depth texture covers a finite seabed. Reach opaque deep water before
// its edge so clamped samples cannot reveal a rectangular sand boundary.
float dataEdge = max(abs(dataUV.x - 0.5), abs(dataUV.y - 0.5)) * 2.0;
float dataInterior = 1.0 - smoothstep(0.82, 0.98, dataEdge);
waterColor = lerp(DeepColor, waterColor, dataInterior);
baseAlpha = lerp(1.0, baseAlpha, dataInterior);

// Broad, intersecting ripples in world space. Blended slopes stay attached to
// the water and need no extra normal texture or screen-space history.
float2 surfaceUV = P.xy * (0.018 * max(SurfaceScale, 0.01));
float surfaceTime = time * SurfaceSpeed;
float2 macroUV = P.xy * 0.0008 + float2(0.003, -0.002) * surfaceTime;
float macroA = Texture2DSample(Dissolve, DissolveSampler, macroUV).r;
float macroB = Texture2DSample(Dissolve, DissolveSampler, macroUV.yx * 0.73 + float2(0.31, 0.67)).r;
float waveA = dot(surfaceUV, float2(0.94, 0.342)) + surfaceTime * 0.83 + macroA * 7.0;
float waveB = dot(surfaceUV, float2(-0.45, 0.893)) * 1.37 - surfaceTime * 0.61 + macroB * 8.0;
float waveC = dot(surfaceUV, float2(0.30, 0.954)) * 2.17 + surfaceTime * 0.47 + (macroA + macroB) * 5.0;
float2 slope = float2(0.94, 0.342) * cos(waveA) * 0.56
             + float2(-0.45, 0.893) * cos(waveB) * 0.31
             + float2(0.30, 0.954) * cos(waveC) * 0.13;
float waveFootprint = max(length(ddx(surfaceUV)), length(ddy(surfaceUV)));
float waveFilter = 1.0 / (1.0 + waveFootprint * waveFootprint * 4.0);
slope *= waveFilter;
float shoreCalm = smoothstep(1.0, 32.0, depthCm);
float3 normal = normalize(float3(-slope * RippleStrength * shoreCalm, 1.0));
float3 view = normalize(ViewDirection);
float facing = saturate(dot(normal, view));
float fresnel = 0.025 + 0.975 * pow(1.0 - facing, 5.0);
float3 reflected = reflect(-view, normal);
float skyHeight = saturate(reflected.z * 0.5 + 0.5);
float3 skyColor = lerp(float3(0.24, 0.39, 0.46), float3(0.055, 0.20, 0.34), skyHeight);
float reflectedCloud = smoothstep(0.25, 0.82,
    sin(reflected.x * 6.0 + reflected.z * 2.0) * 0.34
  + sin(reflected.y * 5.0 - reflected.x * 2.0) * 0.26 + 0.44);
skyColor = lerp(skyColor, float3(0.42, 0.52, 0.56), reflectedCloud * 0.32);
float3 surfaceColor = lerp(waterColor, skyColor,
    saturate(ReflectionStrength * (0.12 + fresnel * 0.72)));
float swellShade = sin(waveA) * 0.55 + sin(waveB) * 0.30 + sin(waveC) * 0.15;
surfaceColor *= 1.0 + swellShade * 0.022 * shoreCalm * waveFilter;

// A soft directional sun lobe. Width is deliberately broad to avoid glitter.
float3 sunDirection = normalize(float3(0.32, 0.50, 0.80));
float3 halfDirection = normalize(view + sunDirection);
float sun = pow(saturate(dot(normal, halfDirection)), 65.0);
surfaceColor += float3(1.0, 0.91, 0.73) * sun * GlintStrength * shoreCalm;

// Unequal slowly moving cells form connected light on the sand, rather than
// repeating crosses. Screen footprint softens the smallest distant lines.
float2 causticUV = P.xy * 0.009;
causticUV += float2(macroA, macroB) * 1.4;
causticUV += float2(sin(causticUV.y * 1.12 + surfaceTime * 0.18)
                         + sin(causticUV.x * 0.71 - surfaceTime * 0.13),
                   sin(causticUV.x * 1.07 - surfaceTime * 0.16)
                         + sin(causticUV.y * 0.67 + surfaceTime * 0.11)) * 0.32;
float2 cell = floor(causticUV);
float2 withinCell = frac(causticUV);
float nearest = 8.0;
float second = 8.0;
[unroll] for (int cy = -1; cy <= 1; ++cy)
{
    [unroll] for (int cx = -1; cx <= 1; ++cx)
    {
        float2 neighbor = float2(cx, cy);
        float2 id = cell + neighbor;
        float2 seed = frac(sin(float2(dot(id, float2(127.1, 311.7)),
                                     dot(id, float2(269.5, 183.3)))) * 43758.5453);
        float2 cellCenter = 0.5 + 0.32 * sin(seed * 6.283185 + surfaceTime * 0.28);
        float2 delta = neighbor + cellCenter - withinCell;
        float squared = dot(delta, delta);
        second = min(second, max(nearest, squared));
        nearest = min(nearest, squared);
    }
}
float cellEdge = sqrt(second) - sqrt(nearest);
float causticAA = max(fwidth(cellEdge), 0.008);
float causticLine = 1.0 - smoothstep(0.012, 0.055 + causticAA, cellEdge);
float causticPatch = Texture2DSample(Dissolve, DissolveSampler,
    P.xy * 0.0016 + float2(-0.004, 0.003) * surfaceTime + 0.47).r;
causticLine *= smoothstep(0.045, 0.23, causticPatch);
float causticDepth = smoothstep(3.0, 18.0, depthCm) * (1.0 - smoothstep(45.0, 150.0, depthCm));
surfaceColor += float3(0.46, 0.72, 0.61) * causticLine * causticDepth * CausticsStrength;
waterColor = lerp(waterColor, surfaceColor, saturate(SurfaceDetailStrength));

float foamBlend = foam * saturate(FoamOpacity);
float3 color = lerp(waterColor, FoamColor, foamBlend);
float alpha = baseAlpha + (1.0 - baseAlpha) * foamBlend;
float shoreFade = smoothstep(0.0, max(ShoreFadeWidth, 0.001), 1.0 - exp(-depthCm / 30.0));
alpha *= shoreFade * coverage;

// Stylized sky/sun shading above is not a scene reflection or refraction pass.
// DebugMask is a render-test output; normal review materials leave it at zero.
if (DebugMask > 0.5)
{
    return float4(foam.xxx, 1.0);
}
return float4(color, saturate(alpha));
