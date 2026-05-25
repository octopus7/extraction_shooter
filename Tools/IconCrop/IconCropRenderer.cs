using System.Globalization;
using System.IO;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Imaging;

namespace IconCropTool;

public static class IconCropRenderer
{
    public static readonly JsonSerializerOptions JsonOptions = new()
    {
        AllowTrailingCommas = true,
        ReadCommentHandling = JsonCommentHandling.Skip,
        WriteIndented = true,
        DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull
    };

    public static BitmapImage LoadBitmap(string path)
    {
        var image = new BitmapImage();
        image.BeginInit();
        image.CacheOption = BitmapCacheOption.OnLoad;
        image.CreateOptions = BitmapCreateOptions.PreservePixelFormat;
        image.UriSource = new Uri(path, UriKind.Absolute);
        image.EndInit();
        image.Freeze();
        return image;
    }

    public static BitmapSource RenderPreview(CropDocument document, BitmapSource sourceSheet, IconEntry icon)
    {
        CanvasSize canvas = GetOutputCanvas(document, icon);
        CropRect placed = GetOutputPlacedRect(document, icon, canvas);

        var target = new RenderTargetBitmap(canvas.Width, canvas.Height, 96, 96, PixelFormats.Pbgra32);
        var visual = new DrawingVisual();
        using (DrawingContext context = visual.RenderOpen())
        {
            Int32Rect cropRect = ToSafeInt32Rect(icon.SourceCropRect, sourceSheet.PixelWidth, sourceSheet.PixelHeight);
            if (cropRect.Width > 0 && cropRect.Height > 0 && placed.Width > 0 && placed.Height > 0)
            {
                var cropped = new CroppedBitmap(sourceSheet, cropRect);
                context.DrawImage(cropped, new Rect(placed.X, placed.Y, placed.Width, placed.Height));
            }
        }

        target.Render(visual);
        target.Freeze();
        return target;
    }

    public static int CropAll(CropDocument document, string documentDirectory, string outputDirectory)
    {
        Directory.CreateDirectory(outputDirectory);

        var sheetCache = new Dictionary<string, BitmapImage>(StringComparer.OrdinalIgnoreCase);
        int saved = 0;

        foreach (IconEntry icon in document.Icons)
        {
            string sheetFilename = string.IsNullOrWhiteSpace(icon.SourceSheetFilename)
                ? document.SheetImageFilename
                : icon.SourceSheetFilename!;
            string sheetPath = Path.Combine(documentDirectory, sheetFilename);
            if (!sheetCache.TryGetValue(sheetPath, out BitmapImage? sheet))
            {
                sheet = LoadBitmap(sheetPath);
                sheetCache[sheetPath] = sheet;
            }

            UpdateIconScale(document, icon);
            BitmapSource rendered = RenderPreview(document, sheet, icon);
            string outputPath = Path.Combine(outputDirectory, GetOutputFilename(icon));
            SavePng(rendered, outputPath);
            saved++;
        }

        return saved;
    }

    public static void NormalizeDocument(CropDocument document)
    {
        foreach (IconEntry icon in document.Icons)
        {
            if (string.IsNullOrWhiteSpace(icon.SourceSheetFilename))
            {
                icon.SourceSheetFilename = document.SheetImageFilename;
            }

            UpdateIconScale(document, icon);
        }
    }

    public static CanvasSize GetOutputCanvas(CropDocument document, IconEntry icon)
    {
        if (icon.OutputCanvas is { Width: > 0, Height: > 0 } iconCanvas)
        {
            return iconCanvas;
        }

        if (document.OutputIconCanvas is { Width: > 0, Height: > 0 } documentCanvas)
        {
            return new CanvasSize
            {
                Width = documentCanvas.Width,
                Height = documentCanvas.Height
            };
        }

        return new CanvasSize
        {
            Width = Math.Max(1, (int)Math.Round(icon.SourceCropRect.Width)),
            Height = Math.Max(1, (int)Math.Round(icon.SourceCropRect.Height))
        };
    }

    public static CropRect GetOutputPlacedRect(CropDocument document, IconEntry icon, CanvasSize canvas)
    {
        if (icon.OutputPlacedRect is { Width: > 0, Height: > 0 } placed)
        {
            return placed;
        }

        int margin = document.OutputIconCanvas?.Margin ?? 0;
        return new CropRect
        {
            X = margin,
            Y = margin,
            Width = Math.Max(1, canvas.Width - (margin * 2)),
            Height = Math.Max(1, canvas.Height - (margin * 2))
        };
    }

    public static string FormatNumber(double value)
    {
        return value.ToString("0.##", CultureInfo.InvariantCulture);
    }

    private static void UpdateIconScale(CropDocument document, IconEntry icon)
    {
        CanvasSize canvas = GetOutputCanvas(document, icon);
        CropRect placed = GetOutputPlacedRect(document, icon, canvas);
        if (icon.SourceCropRect.Width <= 0 || icon.SourceCropRect.Height <= 0)
        {
            icon.Scale = 0;
            return;
        }

        double scaleX = placed.Width / icon.SourceCropRect.Width;
        double scaleY = placed.Height / icon.SourceCropRect.Height;
        icon.Scale = Math.Min(scaleX, scaleY);
    }

    private static Int32Rect ToSafeInt32Rect(CropRect rect, int maxWidth, int maxHeight)
    {
        int x = (int)Math.Round(rect.X);
        int y = (int)Math.Round(rect.Y);
        int width = (int)Math.Round(rect.Width);
        int height = (int)Math.Round(rect.Height);

        if (width <= 0 || height <= 0 || maxWidth <= 0 || maxHeight <= 0)
        {
            return new Int32Rect(0, 0, 0, 0);
        }

        x = Math.Clamp(x, 0, maxWidth - 1);
        y = Math.Clamp(y, 0, maxHeight - 1);
        width = Math.Clamp(width, 1, maxWidth - x);
        height = Math.Clamp(height, 1, maxHeight - y);
        return new Int32Rect(x, y, width, height);
    }

    private static void SavePng(BitmapSource source, string path)
    {
        using var stream = File.Create(path);
        var encoder = new PngBitmapEncoder();
        encoder.Frames.Add(BitmapFrame.Create(source));
        encoder.Save(stream);
    }

    private static string GetOutputFilename(IconEntry icon)
    {
        if (!string.IsNullOrWhiteSpace(icon.IconFilename))
        {
            return icon.IconFilename;
        }

        return $"icon_{icon.Index:000}.png";
    }
}
