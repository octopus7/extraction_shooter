using System.Text.Json;
using System.Text.Json.Serialization;

namespace IconCropTool;

public sealed class CropDocument
{
    [JsonPropertyName("sheetImageFilename")]
    public string SheetImageFilename { get; set; } = "";

    [JsonPropertyName("sheetSize")]
    public SheetSize? SheetSize { get; set; }

    [JsonPropertyName("grid")]
    public CropGrid? Grid { get; set; }

    [JsonPropertyName("cropMethod")]
    public string? CropMethod { get; set; }

    [JsonPropertyName("outputIconCanvas")]
    public OutputIconCanvas? OutputIconCanvas { get; set; }

    [JsonPropertyName("icons")]
    public List<IconEntry> Icons { get; set; } = [];

    [JsonExtensionData]
    public Dictionary<string, JsonElement>? ExtensionData { get; set; }
}

public sealed class SheetSize
{
    [JsonPropertyName("width")]
    public int Width { get; set; }

    [JsonPropertyName("height")]
    public int Height { get; set; }
}

public sealed class CropGrid
{
    [JsonPropertyName("columns")]
    public int Columns { get; set; }

    [JsonPropertyName("rows")]
    public int Rows { get; set; }

    [JsonPropertyName("cellWidth")]
    public double CellWidth { get; set; }

    [JsonPropertyName("cellHeight")]
    public double CellHeight { get; set; }

    [JsonPropertyName("cellInset")]
    public double CellInset { get; set; }
}

public sealed class OutputIconCanvas
{
    [JsonPropertyName("width")]
    public int Width { get; set; }

    [JsonPropertyName("height")]
    public int Height { get; set; }

    [JsonPropertyName("margin")]
    public int Margin { get; set; }
}

public sealed class IconEntry
{
    [JsonPropertyName("index")]
    public int Index { get; set; }

    [JsonPropertyName("row")]
    public int Row { get; set; }

    [JsonPropertyName("column")]
    public int Column { get; set; }

    [JsonPropertyName("iconFilename")]
    public string IconFilename { get; set; } = "";

    [JsonPropertyName("koreanName")]
    public string? KoreanName { get; set; }

    [JsonPropertyName("englishName")]
    public string? EnglishName { get; set; }

    [JsonPropertyName("sourceSheetFilename")]
    public string? SourceSheetFilename { get; set; }

    [JsonPropertyName("sourceCropRect")]
    public CropRect SourceCropRect { get; set; } = new();

    [JsonPropertyName("outputCanvas")]
    public CanvasSize? OutputCanvas { get; set; }

    [JsonPropertyName("outputPlacedRect")]
    public CropRect? OutputPlacedRect { get; set; }

    [JsonPropertyName("scale")]
    public double Scale { get; set; }

    [JsonPropertyName("notes")]
    public string? Notes { get; set; }

    [JsonExtensionData]
    public Dictionary<string, JsonElement>? ExtensionData { get; set; }
}

public sealed class CanvasSize
{
    [JsonPropertyName("width")]
    public int Width { get; set; }

    [JsonPropertyName("height")]
    public int Height { get; set; }
}

public sealed class CropRect
{
    [JsonPropertyName("x")]
    public double X { get; set; }

    [JsonPropertyName("y")]
    public double Y { get; set; }

    [JsonPropertyName("width")]
    public double Width { get; set; }

    [JsonPropertyName("height")]
    public double Height { get; set; }

    public CropRect Clone()
    {
        return new CropRect
        {
            X = X,
            Y = Y,
            Width = Width,
            Height = Height
        };
    }

    public void CopyFrom(CropRect other)
    {
        X = other.X;
        Y = other.Y;
        Width = other.Width;
        Height = other.Height;
    }
}
