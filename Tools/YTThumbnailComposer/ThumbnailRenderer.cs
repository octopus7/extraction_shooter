using System.IO;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Imaging;

namespace YTThumbnailComposer;

public sealed record BackgroundOption(
    string Key,
    string Label,
    string SourceFileName,
    string OutputFileName,
    TitlePlacement Title);

public sealed record TitlePlacement(double X, double Y, double Height);

public sealed record DigitAtlasMetadata(
    string SourceImage,
    int SourceWidth,
    int SourceHeight,
    string GeneratedAt,
    IReadOnlyList<DigitBounds> Digits);

public sealed record DigitBounds(
    string Digit,
    int X,
    int Y,
    int Width,
    int Height,
    int Advance);

public static class ThumbnailRenderer
{
    public const int CanvasWidth = 1280;
    public const int CanvasHeight = 720;

    private const string DevlogMaskFileName = "mask_devlog_hash_001.png";
    private const string DigitMaskFileName = "mask_digits_0_9_001.png";
    private const string TitleBackingMaskFileName = "mask_title_backing_001.png";
    private const string DigitMetadataFileName = "mask_digits_0_9_001.bounds.json";

    public static readonly IReadOnlyList<BackgroundOption> Backgrounds =
    [
        new("a", "BG_A forest bridge", "BG_A.png", "thumb_dev_a_001.png", new TitlePlacement(52, 496, 126)),
        new("b", "BG_B capsule close-up", "BG_B.jpg", "thumb_dev_b_001.png", new TitlePlacement(632, 92, 118)),
        new("c", "BG_C capsule wide", "BG_C.png", "thumb_dev_c_001.png", new TitlePlacement(54, 496, 124)),
    ];

    public static string[] RenderAllToFiles(string episodeNumber = "1")
    {
        return Backgrounds
            .Select(option => RenderToFile(option, episodeNumber))
            .ToArray();
    }

    public static string RenderToFile(BackgroundOption option, string episodeNumber = "1")
    {
        string ytDirectory = GetYtDirectory();
        BitmapSource bitmap = Render(option, episodeNumber);
        string outputPath = Path.Combine(ytDirectory, option.OutputFileName);

        using FileStream stream = File.Create(outputPath);
        var encoder = new PngBitmapEncoder();
        encoder.Frames.Add(BitmapFrame.Create(bitmap));
        encoder.Save(stream);

        return outputPath;
    }

