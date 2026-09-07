$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Drawing
$taskRoot=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$taskDir=Join-Path $taskRoot 'TunaSweeper/SourceArt/Environment/ModularInteriorExpansion/Textures'
$taskImage=[System.Drawing.Image]::FromFile((Join-Path $taskDir 'ImageGen_Expansion_Original.png'))
$taskBitmap=New-Object System.Drawing.Bitmap 2048,2048
$taskGraphics=[System.Drawing.Graphics]::FromImage($taskBitmap)
$taskGraphics.DrawImage($taskImage,0,0,2048,2048)
$taskGraphics.TextRenderingHint=[System.Drawing.Text.TextRenderingHint]::AntiAliasGridFit
$taskFont=New-Object System.Drawing.Font 'Arial',160,([System.Drawing.FontStyle]::Bold),([System.Drawing.GraphicsUnit]::Pixel)
$taskBrush=New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb(230,235,221))
$taskFormat=New-Object System.Drawing.StringFormat
$taskFormat.Alignment=[System.Drawing.StringAlignment]::Center
$taskFormat.LineAlignment=[System.Drawing.StringAlignment]::Center
$taskGraphics.DrawString('ZONE 01',$taskFont,$taskBrush,([System.Drawing.RectangleF]::new(90,1260,844,440)),$taskFormat)
$taskBitmap.Save((Join-Path $taskDir 'Expansion_Typography.png'),[System.Drawing.Imaging.ImageFormat]::Png)
$taskGraphics.Dispose();$taskImage.Dispose();$taskBitmap.Dispose();$taskFont.Dispose();$taskBrush.Dispose();$taskFormat.Dispose()
