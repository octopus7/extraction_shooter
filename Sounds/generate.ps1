$ErrorActionPreference = 'Stop'
# Standalone procedural sound study. Does not access Unreal Engine.
Add-Type -TypeDefinition @'
using System;
using System.IO;
public static class GunSoundStudy {
    const int Rate = 48000;
    static double[] Shot(int kind, int seed) {
        var rng = new Random(seed);
        var a = new double[(int)(Rate * 1.6)];
        double low = 0, mid = 0, phase = 0;
        double[] bass = {115, 95, 155, 65};
        double[] bodyDecay = {.042, .065, .027, .105};
        double[] noiseDecay = {.032, .048, .023, .080};
        for (int i = 0; i < a.Length; i++) {
            double t = (double)i / Rate;
            double n = rng.NextDouble() * 2 - 1;
            low += .027 * (n - low);
            mid += .30 * (n - mid);
            phase += 2 * Math.PI * bass[kind] * (1 + 1.5 * Math.Exp(-t / .009)) / Rate;
            double attack = 1 - Math.Exp(-t / .00012);
            double crack = (n - mid) * 1.35 * Math.Exp(-t / .0035);
            double blast = mid * 2.5 * Math.Exp(-t / noiseDecay[kind]);
            double body = Math.Sin(phase) * .65 * Math.Exp(-t / bodyDecay[kind]);
            double rumble = low * 2.4 * Math.Exp(-t / (bodyDecay[kind] * 2.1));
            double tail = mid * .13 * Math.Exp(-t / .17) * (1 - Math.Exp(-t / .012));
            double mechanical = 0;
            double mt = t - (kind == 3 ? .32 : .055);
            if (mt > 0) mechanical = (.12 * Math.Sin(2*Math.PI*2350*mt) + .20*(n-mid)) * Math.Exp(-mt/.008);
            a[i] = Math.Tanh((crack + blast + body + rumble) * 1.45) * attack + tail + mechanical;
        }
        return a;
    }
    public static void Render(string path, int kind, double seconds, double[] times) {
        var output = new double[(int)(seconds * Rate)];
        for (int s = 0; s < times.Length; s++) {
            var shot = Shot(kind, 1701 + kind * 103 + s * 37);
            int offset = (int)(times[s] * Rate);
            double gain = 1 - s * .016;
            for (int i = 0; i < shot.Length && offset+i < output.Length; i++) {
                output[offset+i] += shot[i] * gain;
                // Quiet, diffuse outdoor reflections instead of a long indoor echo.
                int d1 = offset + i + 1397;
                int d2 = offset + i + 2591;
                if (d1 < output.Length) output[d1] += shot[i] * .075 * gain;
                if (d2 < output.Length) output[d2] += shot[i] * .035 * gain;
            }
        }
        double previousIn = 0, previousOut = 0, peak = 0;
        for (int i = 0; i < output.Length; i++) {
            double v = output[i] - previousIn + .9974 * previousOut;
            previousIn = output[i]; previousOut = v;
            double fade = Math.Min(1, (output.Length - 1 - i) / (Rate * .025));
            output[i] = v * fade;
            peak = Math.Max(peak, Math.Abs(output[i]));
        }
        using (var w = new BinaryWriter(File.Create(path))) {
            int bytes = output.Length * 3;
            w.Write(System.Text.Encoding.ASCII.GetBytes("RIFF")); w.Write(36+bytes);
            w.Write(System.Text.Encoding.ASCII.GetBytes("WAVEfmt ")); w.Write(16);
            w.Write((short)1); w.Write((short)1); w.Write(Rate); w.Write(Rate*3);
            w.Write((short)3); w.Write((short)24);
            w.Write(System.Text.Encoding.ASCII.GetBytes("data")); w.Write(bytes);
            foreach (double v in output) {
                int pcm = (int)Math.Round(v / peak * .707945784 * 8388607);
                w.Write((byte)(pcm & 255)); w.Write((byte)((pcm >> 8) & 255)); w.Write((byte)((pcm >> 16) & 255));
            }
        }
    }
}
'@
[GunSoundStudy]::Render((Join-Path $PSScriptRoot 'Pistol.wav'), 0, 1.1, [double[]]@(0.025))
[GunSoundStudy]::Render((Join-Path $PSScriptRoot 'Rifle.wav'), 1, 1.35, [double[]]@(0.025))
[GunSoundStudy]::Render((Join-Path $PSScriptRoot 'SMG.wav'), 2, 1.8, [double[]]@(0.025, 0.10, 0.175, 0.25, 0.325, 0.40))
[GunSoundStudy]::Render((Join-Path $PSScriptRoot 'Shotgun.wav'), 3, 1.8, [double[]]@(0.025))