    public static BitmapSource Render(BackgroundOption option, string episodeNumber = "1")
    {
        string ytDirectory = GetYtDirectory();
        string backgroundPath = Path.Combine(ytDirectory, option.SourceFileName);
        string devlogMaskPath = Path.Combine(ytDirectory, DevlogMaskFileName);
        string digitMaskPath = Path.Combine(ytDirectory, DigitMaskFileName);
        string titleBackingMaskPath = Path.Combine(ytDirectory, TitleBackingMaskFileName);

        BitmapSource background = LoadBitmap(backgroundPath);
        BitmapSource devlogMask = LoadBitmap(devlogMaskPath);
        BitmapSource digitMask = LoadBitmap(digitMaskPath);
        BitmapSource titleBackingMask = LoadBitmap(titleBackingMaskPath);
        Int32Rect devlogBounds = FindMaskBounds(devlogMask, 40, 12);
        Int32Rect titleBackingBounds = FindMaskBounds(titleBackingMask, 36, 10);
        DigitAtlasMetadata digitAtlas = EnsureDigitMetadata(digitMask, ytDirectory);

        double titleHeight = option.Title.Height;
        double devlogWidth = ScaleWidth(devlogBounds, titleHeight);
        double digitSpacing = titleHeight * 0.04;
        IReadOnlyList<PlacedDigit> digits = LayoutDigits(episodeNumber, digitAtlas, option.Title.X + devlogWidth + digitSpacing, option.Title.Y, titleHeight * 0.94);
        double totalDigitWidth = digits.Count == 0 ? 0.0 : digits[^1].Right - digits[0].X;
        double titleWidth = devlogWidth + digitSpacing + totalDigitWidth;

        var visual = new DrawingVisual();
        using (DrawingContext dc = visual.RenderOpen())
        {
            DrawBackground(dc, background);

            var coolEdge = Color.FromRgb(86, 221, 255);
            var softBlue = Color.FromRgb(162, 238, 255);
            var titleWhite = Color.FromRgb(255, 255, 250);
            double backingX = option.Title.X - 56;
            double backingY = option.Title.Y - 64;
            double backingWidth = titleWidth + 128;
            double backingHeight = titleHeight + 132;

            DrawMask(dc, titleBackingMask, titleBackingBounds, backingX + 8, backingY + 10, backingWidth, backingHeight, Colors.Black, 0.42);
            DrawMask(dc, titleBackingMask, titleBackingBounds, backingX + 2, backingY - 1, backingWidth, backingHeight, coolEdge, 0.34);
            DrawMask(dc, titleBackingMask, titleBackingBounds, backingX, backingY, backingWidth, backingHeight, Colors.Black, 0.82);

            DrawMask(dc, devlogMask, devlogBounds, option.Title.X + 9, option.Title.Y + 10, devlogWidth, titleHeight, Colors.Black, 0.72);
            DrawMask(dc, devlogMask, devlogBounds, option.Title.X + 3, option.Title.Y + 3, devlogWidth, titleHeight, coolEdge, 0.72);
            DrawMask(dc, devlogMask, devlogBounds, option.Title.X, option.Title.Y, devlogWidth, titleHeight, titleWhite, 1.0);
            DrawMask(dc, devlogMask, devlogBounds, option.Title.X, option.Title.Y - 2, devlogWidth, titleHeight, softBlue, 0.16);

            foreach (PlacedDigit digit in digits)
            {
                DrawMask(dc, digitMask, digit.SourceRect, digit.X + 9, digit.Y + 10, digit.Width, digit.Height, Colors.Black, 0.72);
                DrawMask(dc, digitMask, digit.SourceRect, digit.X + 3, digit.Y + 3, digit.Width, digit.Height, coolEdge, 0.72);
                DrawMask(dc, digitMask, digit.SourceRect, digit.X, digit.Y, digit.Width, digit.Height, titleWhite, 1.0);
                DrawMask(dc, digitMask, digit.SourceRect, digit.X, digit.Y - 2, digit.Width, digit.Height, softBlue, 0.16);
            }
        }

        var render = new RenderTargetBitmap(CanvasWidth, CanvasHeight, 96, 96, PixelFormats.Pbgra32);
        render.Render(visual);
        render.Freeze();
        return render;
    }

    private static IReadOnlyList<PlacedDigit> LayoutDigits(
        string episodeNumber,
        DigitAtlasMetadata atlas,
        double startX,
        double titleY,
        double digitHeight)
    {
        var placed = new List<PlacedDigit>();
        double x = startX;
        double gap = digitHeight * 0.02;

        foreach (char character in episodeNumber.Where(char.IsDigit))
        {
            DigitBounds bounds = atlas.Digits.First(digit => digit.Digit == character.ToString());
            double width = ScaleWidth(new Int32Rect(bounds.X, bounds.Y, bounds.Width, bounds.Height), digitHeight);
            double y = titleY + (digitHeight * 0.03);
            placed.Add(new PlacedDigit(new Int32Rect(bounds.X, bounds.Y, bounds.Width, bounds.Height), x, y, width, digitHeight));
            x += width + gap;
        }

        return placed;
    }

    private static double ScaleWidth(Int32Rect bounds, double targetHeight)
    {
        return Math.Max(1.0, targetHeight * bounds.Width / Math.Max(1.0, bounds.Height));
    }

    private static void DrawBackground(DrawingContext dc, BitmapSource source)
    {
        double sourceRatio = source.PixelWidth / (double)source.PixelHeight;
        double targetRatio = CanvasWidth / (double)CanvasHeight;

        int cropWidth = source.PixelWidth;
        int cropHeight = source.PixelHeight;
        int cropX = 0;
        int cropY = 0;

        if (sourceRatio > targetRatio)
        {
            cropWidth = (int)Math.Round(source.PixelHeight * targetRatio);
            cropX = (source.PixelWidth - cropWidth) / 2;
        }
        else if (sourceRatio < targetRatio)
        {
            cropHeight = (int)Math.Round(source.PixelWidth / targetRatio);
            cropY = (source.PixelHeight - cropHeight) / 2;
        }

        var crop = new CroppedBitmap(source, new Int32Rect(cropX, cropY, cropWidth, cropHeight));
        dc.DrawImage(crop, new Rect(0, 0, CanvasWidth, CanvasHeight));
    }

