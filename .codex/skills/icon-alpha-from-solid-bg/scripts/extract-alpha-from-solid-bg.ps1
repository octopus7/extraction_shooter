param(
    [Parameter(Mandatory = $true)]
    [string]$BasePath,

    [Parameter(Mandatory = $true)]
    [string]$OutputPath,

    [string]$BlackPath = "",
    [string]$WhitePath = "",
    [string]$PreviewPath = "",
    [string]$KeyColor = "",
    [int]$OutputSize = 0,
    [double]$TransparentDistance = 14.0,
    [double]$OpaqueDistance = 88.0
)

$ErrorActionPreference = "Stop"

function Resolve-FullPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    if ([System.IO.Path]::IsPathRooted($Path)) {
        return [System.IO.Path]::GetFullPath($Path)
    }

    return [System.IO.Path]::GetFullPath((Join-Path (Get-Location).Path $Path))
}

$baseFullPath = Resolve-FullPath $BasePath
$outputFullPath = Resolve-FullPath $OutputPath

if (-not (Test-Path -LiteralPath $baseFullPath)) {
    throw "Base image not found: $baseFullPath"
}

$outputDirectory = [System.IO.Path]::GetDirectoryName($outputFullPath)
if (-not [System.IO.Directory]::Exists($outputDirectory)) {
    [System.IO.Directory]::CreateDirectory($outputDirectory) | Out-Null
}

$stem = [System.IO.Path]::GetFileNameWithoutExtension($outputFullPath)
if ([string]::IsNullOrWhiteSpace($BlackPath)) {
    $BlackPath = Join-Path $outputDirectory ($stem + "_BlackBackground_Source.png")
}
if ([string]::IsNullOrWhiteSpace($WhitePath)) {
    $WhitePath = Join-Path $outputDirectory ($stem + "_WhiteBackground_Source.png")
}
if ([string]::IsNullOrWhiteSpace($PreviewPath)) {
    $PreviewPath = Join-Path $outputDirectory ($stem + "_PreviewChecker.png")
}

$blackFullPath = Resolve-FullPath $BlackPath
$whiteFullPath = Resolve-FullPath $WhitePath
$previewFullPath = Resolve-FullPath $PreviewPath

$code = @'
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;

public static class SolidBackgroundAlphaExtractor
{
    private static byte ClampByte(double value)
    {
        if (value <= 0) return 0;
        if (value >= 255) return 255;
        return (byte)Math.Round(value);
    }

    private static double SmoothStep(double edge0, double edge1, double x)
    {
        if (x <= edge0) return 0.0;
        if (x >= edge1) return 1.0;
        double t = (x - edge0) / (edge1 - edge0);
        return t * t * (3.0 - 2.0 * t);
    }

    private static Bitmap Load32(string path)
    {
        using (var loaded = new Bitmap(path))
        {
            var copy = new Bitmap(loaded.Width, loaded.Height, PixelFormat.Format32bppArgb);
            using (var g = Graphics.FromImage(copy))
            {
                g.Clear(Color.Transparent);
                g.DrawImage(loaded, 0, 0, loaded.Width, loaded.Height);
            }
            return copy;
        }
    }

    private static Color ParseKeyColor(string keyColor)
    {
        if (String.IsNullOrWhiteSpace(keyColor)) return Color.Empty;
        string hex = keyColor.Trim();
        if (hex.StartsWith("#")) hex = hex.Substring(1);
        if (hex.Length != 6) throw new ArgumentException("KeyColor must be #RRGGBB.");
        int r = Convert.ToInt32(hex.Substring(0, 2), 16);
        int g = Convert.ToInt32(hex.Substring(2, 2), 16);
        int b = Convert.ToInt32(hex.Substring(4, 2), 16);
        return Color.FromArgb(255, r, g, b);
    }

    private static Color EstimateBorderKey(Bitmap bitmap)
    {
        const int band = 12;
        var samples = new List<Color>();
        for (int y = 0; y < bitmap.Height; y++)
        {
            for (int x = 0; x < bitmap.Width; x++)
            {
                if (x >= band && y >= band && x < bitmap.Width - band && y < bitmap.Height - band) continue;
                samples.Add(bitmap.GetPixel(x, y));
            }
        }

        samples.Sort((left, right) => left.R.CompareTo(right.R));
        byte keyR = samples[samples.Count / 2].R;
        samples.Sort((left, right) => left.G.CompareTo(right.G));
        byte keyG = samples[samples.Count / 2].G;
        samples.Sort((left, right) => left.B.CompareTo(right.B));
        byte keyB = samples[samples.Count / 2].B;
        return Color.FromArgb(255, keyR, keyG, keyB);
    }

