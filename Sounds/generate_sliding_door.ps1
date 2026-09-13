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
        double filtered = 0, smooth = 0, phase = 0;
        using (var writer = new BinaryWriter(File.Create(path))) {
            writer.Write(System.Text.Encoding.ASCII.GetBytes("RIFF")); writer.Write(36 + count*2);
            writer.Write(System.Text.Encoding.ASCII.GetBytes("WAVEfmt ")); writer.Write(16);
            writer.Write((short)1); writer.Write((short)1); writer.Write(rate); writer.Write(rate*2);
            writer.Write((short)2); writer.Write((short)16);
            writer.Write(System.Text.Encoding.ASCII.GetBytes("data")); writer.Write(count*2);
            for (int i=0; i<count; i++) {
                double t = (double)i / rate;
                double p = t / duration;
                double speed = Math.Pow(Math.Sin(Math.PI * p), .55);
                double envelope = Math.Pow(Math.Sin(Math.PI * p), 1.15);
                double noise = random.NextDouble()*2-1;
                filtered += .09 * (noise-filtered);
                smooth += .012 * (filtered-smooth);
                double frequency = (closing ? 125 : 145) + 38*speed;
                phase += 2*Math.PI*frequency/rate;
                double motor = .032*Math.Sin(phase) + .013*Math.Sin(2*phase) + .004*Math.Sin(4*phase);
                double rail = .075*(filtered-smooth);
                double v = (motor+rail)*envelope;
                // Soft rubber seating, with no metallic impact or latch clank.
                double seat = (p-.87)/.045;
                v += (closing ? .012 : .006)*Math.Exp(-seat*seat)*Math.Sin(2*Math.PI*78*t);
                double edge = Math.Min(1, (count-1-i)/(rate*.012));
                writer.Write((short)Math.Round(v*edge*32767));
            }
        }
    }
}
'@
[QuietSlidingDoorSound]::Render((Join-Path $PSScriptRoot 'SlidingDoor_Open.wav'), 0.6, $false)
[QuietSlidingDoorSound]::Render((Join-Path $PSScriptRoot 'SlidingDoor_Close.wav'), 0.75, $true)
