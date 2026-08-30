param(
    [string]$SourcePath = "TunaSweeper/Content/SourceArt/Robot/T_Robot_Source.png",
    [string]$MaskPath = "TunaSweeper/Content/SourceArt/Robot/T_Robot_BodyColorMask.png",
    [string]$PreviewPath = "TunaSweeper/Content/SourceArt/Robot/T_Robot_BodyColorMask_Preview.png"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

Add-Type -AssemblyName System.Drawing
$drawingAssemblies = @(
    [System.Drawing.Bitmap].Assembly.Location
    [System.Drawing.Color].Assembly.Location
    [System.Reflection.Assembly]::Load("System.Private.Windows.GdiPlus").Location
    [System.Reflection.Assembly]::Load("System.Private.Windows.Core").Location
)
Add-Type -ReferencedAssemblies $drawingAssemblies -TypeDefinition @"
using System;
using System.Drawing;
using System.Drawing.Imaging;

public static class RobotBodyColorMaskGenerator
{
    private static float SmoothStep(float edge0, float edge1, float value)
    {
        float t = Math.Max(0.0f, Math.Min(1.0f, (value - edge0) / (edge1 - edge0)));
        return t * t * (3.0f - (2.0f * t));
    }

    public static void Generate(string sourcePath, string maskPath, string previewPath)
    {
        using (Bitmap source = new Bitmap(sourcePath))
        using (Bitmap mask = new Bitmap(source.Width, source.Height, PixelFormat.Format24bppRgb))
        using (Bitmap preview = new Bitmap(source.Width, source.Height, PixelFormat.Format24bppRgb))
        {
            for (int y = 0; y < source.Height; ++y)
            {
                for (int x = 0; x < source.Width; ++x)
                {
                    Color color = source.GetPixel(x, y);
                    float r = color.R / 255.0f;
                    float g = color.G / 255.0f;
                    float b = color.B / 255.0f;
                    float minChannel = Math.Min(r, Math.Min(g, b));
                    float maxChannel = Math.Max(r, Math.Max(g, b));
                    float saturation = maxChannel > 0.0001f ? (maxChannel - minChannel) / maxChannel : 0.0f;

                    // Select bright, nearly neutral paint while rejecting the black machinery and cyan LEDs.
                    float brightnessWeight = SmoothStep(0.32f, 0.68f, minChannel);
                    float neutralWeight = 1.0f - SmoothStep(0.18f, 0.48f, saturation);
                    float selection = SmoothStep(0.04f, 0.92f, brightnessWeight * neutralWeight);
                    int maskValue = (int)Math.Round(selection * 255.0f);

                    mask.SetPixel(x, y, Color.FromArgb(maskValue, maskValue, maskValue));

                    float overlay = selection * 0.68f;
                    int previewR = (int)Math.Round(color.R * (1.0f - overlay) + 255.0f * overlay);
                    int previewG = (int)Math.Round(color.G * (1.0f - overlay) + 24.0f * overlay);
                    int previewB = (int)Math.Round(color.B * (1.0f - overlay) + 176.0f * overlay);
                    preview.SetPixel(x, y, Color.FromArgb(previewR, previewG, previewB));
                }
            }

            mask.Save(maskPath, ImageFormat.Png);
            preview.Save(previewPath, ImageFormat.Png);
        }
    }
}
"@

$sourceFullPath = [System.IO.Path]::GetFullPath($SourcePath)
$maskFullPath = [System.IO.Path]::GetFullPath($MaskPath)
$previewFullPath = [System.IO.Path]::GetFullPath($PreviewPath)
[System.IO.Directory]::CreateDirectory([System.IO.Path]::GetDirectoryName($maskFullPath)) | Out-Null
[System.IO.Directory]::CreateDirectory([System.IO.Path]::GetDirectoryName($previewFullPath)) | Out-Null

[RobotBodyColorMaskGenerator]::Generate($sourceFullPath, $maskFullPath, $previewFullPath)
Write-Output "ROBOT_COLOR_MASK=$maskFullPath"
Write-Output "ROBOT_COLOR_MASK_PREVIEW=$previewFullPath"
