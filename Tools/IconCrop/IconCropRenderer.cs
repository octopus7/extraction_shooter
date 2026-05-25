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
    private const int CompletedEdgeGapTolerance = 4;

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
        byte alphaThreshold = 0,
        IList<string>? trace = null)
    {
        Int32Rect region = ToSafeInt32Rect(searchRect, alphaMask.Width, alphaMask.Height);
        trace?.Add($"FindAlphaTrimRect search={FormatRect(region)} alphaThreshold>{alphaThreshold} edgeGap={trimFromFirstEmptyAfterEdgeContent}");
        if (region.Width <= 0 || region.Height <= 0)
        {
            trace?.Add("Search region is empty.");
            return null;
        }

        EdgeScanResult scanResult = trimFromFirstEmptyAfterEdgeContent
            ? MovePastEdgeConnectedContent(alphaMask, region, alphaThreshold, trace)
            : new EdgeScanResult(region);
        Int32Rect scanRegion = scanResult.Region;
        trace?.Add($"Scan region={FormatRect(scanRegion)}");

        if (!alphaMask.TryFindOpaqueBounds(scanRegion, alphaThreshold, out Int32Rect bounds))
        {
            trace?.Add("No opaque bounds found in scan region.");
            if (scanRegion != region && alphaMask.TryFindOpaqueBounds(region, alphaThreshold, out bounds))
            {
                trace?.Add($"Fallback original region bounds={FormatRect(bounds)}");
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

        trace?.Add($"Opaque bounds before edge expansion={FormatRect(bounds)}");

        if (trimFromFirstEmptyAfterEdgeContent)
        {
            bounds = ExpandBoundsPastSearchEdges(alphaMask, bounds, region, alphaThreshold, trace);
            bounds = PreserveCompletedEdges(alphaMask, bounds, region, scanResult, alphaThreshold, trace);
            trace?.Add($"Opaque bounds after edge expansion={FormatRect(bounds)}");
        }

        return new CropRect
        {
            X = bounds.X,
            Y = bounds.Y,
            Width = bounds.Width,
            Height = bounds.Height
        };
    }

    public static CropRect? FindCenterOutCleanTrimRect(
        AlphaMask alphaMask,
        CropRect searchRect,
        byte alphaThreshold = 0,
        IList<string>? trace = null)
    {
        Int32Rect region = ToSafeInt32Rect(searchRect, alphaMask.Width, alphaMask.Height);
        trace?.Add($"FindCenterOutCleanTrimRect search={FormatRect(region)} alphaThreshold>{alphaThreshold}");
        if (region.Width <= 0 || region.Height <= 0)
        {
            trace?.Add("Search region is empty.");
            return null;
        }

        int centerX = region.X + ((region.Width - 1) / 2);
        int centerY = region.Y + ((region.Height - 1) / 2);
        int left = centerX;
        int right = centerX;
        int top = centerY;
        int bottom = centerY;
        bool leftSawOpaque = false;
        bool rightSawOpaque = false;
        bool topSawOpaque = false;
        bool bottomSawOpaque = false;

        trace?.Add($"Center-out seed center=({centerX}, {centerY})");
        while (true)
        {
            bool leftHasOpaque = alphaMask.ColumnHasOpaque(left, top, bottom + 1, alphaThreshold);
            bool rightHasOpaque = alphaMask.ColumnHasOpaque(right, top, bottom + 1, alphaThreshold);
            bool topHasOpaque = alphaMask.RowHasOpaque(top, left, right + 1, alphaThreshold);
            bool bottomHasOpaque = alphaMask.RowHasOpaque(bottom, left, right + 1, alphaThreshold);
            leftSawOpaque |= leftHasOpaque;
            rightSawOpaque |= rightHasOpaque;
            topSawOpaque |= topHasOpaque;
            bottomSawOpaque |= bottomHasOpaque;

            bool leftDone = leftSawOpaque && !leftHasOpaque;
            bool rightDone = rightSawOpaque && !rightHasOpaque;
            bool topDone = topSawOpaque && !topHasOpaque;
            bool bottomDone = bottomSawOpaque && !bottomHasOpaque;

            if (leftDone && rightDone && topDone && bottomDone)
            {
                var result = new Int32Rect(left, top, right - left + 1, bottom - top + 1);
                trace?.Add($"Center-out found clean edge rect={FormatRect(result)}");
                return new CropRect
                {
                    X = result.X,
                    Y = result.Y,
                    Width = result.Width,
                    Height = result.Height
                };
            }

            bool expanded = false;
            if (!leftDone && left > 0)
            {
                left--;
                expanded = true;
            }

            if (!rightDone && right + 1 < alphaMask.Width)
            {
                right++;
                expanded = true;
            }

            if (!topDone && top > 0)
            {
                top--;
                expanded = true;
            }

            if (!bottomDone && bottom + 1 < alphaMask.Height)
            {
                bottom++;
                expanded = true;
            }

            if (!expanded)
            {
                break;
            }
        }

        trace?.Add("Center-out reached sheet bounds before all edges became clean.");
        if (!alphaMask.TryFindOpaqueBounds(region, alphaThreshold, out Int32Rect fallbackBounds))
        {
            trace?.Add("No opaque bounds found in original region.");
            return null;
        }

        trace?.Add($"Center-out fallback original region bounds={FormatRect(fallbackBounds)}");
        return new CropRect
        {
            X = fallbackBounds.X,
            Y = fallbackBounds.Y,
            Width = fallbackBounds.Width,
            Height = fallbackBounds.Height
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

    private static string FormatRect(Int32Rect rect)
    {
        return $"x={rect.X}, y={rect.Y}, w={rect.Width}, h={rect.Height}, r={rect.X + rect.Width - 1}, b={rect.Y + rect.Height - 1}";
    }

    private static EdgeScanResult MovePastEdgeConnectedContent(
        AlphaMask alphaMask,
        Int32Rect region,
        byte alphaThreshold,
        IList<string>? trace)
    {
        int left = region.X;
        int top = region.Y;
        int right = region.X + region.Width - 1;
        int bottom = region.Y + region.Height - 1;

        trace?.Add($"MovePastEdgeConnectedContent start L={left}, T={top}, R={right}, B={bottom}");
        EdgeResolveResult leftResult = ResolveLeftEdge(alphaMask, left, right, top, bottom + 1, alphaThreshold, trace);
        left = leftResult.Value;
        EdgeResolveResult rightResult = ResolveRightEdge(alphaMask, left, right, top, bottom + 1, alphaThreshold, trace);
        right = rightResult.Value;

        if (left > right)
        {
            trace?.Add("Horizontal edge resolution inverted bounds; using original region.");
            return new EdgeScanResult(region);
        }

        EdgeResolveResult topResult = ResolveTopEdge(alphaMask, top, bottom, left, right + 1, alphaThreshold, trace);
        top = topResult.Value;
        EdgeResolveResult bottomResult = ResolveBottomEdge(alphaMask, top, bottom, left, right + 1, alphaThreshold, trace);
        bottom = bottomResult.Value;

        if (top > bottom)
        {
            trace?.Add("Vertical edge resolution inverted bounds; using original region.");
            return new EdgeScanResult(region);
        }

        trace?.Add($"MovePastEdgeConnectedContent result L={left}, T={top}, R={right}, B={bottom}");
        return new EdgeScanResult(
            new Int32Rect(left, top, right - left + 1, bottom - top + 1),
            leftResult.PreserveOriginalEdge,
            rightResult.PreserveOriginalEdge,
            topResult.PreserveOriginalEdge,
            bottomResult.PreserveOriginalEdge);
    }

    private static Int32Rect PreserveCompletedEdges(
        AlphaMask alphaMask,
        Int32Rect bounds,
        Int32Rect originalRegion,
        EdgeScanResult scanResult,
        byte alphaThreshold,
        IList<string>? trace)
    {
        int originalRight = originalRegion.X + originalRegion.Width - 1;
        int originalBottom = originalRegion.Y + originalRegion.Height - 1;

        int left = bounds.X;
        int top = bounds.Y;
        int right = bounds.X + bounds.Width - 1;
        int bottom = bounds.Y + bounds.Height - 1;

        if (scanResult.PreserveLeft)
        {
            int before = left;
            if (left > originalRegion.X ||
                (left < originalRegion.X && !AnyOpaqueColumnInRange(alphaMask, left, originalRegion.X - 1, top, bottom + 1, alphaThreshold)))
            {
                left = originalRegion.X;
            }

            if (left != before)
            {
                trace?.Add($"Preserve completed left edge from {before} to {left}.");
            }
        }

        if (scanResult.PreserveRight)
        {
            int before = right;
            if (right < originalRight ||
                (right > originalRight && !AnyOpaqueColumnInRange(alphaMask, originalRight + 1, right, top, bottom + 1, alphaThreshold)))
            {
                right = originalRight;
            }

            if (right != before)
            {
                trace?.Add($"Preserve completed right edge from {before} to {right}.");
            }
        }

        if (scanResult.PreserveTop)
        {
            int before = top;
            if (top > originalRegion.Y ||
                (top < originalRegion.Y && !AnyOpaqueRowInRange(alphaMask, top, originalRegion.Y - 1, left, right + 1, alphaThreshold)))
            {
                top = originalRegion.Y;
            }

            if (top != before)
            {
                trace?.Add($"Preserve completed top edge from {before} to {top}.");
            }
        }

        if (scanResult.PreserveBottom)
        {
            int before = bottom;
            if (bottom < originalBottom ||
                (bottom > originalBottom && !AnyOpaqueRowInRange(alphaMask, originalBottom + 1, bottom, left, right + 1, alphaThreshold)))
            {
                bottom = originalBottom;
            }

            if (bottom != before)
            {
                trace?.Add($"Preserve completed bottom edge from {before} to {bottom}.");
            }
        }

        return new Int32Rect(left, top, right - left + 1, bottom - top + 1);
    }

    private static Int32Rect ExpandBoundsPastSearchEdges(
        AlphaMask alphaMask,
        Int32Rect bounds,
        Int32Rect searchRegion,
        byte alphaThreshold,
        IList<string>? trace)
    {
        const int EmptyEdgeTolerance = 1;
        int searchRight = searchRegion.X + searchRegion.Width - 1;
        int searchBottom = searchRegion.Y + searchRegion.Height - 1;

        int left = bounds.X;
        int top = bounds.Y;
        int right = bounds.X + bounds.Width - 1;
        int bottom = bounds.Y + bounds.Height - 1;

        for (int iteration = 0; iteration < 4; iteration++)
        {
            int previousLeft = left;
            int previousTop = top;
            int previousRight = right;
            int previousBottom = bottom;

            if (left <= searchRegion.X + EmptyEdgeTolerance)
            {
                int before = left;
                left = ExpandLeftToOuterGap(alphaMask, left, top, bottom + 1, alphaThreshold);
                trace?.Add($"Expand left from {before} to {left}");
            }

            if (right >= searchRight - EmptyEdgeTolerance)
            {
                int before = right;
                right = ExpandRightToOuterGap(alphaMask, right, top, bottom + 1, alphaThreshold);
                trace?.Add($"Expand right from {before} to {right}");
            }

            if (top <= searchRegion.Y + EmptyEdgeTolerance)
            {
                int before = top;
                top = ExpandTopToOuterGap(alphaMask, top, left, right + 1, alphaThreshold);
                trace?.Add($"Expand top from {before} to {top}");
            }

            if (bottom >= searchBottom - EmptyEdgeTolerance)
            {
                int before = bottom;
                bottom = ExpandBottomToOuterGap(alphaMask, bottom, left, right + 1, alphaThreshold);
                trace?.Add($"Expand bottom from {before} to {bottom}");
            }

            if (left == previousLeft && top == previousTop && right == previousRight && bottom == previousBottom)
            {
                break;
            }
        }

        return new Int32Rect(left, top, right - left + 1, bottom - top + 1);
    }

    private static EdgeResolveResult ResolveLeftEdge(
        AlphaMask alphaMask,
        int left,
        int right,
        int top,
        int bottomExclusive,
        byte alphaThreshold,
        IList<string>? trace)
    {
        if (!alphaMask.ColumnHasOpaque(left, top, bottomExclusive, alphaThreshold))
        {
            bool preserve = HasNearOpaqueColumnFromLeft(alphaMask, left, right, top, bottomExclusive, alphaThreshold);
            trace?.Add(preserve
                ? $"Left edge column {left} is empty with nearby content; preserve completed empty edge."
                : $"Left edge column {left} is empty.");
            return new EdgeResolveResult(left, preserve);
        }

        for (int x = left + 1; x <= right; x++)
        {
            if (alphaMask.ColumnHasOpaque(x, top, bottomExclusive, alphaThreshold))
            {
                continue;
            }

            trace?.Add($"Left edge found inner empty column {x}.");
            double centerX = (left + right) * 0.5;
            if (x > centerX)
            {
                return ResolveLeftOutwardOrPreserve(
                    alphaMask,
                    left,
                    top,
                    bottomExclusive,
                    alphaThreshold,
                    trace,
                    $"Left edge inner empty column {x} crossed center {centerX:0.##}");
            }

            for (int nextX = x + 1; nextX <= right; nextX++)
            {
                if (alphaMask.ColumnHasOpaque(nextX, top, bottomExclusive, alphaThreshold))
                {
                    trace?.Add($"Left edge found next filled column {nextX}; trim starts there.");
                    return new EdgeResolveResult(nextX, false);
                }
            }

            return ResolveLeftOutwardOrPreserve(
                alphaMask,
                left,
                top,
                bottomExclusive,
                alphaThreshold,
                trace,
                "Left edge found no next filled column");
        }

        return ResolveLeftOutwardOrPreserve(
            alphaMask,
            left,
            top,
            bottomExclusive,
            alphaThreshold,
            trace,
            "Left edge found no inner empty column");
    }

    private static EdgeResolveResult ResolveRightEdge(
        AlphaMask alphaMask,
        int left,
        int right,
        int top,
        int bottomExclusive,
        byte alphaThreshold,
        IList<string>? trace)
    {
        if (!alphaMask.ColumnHasOpaque(right, top, bottomExclusive, alphaThreshold))
        {
            bool preserve = HasNearOpaqueColumnFromRight(alphaMask, left, right, top, bottomExclusive, alphaThreshold);
            trace?.Add(preserve
                ? $"Right edge column {right} is empty with nearby content; preserve completed empty edge."
                : $"Right edge column {right} is empty.");
            return new EdgeResolveResult(right, preserve);
        }

        for (int x = right - 1; x >= left; x--)
        {
            if (alphaMask.ColumnHasOpaque(x, top, bottomExclusive, alphaThreshold))
            {
                continue;
            }

            trace?.Add($"Right edge found inner empty column {x}.");
            double centerX = (left + right) * 0.5;
            if (x < centerX)
            {
                return ResolveRightOutwardOrPreserve(
                    alphaMask,
                    right,
                    top,
                    bottomExclusive,
                    alphaThreshold,
                    trace,
                    $"Right edge inner empty column {x} crossed center {centerX:0.##}");
            }

            for (int nextX = x - 1; nextX >= left; nextX--)
            {
                if (alphaMask.ColumnHasOpaque(nextX, top, bottomExclusive, alphaThreshold))
                {
                    trace?.Add($"Right edge found next filled column {nextX}; trim ends there.");
                    return new EdgeResolveResult(nextX, false);
                }
            }

            return ResolveRightOutwardOrPreserve(
                alphaMask,
                right,
                top,
                bottomExclusive,
                alphaThreshold,
                trace,
                "Right edge found no next filled column");
        }

        return ResolveRightOutwardOrPreserve(
            alphaMask,
            right,
            top,
            bottomExclusive,
            alphaThreshold,
            trace,
            "Right edge found no inner empty column");
    }

    private static EdgeResolveResult ResolveTopEdge(
        AlphaMask alphaMask,
        int top,
        int bottom,
        int left,
        int rightExclusive,
        byte alphaThreshold,
        IList<string>? trace)
    {
        if (!alphaMask.RowHasOpaque(top, left, rightExclusive, alphaThreshold))
        {
            bool preserve = HasNearOpaqueRowFromTop(alphaMask, top, bottom, left, rightExclusive, alphaThreshold);
            trace?.Add(preserve
                ? $"Top edge row {top} is empty with nearby content; preserve completed empty edge."
                : $"Top edge row {top} is empty.");
            return new EdgeResolveResult(top, preserve);
        }

        for (int y = top + 1; y <= bottom; y++)
        {
            if (alphaMask.RowHasOpaque(y, left, rightExclusive, alphaThreshold))
            {
                continue;
            }

            trace?.Add($"Top edge found inner empty row {y}.");
            double centerY = (top + bottom) * 0.5;
            if (y > centerY)
            {
                return ResolveTopOutwardOrPreserve(
                    alphaMask,
                    top,
                    left,
                    rightExclusive,
                    alphaThreshold,
                    trace,
                    $"Top edge inner empty row {y} crossed center {centerY:0.##}");
            }

            for (int nextY = y + 1; nextY <= bottom; nextY++)
            {
                if (alphaMask.RowHasOpaque(nextY, left, rightExclusive, alphaThreshold))
                {
                    trace?.Add($"Top edge found next filled row {nextY}; trim starts there.");
                    return new EdgeResolveResult(nextY, false);
                }
            }

            return ResolveTopOutwardOrPreserve(
                alphaMask,
                top,
                left,
                rightExclusive,
                alphaThreshold,
                trace,
                "Top edge found no next filled row");
        }

        return ResolveTopOutwardOrPreserve(
            alphaMask,
            top,
            left,
            rightExclusive,
            alphaThreshold,
            trace,
            "Top edge found no inner empty row");
    }

    private static EdgeResolveResult ResolveBottomEdge(
        AlphaMask alphaMask,
        int top,
        int bottom,
        int left,
        int rightExclusive,
        byte alphaThreshold,
        IList<string>? trace)
    {
        if (!alphaMask.RowHasOpaque(bottom, left, rightExclusive, alphaThreshold))
        {
            bool preserve = HasNearOpaqueRowFromBottom(alphaMask, top, bottom, left, rightExclusive, alphaThreshold);
            trace?.Add(preserve
                ? $"Bottom edge row {bottom} is empty with nearby content; preserve completed empty edge."
                : $"Bottom edge row {bottom} is empty.");
            return new EdgeResolveResult(bottom, preserve);
        }

        for (int y = bottom - 1; y >= top; y--)
        {
            if (alphaMask.RowHasOpaque(y, left, rightExclusive, alphaThreshold))
            {
                continue;
            }

            trace?.Add($"Bottom edge found inner empty row {y}.");
            double centerY = (top + bottom) * 0.5;
            if (y < centerY)
            {
                return ResolveBottomOutwardOrPreserve(
                    alphaMask,
                    bottom,
                    left,
                    rightExclusive,
                    alphaThreshold,
                    trace,
                    $"Bottom edge inner empty row {y} crossed center {centerY:0.##}");
            }

            for (int nextY = y - 1; nextY >= top; nextY--)
            {
                if (alphaMask.RowHasOpaque(nextY, left, rightExclusive, alphaThreshold))
                {
                    trace?.Add($"Bottom edge found next filled row {nextY}; trim ends there.");
                    return new EdgeResolveResult(nextY, false);
                }
            }

            return ResolveBottomOutwardOrPreserve(
                alphaMask,
                bottom,
                left,
                rightExclusive,
                alphaThreshold,
                trace,
                "Bottom edge found no next filled row");
        }

        return ResolveBottomOutwardOrPreserve(
            alphaMask,
            bottom,
            left,
            rightExclusive,
            alphaThreshold,
            trace,
            "Bottom edge found no inner empty row");
    }

    private static EdgeResolveResult ResolveLeftOutwardOrPreserve(
        AlphaMask alphaMask,
        int left,
        int top,
        int bottomExclusive,
        byte alphaThreshold,
        IList<string>? trace,
        string reason)
    {
        if (left > 0 && alphaMask.ColumnHasOpaque(left - 1, top, bottomExclusive, alphaThreshold))
        {
            int expanded = ExpandLeftToOuterGap(alphaMask, left, top, bottomExclusive, alphaThreshold);
            trace?.Add($"{reason}; outside column has alpha, expand outward to {expanded}.");
            return new EdgeResolveResult(expanded, false);
        }

        trace?.Add($"{reason}; outside is empty, keep completed edge {left}.");
        return new EdgeResolveResult(left, true);
    }

    private static EdgeResolveResult ResolveRightOutwardOrPreserve(
        AlphaMask alphaMask,
        int right,
        int top,
        int bottomExclusive,
        byte alphaThreshold,
        IList<string>? trace,
        string reason)
    {
        if (right + 1 < alphaMask.Width && alphaMask.ColumnHasOpaque(right + 1, top, bottomExclusive, alphaThreshold))
        {
            int expanded = ExpandRightToOuterGap(alphaMask, right, top, bottomExclusive, alphaThreshold);
            trace?.Add($"{reason}; outside column has alpha, expand outward to {expanded}.");
            return new EdgeResolveResult(expanded, false);
        }

        trace?.Add($"{reason}; outside is empty, keep completed edge {right}.");
        return new EdgeResolveResult(right, true);
    }

    private static EdgeResolveResult ResolveTopOutwardOrPreserve(
        AlphaMask alphaMask,
        int top,
        int left,
        int rightExclusive,
        byte alphaThreshold,
        IList<string>? trace,
        string reason)
    {
        if (top > 0 && alphaMask.RowHasOpaque(top - 1, left, rightExclusive, alphaThreshold))
        {
            int expanded = ExpandTopToOuterGap(alphaMask, top, left, rightExclusive, alphaThreshold);
            trace?.Add($"{reason}; outside row has alpha, expand outward to {expanded}.");
            return new EdgeResolveResult(expanded, false);
        }

        trace?.Add($"{reason}; outside is empty, keep completed edge {top}.");
        return new EdgeResolveResult(top, true);
    }

    private static EdgeResolveResult ResolveBottomOutwardOrPreserve(
        AlphaMask alphaMask,
        int bottom,
        int left,
        int rightExclusive,
        byte alphaThreshold,
        IList<string>? trace,
        string reason)
    {
        if (bottom + 1 < alphaMask.Height && alphaMask.RowHasOpaque(bottom + 1, left, rightExclusive, alphaThreshold))
        {
            int expanded = ExpandBottomToOuterGap(alphaMask, bottom, left, rightExclusive, alphaThreshold);
            trace?.Add($"{reason}; outside row has alpha, expand outward to {expanded}.");
            return new EdgeResolveResult(expanded, false);
        }

        trace?.Add($"{reason}; outside is empty, keep completed edge {bottom}.");
        return new EdgeResolveResult(bottom, true);
    }

    private static bool HasNearOpaqueColumnFromLeft(
        AlphaMask alphaMask,
        int left,
        int right,
        int top,
        int bottomExclusive,
        byte alphaThreshold)
    {
        return AnyOpaqueColumnInRange(
            alphaMask,
            left + 1,
            Math.Min(right, left + CompletedEdgeGapTolerance),
            top,
            bottomExclusive,
            alphaThreshold);
    }

    private static bool HasNearOpaqueColumnFromRight(
        AlphaMask alphaMask,
        int left,
        int right,
        int top,
        int bottomExclusive,
        byte alphaThreshold)
    {
        return AnyOpaqueColumnInRange(
            alphaMask,
            Math.Max(left, right - CompletedEdgeGapTolerance),
            right - 1,
            top,
            bottomExclusive,
            alphaThreshold);
    }

    private static bool HasNearOpaqueRowFromTop(
        AlphaMask alphaMask,
        int top,
        int bottom,
        int left,
        int rightExclusive,
        byte alphaThreshold)
    {
        return AnyOpaqueRowInRange(
            alphaMask,
            top + 1,
            Math.Min(bottom, top + CompletedEdgeGapTolerance),
            left,
            rightExclusive,
            alphaThreshold);
    }

    private static bool HasNearOpaqueRowFromBottom(
        AlphaMask alphaMask,
        int top,
        int bottom,
        int left,
        int rightExclusive,
        byte alphaThreshold)
    {
        return AnyOpaqueRowInRange(
            alphaMask,
            Math.Max(top, bottom - CompletedEdgeGapTolerance),
            bottom - 1,
            left,
            rightExclusive,
            alphaThreshold);
    }

    private static bool AnyOpaqueColumnInRange(
        AlphaMask alphaMask,
        int startInclusive,
        int endInclusive,
        int top,
        int bottomExclusive,
        byte alphaThreshold)
    {
        if (startInclusive > endInclusive || endInclusive < 0 || startInclusive >= alphaMask.Width)
        {
            return false;
        }

        int start = Math.Clamp(startInclusive, 0, alphaMask.Width - 1);
        int end = Math.Clamp(endInclusive, 0, alphaMask.Width - 1);
        for (int x = start; x <= end; x++)
        {
            if (alphaMask.ColumnHasOpaque(x, top, bottomExclusive, alphaThreshold))
            {
                return true;
            }
        }

        return false;
    }

    private static bool AnyOpaqueRowInRange(
        AlphaMask alphaMask,
        int startInclusive,
        int endInclusive,
        int left,
        int rightExclusive,
        byte alphaThreshold)
    {
        if (startInclusive > endInclusive || endInclusive < 0 || startInclusive >= alphaMask.Height)
        {
            return false;
        }

        int start = Math.Clamp(startInclusive, 0, alphaMask.Height - 1);
        int end = Math.Clamp(endInclusive, 0, alphaMask.Height - 1);
        for (int y = start; y <= end; y++)
        {
            if (alphaMask.RowHasOpaque(y, left, rightExclusive, alphaThreshold))
            {
                return true;
            }
        }

        return false;
    }

    private static int ExpandLeftToOuterGap(AlphaMask alphaMask, int left, int top, int bottomExclusive, byte alphaThreshold)
    {
        int x = left - 1;
        while (x >= 0 && alphaMask.ColumnHasOpaque(x, top, bottomExclusive, alphaThreshold))
        {
            x--;
        }

        return Math.Max(0, x);
    }

    private static int ExpandRightToOuterGap(AlphaMask alphaMask, int right, int top, int bottomExclusive, byte alphaThreshold)
    {
        int x = right + 1;
        while (x < alphaMask.Width && alphaMask.ColumnHasOpaque(x, top, bottomExclusive, alphaThreshold))
        {
            x++;
        }

        return Math.Min(alphaMask.Width - 1, x);
    }

    private static int ExpandTopToOuterGap(AlphaMask alphaMask, int top, int left, int rightExclusive, byte alphaThreshold)
    {
        int y = top - 1;
        while (y >= 0 && alphaMask.RowHasOpaque(y, left, rightExclusive, alphaThreshold))
        {
            y--;
        }

        return Math.Max(0, y);
    }

    private static int ExpandBottomToOuterGap(AlphaMask alphaMask, int bottom, int left, int rightExclusive, byte alphaThreshold)
    {
        int y = bottom + 1;
        while (y < alphaMask.Height && alphaMask.RowHasOpaque(y, left, rightExclusive, alphaThreshold))
        {
            y++;
        }

        return Math.Min(alphaMask.Height - 1, y);
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

    private readonly struct EdgeScanResult
    {
        public EdgeScanResult(
            Int32Rect region,
            bool preserveLeft = false,
            bool preserveRight = false,
            bool preserveTop = false,
            bool preserveBottom = false)
        {
            Region = region;
            PreserveLeft = preserveLeft;
            PreserveRight = preserveRight;
            PreserveTop = preserveTop;
            PreserveBottom = preserveBottom;
        }

        public Int32Rect Region { get; }

        public bool PreserveLeft { get; }

        public bool PreserveRight { get; }

        public bool PreserveTop { get; }

        public bool PreserveBottom { get; }
    }

    private readonly struct EdgeResolveResult
    {
        public EdgeResolveResult(int value, bool preserveOriginalEdge)
        {
            Value = value;
            PreserveOriginalEdge = preserveOriginalEdge;
        }

        public int Value { get; }

        public bool PreserveOriginalEdge { get; }
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
