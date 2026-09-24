// Inputs: UV, T, N, FlowTex (linear mask TextureObject).
float2 uv0 = float2(UV.x, UV.y * 0.85 - T * 0.42);
float2 uv1 = float2(UV.x * 1.17 + 0.37, UV.y * 0.69 - T * 0.31 + 0.53);
float a = Texture2DSample(FlowTex, FlowTexSampler, uv0).r;
float b = Texture2DSample(FlowTex, FlowTexSampler, uv1).r;
float foam = smoothstep(0.19, 0.83, a * 0.74 + b * 0.26);
float side = smoothstep(0, 0.035, UV.x) * smoothstep(0, 0.035, 1 - UV.x);
float rim = smoothstep(0, 0.025, UV.y) * smoothstep(0, 0.025, 1 - UV.y);
float3 color = lerp(float3(0.025, 0.13, 0.16), float3(0.62, 0.78, 0.81), foam);
return float4(color, side * rim * (0.48 + foam * 0.47));