    private static void DrawMask(
        DrawingContext dc,
        BitmapSource mask,
        Int32Rect sourceRect,
        double x,
        double y,
        double width,
        double height,
        Color color,
        double opacity)
    {
        BitmapSource layer = CreateColorLayer(mask, sourceRect, Math.Ceiling(width), Math.Ceiling(height), color, opacity);
        dc.DrawImage(layer, new Rect(x, y, width, height));
    }

    private static BitmapSource CreateColorLayer(
        BitmapSource mask,
        Int32Rect sourceRect,
        double targetWidth,
        double targetHeight,
        Color color,
        double opacity)
    {
        int width = Math.Max(1, (int)targetWidth);
        int height = Math.Max(1, (int)targetHeight);
        var crop = new CroppedBitmap(mask, sourceRect);
        var scaled = new TransformedBitmap(crop, new ScaleTransform(width / (double)sourceRect.Width, height / (double)sourceRect.Height));
        BitmapSource bgra = ToBgra32(scaled);

        int stride = width * 4;
        var sourcePixels = new byte[stride * height];
        bgra.CopyPixels(sourcePixels, stride, 0);

        var output = new byte[sourcePixels.Length];
        for (int i = 0; i < sourcePixels.Length; i += 4)
        {
            int luminance = Math.Max(sourcePixels[i + 2], Math.Max(sourcePixels[i + 1], sourcePixels[i]));
            double alphaValue = Math.Clamp((luminance - 5) / 250.0, 0.0, 1.0) * opacity;
            byte alpha = (byte)Math.Round(alphaValue * 255.0);

            output[i] = Premultiply(color.B, alpha);
            output[i + 1] = Premultiply(color.G, alpha);
            output[i + 2] = Premultiply(color.R, alpha);
            output[i + 3] = alpha;
        }

        BitmapSource result = BitmapSource.Create(width, height, 96, 96, PixelFormats.Pbgra32, null, output, stride);
        result.Freeze();
        return result;
    }

    private static byte Premultiply(byte channel, byte alpha)
    {
        return (byte)Math.Round(channel * alpha / 255.0);
    }

    private static DigitAtlasMetadata EnsureDigitMetadata(BitmapSource digitMask, string ytDirectory)
    {
        string metadataPath = Path.Combine(ytDirectory, DigitMetadataFileName);
        var jsonOptions = new JsonSerializerOptions
        {
            WriteIndented = true,
            DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull,
        };

        if (File.Exists(metadataPath))
        {
            try
            {
                DigitAtlasMetadata? existing = JsonSerializer.Deserialize<DigitAtlasMetadata>(
                    File.ReadAllText(metadataPath),
                    jsonOptions);
                if (existing is not null &&
                    existing.SourceImage == DigitMaskFileName &&
                    existing.SourceWidth == digitMask.PixelWidth &&
                    existing.SourceHeight == digitMask.PixelHeight &&
                    existing.Digits.Count == 10)
                {
                    return existing;
                }
            }
            catch (JsonException)
            {
                // Regenerate malformed metadata below.
            }
        }

        IReadOnlyList<DigitBounds> bounds = AnalyzeDigitBounds(digitMask);
        var metadata = new DigitAtlasMetadata(
            DigitMaskFileName,
            digitMask.PixelWidth,
            digitMask.PixelHeight,
            DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss"),
            bounds);

        File.WriteAllText(metadataPath, JsonSerializer.Serialize(metadata, jsonOptions));
        return metadata;
    }

