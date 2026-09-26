// Inputs: UV, T, N, FoamTex (linear mask TextureObject).
float2 p = (UV - 0.5) * 2;
float r = length(p);
// Keep texture derivatives stable over long sessions; the wave phase carries
// radial motion, while foam translates without accumulating radial distortion.
float breakup = Texture2DSample(FoamTex, FoamTexSampler, UV * 1.25 + float2(-T * 0.028, T * 0.012)).r;
float drift = Texture2DSample(FoamTex, FoamTexSampler, UV * 0.91 + float2(T * 0.018, -T * 0.011) + 0.31).r;
float textureMask = smoothstep(0.22, 0.78, breakup * 0.7 + drift * 0.3);
float phase = r * 28 - T * 4.1 + sin(atan2(p.y, p.x) * 9) * 0.5 + (drift - 0.5) * 2.2;
float wave = 0.5 + 0.5 * sin(phase);
// Preserve the center: only the outer boundary fades. Impact particles veil it.
float edge = 1 - smoothstep(0.48, 1, r);
float alpha = edge * textureMask * (0.11 + 0.29 * wave);
return float4(lerp(float3(0.19, 0.36, 0.39), float3(0.48, 0.68, 0.70), textureMask), alpha);
