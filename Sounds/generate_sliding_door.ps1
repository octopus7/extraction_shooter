$ErrorActionPreference = 'Stop'
# Standalone WAV synthesis; no Unreal Editor dependencies or asset generation.
Add-Type -TypeDefinition @'
using System;
using System.IO;
public static class QuietSlidingDoorSound {
    public static void Render(string path, double duration, bool closing) {
        const int rate = 48000;
        int count = (int)(duration * rate);
        var random = new Random(closing ? 602 : 601);
        double filtered = 0, smooth = 0, phase = 0, airPhase = 0, peak = 0;
        var samples = new double[count];
        for (int i=0; i<count; i++) {
            double t = (double)i / rate;
            double p = t / duration;
            double rise = 1-Math.Exp(-t/.055);
            double fall = Math.Min(1, (duration-t)/.16);
            fall = fall*fall*(3-2*fall);
            double envelope = rise*fall;
            double speed = Math.Sin(Math.PI*Math.Pow(p, .70));
            // Smooth independent servo glides, without the low harmonic buzz of the first version.
            double frequency = (closing ? 380 : 440) + (closing ? 170 : 230)*speed;
            phase += 2*Math.PI*frequency/rate;
            airPhase += 2*Math.PI*((closing ? 1050 : 1200)-260*p)/rate;
            double noise = random.NextDouble()*2-1;
            filtered += .18*(noise-filtered);
            smooth += .035*(filtered-smooth);
            double servo = .27*Math.Sin(phase) + .045*Math.Sin(airPhase);
            double air = .62*(filtered-smooth);
            double weight = .05*Math.Sin(2*Math.PI*72*t)*Math.Sin(Math.PI*p);
            double v = (servo+air+weight)*envelope;
            // Subtle cushioned arrival; no impact, flutter, tremolo or metallic rattle.
            double seat = (p-.88)/.06;
            v += (closing ? .025 : .012)*Math.Exp(-seat*seat)*Math.Sin(2*Math.PI*95*t);
            double edge = Math.Min(1, (count-1-i)/(rate*.012));
            samples[i] = v*edge;
            peak = Math.Max(peak, Math.Abs(samples[i]));
        }
        using (var writer = new BinaryWriter(File.Create(path))) {
            writer.Write(System.Text.Encoding.ASCII.GetBytes("RIFF")); writer.Write(36 + count*2);
            writer.Write(System.Text.Encoding.ASCII.GetBytes("WAVEfmt ")); writer.Write(16);
            writer.Write((short)1); writer.Write((short)1); writer.Write(rate); writer.Write(rate*2);
            writer.Write((short)2); writer.Write((short)16);
            writer.Write(System.Text.Encoding.ASCII.GetBytes("data")); writer.Write(count*2);
            // -9 dBFS source peak leaves headroom while remaining audible at door volume 1.
            foreach (double sample in samples) {
                writer.Write((short)Math.Round(sample/peak*Math.Pow(10, -9.0/20)*32767));
            }
        }
    }
}
'@
[QuietSlidingDoorSound]::Render((Join-Path $PSScriptRoot 'SlidingDoor_Open.wav'), 0.6, $false)
[QuietSlidingDoorSound]::Render((Join-Path $PSScriptRoot 'SlidingDoor_Close.wav'), 0.75, $true)
