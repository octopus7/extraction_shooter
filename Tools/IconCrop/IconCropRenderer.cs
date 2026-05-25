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

    public static AlphaMask CreateAlphaMask(BitmapSource source)
    {
        return AlphaMask.Create(source);
    }

    public static CropRect? FindAlphaTrimRect(
        AlphaMask alphaMask,
        CropRect searchRect,
        bool trimFromFirstEmptyAfterEdgeContent,
        byte alphaThreshold = 0)
    {
        Int32Rect region = ToSafeInt32Rect(searchRect, alphaMask.Width, alphaMask.Height);
        if (region.Width <= 0 || region.Height <= 0)
        {
            return null;
        }

        Int32Rect scanRegion = trimFromFirstEmptyAfterEdgeContent
            ? MovePastEdgeConnectedContent(alphaMask, region, alphaThreshold)
            : region;

        if (!alphaMask.TryFindOpaqueBounds(scanRegion, alphaThreshold, out Int32Rect bounds))
        {
            if (scanRegion != region && alphaMask.TryFindOpaqueBounds(region, alphaThreshold, out bounds))
            {
                return new CropRect
                {
                    X = bounds.X,
                    Y = bounds.Y,
                    Width = bounds.Width,
                    Height = bounds.Height
                };
            }

            return null;
        }

        return new CropRect
        {
            X = bounds.X,
            Y = bounds.Y,
            Width = bounds.Width,
            Height = bounds.Height
        };
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

    private static Int32Rect MovePastEdgeConnectedContent(AlphaMask alphaMask, Int32Rect region, byte alphaThreshold)
    {
        int left = region.X;
        int top = region.Y;
        int right = region.X + region.Width - 1;
        int bottom = region.Y + region.Height - 1;

        left = ResolveLeftEdge(alphaMask, left, right, top, bottom + 1, alphaThreshold);
        right = ResolveRightEdge(alphaMask, left, right, top, bottom + 1, alphaThreshold);

        if (left > right)
        {
            return region;
        }

        top = ResolveTopEdge(alphaMask, top, bottom, left, right + 1, alphaThreshold);
        bottom = ResolveBottomEdge(alphaMask, top, bottom, left, right + 1, alphaThreshold);

        if (top > bottom)
        {
            return region;
        }

        return new Int32Rect(left, top, right - left + 1, bottom - top + 1);
    }

    private static int ResolveLeftEdge(AlphaMask alphaMask, int left, int right, int top, int bottomExclusive, byte alphaThreshold)
    {
        if (!alphaMask.ColumnHasOpaque(left, top, bottomExclusive, alphaThreshold))
        {
            return left;
        }

        for (int x = left + 1; x <= right; x++)
        {
            if (alphaMask.ColumnHasOpaque(x, top, bottomExclusive, alphaThreshold))
            {
                continue;
            }

            for (int nextX = x + 1; nextX <= right; nextX++)
            {
                if (alphaMask.ColumnHasOpaque(nextX, top, bottomExclusive, alphaThreshold))
                {
                    return nextX;
                }
            }

            return ExpandLeftToOuterGap(alphaMask, left, top, bottomExclusive, alphaThreshold);
        }

        return ExpandLeftToOuterGap(alphaMask, left, top, bottomExclusive, alphaThreshold);
    }

    private static int ResolveRightEdge(AlphaMask alphaMask, int left, int right, int top, int bottomExclusive, byte alphaThreshold)
    {
        if (!alphaMask.ColumnHasOpaque(right, top, bottomExclusive, alphaThreshold))
        {
            return right;
        }

        for (int x = right - 1; x >= left; x--)
        {
            if (alphaMask.ColumnHasOpaque(x, top, bottomExclusive, alphaThreshold))
            {
                continue;
            }

            for (int nextX = x - 1; nextX >= left; nextX--)
            {
                if (alphaMask.ColumnHasOpaque(nextX, top, bottomExclusive, alphaThreshold))
                {
                    return nextX;
                }
            }

            return ExpandRightToOuterGap(alphaMask, right, top, bottomExclusive, alphaThreshold);
        }

        return ExpandRightToOuterGap(alphaMask, right, top, bottomExclusive, alphaThreshold);
    }

    private static int ResolveTopEdge(AlphaMask alphaMask, int top, int bottom, int left, int rightExclusive, byte alphaThreshold)
    {
        if (!alphaMask.RowHasOpaque(top, left, rightExclusive, alphaThreshold))
        {
            return top;
        }

        for (int y = top + 1; y <= bottom; y++)
        {
            if (alphaMask.RowHasOpaque(y, left, rightExclusive, alphaThreshold))
            {
                continue;
            }

            for (int nextY = y + 1; nextY <= bottom; nextY++)
            {
                if (alphaMask.RowHasOpaque(nextY, left, rightExclusive, alphaThreshold))
                {
                    return nextY;
                }
            }

            return ExpandTopToOuterGap(alphaMask, top, left, rightExclusive, alphaThreshold);
        }

        return ExpandTopToOuterGap(alphaMask, top, left, rightExclusive, alphaThreshold);
    }

    private static int ResolveBottomEdge(AlphaMask alphaMask, int top, int bottom, int left, int rightExclusive, byte alphaThreshold)
    {
        if (!alphaMask.RowHasOpaque(bottom, left, rightExclusive, alphaThreshold))
        {
            return bottom;
        }

        for (int y = bottom - 1; y >= top; y--)
        {
            if (alphaMask.RowHasOpaque(y, left, rightExclusive, alphaThreshold))
            {
                continue;
            }

            for (int nextY = y - 1; nextY >= top; nextY--)
            {
                if (alphaMask.RowHasOpaque(nextY, left, rightExclusive, alphaThreshold))
                {
                    return nextY;
                }
            }

            return ExpandBottomToOuterGap(alphaMask, bottom, left, rightExclusive, alphaThreshold);
        }

        return ExpandBottomToOuterGap(alphaMask, bottom, left, rightExclusive, alphaThreshold);
    }

    private static int ExpandLeftToOuterGap(AlphaMask alphaMask, int left, int top, int bottomExclusive, byte alphaThreshold)
    {
        int x = left - 1;
        while (x >= 0 && alphaMask.ColumnHasOpaque(x, top, bottomExclusive, alphaThreshold))
        {
            x--;
        }

        return x + 1;
    }

    private static int ExpandRightToOuterGap(AlphaMask alphaMask, int right, int top, int bottomExclusive, byte alphaThreshold)
    {
        int x = right + 1;
        while (x < alphaMask.Width && alphaMask.ColumnHasOpaque(x, top, bottomExclusive, alphaThreshold))
        {
            x++;
        }

        return x - 1;
    }

    private static int ExpandTopToOuterGap(AlphaMask alphaMask, int top, int left, int rightExclusive, byte alphaThreshold)
    {
        int y = top - 1;
        while (y >= 0 && alphaMask.RowHasOpaque(y, left, rightExclusive, alphaThreshold))
        {
            y--;
        }

        return y + 1;
    }

    private static int ExpandBottomToOuterGap(AlphaMask alphaMask, int bottom, int left, int rightExclusive, byte alphaThreshold)
    {
        int y = bottom + 1;
        while (y < alphaMask.Height && alphaMask.RowHasOpaque(y, left, rightExclusive, alphaThreshold))
        {
            y++;
        }

        return y - 1;
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

    public sealed class AlphaMask
    {
        private const int BytesPerPixel = 4;

        private readonly byte[] _pixels;
        private readonly int _stride;

        private AlphaMask(byte[] pixels, int width, int height, int stride)
        {
            _pixels = pixels;
            Width = width;
            Height = height;
            _stride = stride;
        }

        public int Width { get; }

        public int Height { get; }

        public static AlphaMask Create(BitmapSource source)
        {
            BitmapSource readable = source;
            if (source.Format != PixelFormats.Bgra32 && source.Format != PixelFormats.Pbgra32)
            {
                readable = new FormatConvertedBitmap(source, PixelFormats.Pbgra32, null, 0);
                readable.Freeze();
            }

            int stride = readable.PixelWidth * BytesPerPixel;
            var pixels = new byte[stride * readable.PixelHeight];
            readable.CopyPixels(pixels, stride, 0);
            return new AlphaMask(pixels, readable.PixelWidth, readable.PixelHeight, stride);
        }

        public bool TryFindOpaqueBounds(Int32Rect region, byte alphaThreshold, out Int32Rect bounds)
        {
            int left = region.X + region.Width;
            int top = region.Y + region.Height;
            int right = region.X - 1;
            int bottom = region.Y - 1;

            for (int y = region.Y; y < region.Y + region.Height; y++)
            {
                int rowOffset = y * _stride;
                for (int x = region.X; x < region.X + region.Width; x++)
                {
                    if (_pixels[rowOffset + (x * BytesPerPixel) + 3] <= alphaThreshold)
                    {
                        continue;
                    }

                    left = Math.Min(left, x);
                    top = Math.Min(top, y);
                    right = Math.Max(right, x);
                    bottom = Math.Max(bottom, y);
                }
            }

            if (right < left || bottom < top)
            {
                bounds = new Int32Rect();
                return false;
            }

            bounds = new Int32Rect(left, top, right - left + 1, bottom - top + 1);
            return true;
        }

        public bool ColumnHasOpaque(int x, int topInclusive, int bottomExclusive, byte alphaThreshold)
        {
            if (x < 0 || x >= Width)
            {
                return false;
            }

            int top = Math.Clamp(topInclusive, 0, Height);
            int bottom = Math.Clamp(bottomExclusive, top, Height);
            for (int y = top; y < bottom; y++)
            {
                if (_pixels[(y * _stride) + (x * BytesPerPixel) + 3] > alphaThreshold)
                {
                    return true;
                }
            }

            return false;
        }

        public bool RowHasOpaque(int y, int leftInclusive, int rightExclusive, byte alphaThreshold)
        {
            if (y < 0 || y >= Height)
            {
                return false;
            }

            int left = Math.Clamp(leftInclusive, 0, Width);
            int right = Math.Clamp(rightExclusive, left, Width);
            int rowOffset = y * _stride;
            for (int x = left; x < right; x++)
            {
                if (_pixels[rowOffset + (x * BytesPerPixel) + 3] > alphaThreshold)
                {
                    return true;
                }
            }

            return false;
        }
    }
}