    public static void Build(string basePath, string blackPath, string whitePath, string finalPath, string previewPath, string keyColorText, int targetSize, double transparentDistance, double opaqueDistance)
    {
        using (var baseImage = Load32(basePath))
        {
            int width = baseImage.Width;
            int height = baseImage.Height;
            Color parsedKey = ParseKeyColor(keyColorText);
            Color key = parsedKey.IsEmpty ? EstimateBorderKey(baseImage) : parsedKey;

            using (var blackImage = new Bitmap(width, height, PixelFormat.Format32bppArgb))
            using (var whiteImage = new Bitmap(width, height, PixelFormat.Format32bppArgb))
            using (var alphaImage = new Bitmap(width, height, PixelFormat.Format32bppArgb))
            {
                var rect = new Rectangle(0, 0, width, height);
                var baseData = baseImage.LockBits(rect, ImageLockMode.ReadOnly, PixelFormat.Format32bppArgb);
                var blackData = blackImage.LockBits(rect, ImageLockMode.WriteOnly, PixelFormat.Format32bppArgb);
                var whiteData = whiteImage.LockBits(rect, ImageLockMode.WriteOnly, PixelFormat.Format32bppArgb);

                byte[] src = new byte[Math.Abs(baseData.Stride) * height];
                byte[] black = new byte[Math.Abs(blackData.Stride) * height];
                byte[] white = new byte[Math.Abs(whiteData.Stride) * height];
                Marshal.Copy(baseData.Scan0, src, 0, src.Length);

                for (int y = 0; y < height; y++)
                {
                    for (int x = 0; x < width; x++)
                    {
                        int ps = y * baseData.Stride + x * 4;
                        int poB = y * blackData.Stride + x * 4;
                        int poW = y * whiteData.Stride + x * 4;

                        double srcB = src[ps + 0];
                        double srcG = src[ps + 1];
                        double srcR = src[ps + 2];
                        double dist = Math.Sqrt((srcR - key.R) * (srcR - key.R) + (srcG - key.G) * (srcG - key.G) + (srcB - key.B) * (srcB - key.B));
                        double alpha = SmoothStep(transparentDistance, opaqueDistance, dist);

                        if (alpha < 0.01)
                        {
                            black[poB + 0] = black[poB + 1] = black[poB + 2] = 0;
                            black[poB + 3] = 255;
                            white[poW + 0] = white[poW + 1] = white[poW + 2] = 255;
                            white[poW + 3] = 255;
                            continue;
                        }

                        if (alpha > 0.985) alpha = 1.0;
                        double inv = 1.0 - alpha;
                        double fgR = (srcR - key.R * inv) / alpha;
                        double fgG = (srcG - key.G * inv) / alpha;
                        double fgB = (srcB - key.B * inv) / alpha;

                        black[poB + 0] = ClampByte(fgB * alpha);
                        black[poB + 1] = ClampByte(fgG * alpha);
                        black[poB + 2] = ClampByte(fgR * alpha);
                        black[poB + 3] = 255;
                        white[poW + 0] = ClampByte(fgB * alpha + 255.0 * inv);
                        white[poW + 1] = ClampByte(fgG * alpha + 255.0 * inv);
                        white[poW + 2] = ClampByte(fgR * alpha + 255.0 * inv);
                        white[poW + 3] = 255;
                    }
                }

                Marshal.Copy(black, 0, blackData.Scan0, black.Length);
                Marshal.Copy(white, 0, whiteData.Scan0, white.Length);
                baseImage.UnlockBits(baseData);
                blackImage.UnlockBits(blackData);
                whiteImage.UnlockBits(whiteData);

                blackImage.Save(blackPath, ImageFormat.Png);
                whiteImage.Save(whitePath, ImageFormat.Png);

                var bData = blackImage.LockBits(rect, ImageLockMode.ReadOnly, PixelFormat.Format32bppArgb);
                var wData = whiteImage.LockBits(rect, ImageLockMode.ReadOnly, PixelFormat.Format32bppArgb);
                var aData = alphaImage.LockBits(rect, ImageLockMode.WriteOnly, PixelFormat.Format32bppArgb);
                byte[] bpx = new byte[Math.Abs(bData.Stride) * height];
                byte[] wpx = new byte[Math.Abs(wData.Stride) * height];
                byte[] apx = new byte[Math.Abs(aData.Stride) * height];
                Marshal.Copy(bData.Scan0, bpx, 0, bpx.Length);
                Marshal.Copy(wData.Scan0, wpx, 0, wpx.Length);

                for (int y = 0; y < height; y++)
                {
                    for (int x = 0; x < width; x++)
                    {
                        int pi = y * bData.Stride + x * 4;
                        int po = y * aData.Stride + x * 4;
                        double diffB = Math.Max(0.0, wpx[pi + 0] - bpx[pi + 0]);
                        double diffG = Math.Max(0.0, wpx[pi + 1] - bpx[pi + 1]);
                        double diffR = Math.Max(0.0, wpx[pi + 2] - bpx[pi + 2]);
                        byte alphaByte = ClampByte(255.0 - ((diffB + diffG + diffR) / 3.0));

                        if (alphaByte <= 1)
                        {
                            apx[po + 0] = apx[po + 1] = apx[po + 2] = apx[po + 3] = 0;
                            continue;
                        }

                        double af = alphaByte / 255.0;
                        apx[po + 0] = ClampByte(bpx[pi + 0] / af);
                        apx[po + 1] = ClampByte(bpx[pi + 1] / af);
                        apx[po + 2] = ClampByte(bpx[pi + 2] / af);
                        apx[po + 3] = alphaByte;
                    }
                }

                Marshal.Copy(apx, 0, aData.Scan0, apx.Length);
                blackImage.UnlockBits(bData);
                whiteImage.UnlockBits(wData);
                alphaImage.UnlockBits(aData);

                int finalSize = targetSize > 0 ? targetSize : width;
                int finalHeight = targetSize > 0 ? targetSize : height;
                using (var final = new Bitmap(finalSize, finalHeight, PixelFormat.Format32bppArgb))
                using (var g = Graphics.FromImage(final))
                {
                    g.Clear(Color.Transparent);
                    g.CompositingMode = CompositingMode.SourceCopy;
                    g.CompositingQuality = CompositingQuality.HighQuality;
                    g.InterpolationMode = InterpolationMode.HighQualityBicubic;
                    g.SmoothingMode = SmoothingMode.HighQuality;
                    g.PixelOffsetMode = PixelOffsetMode.HighQuality;
                    g.DrawImage(alphaImage, new Rectangle(0, 0, finalSize, finalHeight), new Rectangle(0, 0, width, height), GraphicsUnit.Pixel);
                    final.Save(finalPath, ImageFormat.Png);
                }

                using (var finalLoaded = new Bitmap(finalPath))
                using (var preview = new Bitmap(finalLoaded.Width, finalLoaded.Height, PixelFormat.Format32bppArgb))
                using (var g = Graphics.FromImage(preview))
                {
                    for (int y = 0; y < finalLoaded.Height; y += 32)
                    {
                        for (int x = 0; x < finalLoaded.Width; x += 32)
                        {
                            bool dark = ((x / 32) + (y / 32)) % 2 == 0;
                            using (var brush = new SolidBrush(dark ? Color.FromArgb(204, 204, 204) : Color.White))
                            {
                                g.FillRectangle(brush, x, y, 32, 32);
                            }
                        }
                    }
                    g.DrawImage(finalLoaded, 0, 0, finalLoaded.Width, finalLoaded.Height);
                    preview.Save(previewPath, ImageFormat.Png);
                }

                Console.WriteLine("Estimated key: #{0:X2}{1:X2}{2:X2}", key.R, key.G, key.B);
            }
        }
    }
}
'@

Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition $code
[SolidBackgroundAlphaExtractor]::Build(
    $baseFullPath,
    $blackFullPath,
    $whiteFullPath,
    $outputFullPath,
    $previewFullPath,
    $KeyColor,
    $OutputSize,
    $TransparentDistance,
    $OpaqueDistance
)

Add-Type -AssemblyName System.Drawing
$image = [System.Drawing.Image]::FromFile($outputFullPath)
try {
    [PSCustomObject]@{
        Output = $outputFullPath
        Black = $blackFullPath
        White = $whiteFullPath
        Preview = $previewFullPath
        Width = $image.Width
        Height = $image.Height
        PixelFormat = $image.PixelFormat.ToString()
    } | Format-List
}
finally {
    $image.Dispose()
}