    private static IReadOnlyList<DigitBounds> AnalyzeDigitBounds(BitmapSource source)
    {
        BitmapSource bgra = ToBgra32(source);
        int width = bgra.PixelWidth;
        int height = bgra.PixelHeight;
        int stride = width * 4;
        var pixels = new byte[stride * height];
        bgra.CopyPixels(pixels, stride, 0);

        var activeColumns = new bool[width];
        for (int x = 0; x < width; ++x)
        {
            int count = 0;
            for (int y = 0; y < height; ++y)
            {
                int index = y * stride + x * 4;
                int luminance = Math.Max(pixels[index + 2], Math.Max(pixels[index + 1], pixels[index]));
                if (luminance > 52)
                {
                    ++count;
                }
            }

            activeColumns[x] = count > 8;
        }

        var runs = new List<(int Start, int End)>();
        int start = -1;
        int lastActive = -1;
        int gap = 0;
        const int maxInternalGap = 12;

        for (int x = 0; x < width; ++x)
        {
            if (activeColumns[x])
            {
                if (start < 0)
                {
                    start = x;
                }

                lastActive = x;
                gap = 0;
                continue;
            }

            if (start < 0)
            {
                continue;
            }

            ++gap;
            if (gap > maxInternalGap)
            {
                runs.Add((start, lastActive));
                start = -1;
                lastActive = -1;
                gap = 0;
            }
        }

        if (start >= 0)
        {
            runs.Add((start, lastActive));
        }

        runs = runs.Where(run => run.End - run.Start > 24).ToList();
        if (runs.Count != 10)
        {
            throw new InvalidOperationException($"Expected 10 digit mask regions, found {runs.Count}.");
        }

        var result = new List<DigitBounds>(10);
        for (int digit = 0; digit < runs.Count; ++digit)
        {
            (int runStart, int runEnd) = runs[digit];
            int minX = runStart;
            int maxX = runEnd;
            int minY = height - 1;
            int maxY = 0;

            for (int y = 0; y < height; ++y)
            {
                for (int x = runStart; x <= runEnd; ++x)
                {
                    int index = y * stride + x * 4;
                    int luminance = Math.Max(pixels[index + 2], Math.Max(pixels[index + 1], pixels[index]));
                    if (luminance <= 52)
                    {
                        continue;
                    }

                    minX = Math.Min(minX, x);
                    maxX = Math.Max(maxX, x);
                    minY = Math.Min(minY, y);
                    maxY = Math.Max(maxY, y);
                }
            }

            const int padding = 8;
            minX = Math.Max(0, minX - padding);
            minY = Math.Max(0, minY - padding);
            maxX = Math.Min(width - 1, maxX + padding);
            maxY = Math.Min(height - 1, maxY + padding);
            int glyphWidth = maxX - minX + 1;
            int glyphHeight = maxY - minY + 1;
            result.Add(new DigitBounds(digit.ToString(), minX, minY, glyphWidth, glyphHeight, glyphWidth + 10));
        }

        return result;
    }

    private static Int32Rect FindMaskBounds(BitmapSource source, int threshold, int padding)
    {
        BitmapSource bgra = ToBgra32(source);
        int width = bgra.PixelWidth;
        int height = bgra.PixelHeight;
        int stride = width * 4;
        var pixels = new byte[stride * height];
        bgra.CopyPixels(pixels, stride, 0);

        int minX = width - 1;
        int minY = height - 1;
        int maxX = 0;
        int maxY = 0;
        bool found = false;

        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                int index = y * stride + x * 4;
                int luminance = Math.Max(pixels[index + 2], Math.Max(pixels[index + 1], pixels[index]));
                if (luminance <= threshold)
                {
                    continue;
                }

                minX = Math.Min(minX, x);
                minY = Math.Min(minY, y);
                maxX = Math.Max(maxX, x);
                maxY = Math.Max(maxY, y);
                found = true;
            }
        }

        if (!found)
        {
            throw new InvalidOperationException("Mask has no visible pixels.");
        }

        minX = Math.Max(0, minX - padding);
        minY = Math.Max(0, minY - padding);
        maxX = Math.Min(width - 1, maxX + padding);
        maxY = Math.Min(height - 1, maxY + padding);
        return new Int32Rect(minX, minY, maxX - minX + 1, maxY - minY + 1);
    }

    private static BitmapSource LoadBitmap(string path)
    {
        if (!File.Exists(path))
        {
            throw new FileNotFoundException($"Missing image: {path}", path);
        }

        var image = new BitmapImage();
        image.BeginInit();
        image.CacheOption = BitmapCacheOption.OnLoad;
        image.UriSource = new Uri(path, UriKind.Absolute);
        image.EndInit();
        image.Freeze();
        return image;
    }

    private static BitmapSource ToBgra32(BitmapSource source)
    {
        if (source.Format == PixelFormats.Bgra32 || source.Format == PixelFormats.Pbgra32)
        {
            return source;
        }

        var converted = new FormatConvertedBitmap(source, PixelFormats.Bgra32, null, 0);
        converted.Freeze();
        return converted;
    }

    private static string GetYtDirectory()
    {
        DirectoryInfo? directory = new(AppContext.BaseDirectory);
        while (directory is not null)
        {
            string yt = Path.Combine(directory.FullName, "YT");
            if (File.Exists(Path.Combine(yt, "BG_A.png")))
            {
                return yt;
            }

            directory = directory.Parent;
        }

        throw new DirectoryNotFoundException("Could not locate the repository YT directory.");
    }

    private sealed record PlacedDigit(Int32Rect SourceRect, double X, double Y, double Width, double Height)
    {
        public double Right => X + Width;
    }
}
