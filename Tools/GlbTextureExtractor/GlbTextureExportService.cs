using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
using System.Text.Json.Nodes;

namespace GlbTextureExtractor;

internal sealed class GlbTextureExportService
{
    public ExportResult Export(ExportOptions options)
    {
        var result = new ExportResult();
        Directory.CreateDirectory(options.OutputDirectory);

        var document = GlbDocument.Load(options.InputPath);
        var modelName = SanitizeName(Path.GetFileNameWithoutExtension(options.InputPath));
        document.RenameAssets(modelName);

        var textureDirectory = Path.Combine(options.OutputDirectory, "Textures");
        Directory.CreateDirectory(textureDirectory);
        var usages = GetTextureUsages(document.Root);
        var externalTextureBindings = new List<ExternalTextureBinding>();
        var slotOrdinals = new Dictionary<string, int>(StringComparer.OrdinalIgnoreCase);
        foreach (var usage in usages)
        {
            var ordinal = slotOrdinals.TryGetValue(usage.Slot, out var currentOrdinal) ? currentOrdinal + 1 : 1;
            slotOrdinals[usage.Slot] = ordinal;
            var outputFileName = $"T_{modelName}_{GetSlotSuffix(usage.Slot)}_{ordinal:00}.png";
            try
            {
                var images = document.Root["images"]?.AsArray()
                    ?? throw new InvalidDataException("GLB에 images 배열이 없습니다.");
                var image = images[usage.ImageIndex]?.AsObject()
                    ?? throw new InvalidDataException($"존재하지 않는 image {usage.ImageIndex}입니다.");
                var sourceBytes = document.GetImageBytes(image, options.InputPath);
                if (IsKtx2(sourceBytes))
                {
                    result.SkippedTextureCount++;
                    result.Messages.Add($"건너뜀 (KTX2): image {usage.ImageIndex}, {usage.MaterialName}, {usage.Slot}");
                }
                else
                {
                    var outputPath = Path.Combine(textureDirectory, outputFileName);
                    var targetLongestEdge = usage.Slot.Equals("BaseColor", StringComparison.OrdinalIgnoreCase)
                        ? options.BaseColorTargetLongestEdge
                        : options.OtherTextureTargetLongestEdge;
                    var resizeResult = ResampleToPng(sourceBytes, outputPath, targetLongestEdge);
                    result.ExportedTextureCount++;
                    result.Messages.Add($"추출: {outputFileName} ({resizeResult.SourceWidth}x{resizeResult.SourceHeight} → {resizeResult.OutputWidth}x{resizeResult.OutputHeight}, {usage.MaterialName}, {usage.Slot})");
                    if (usage.MaterialIndex is int materialIndex)
                    {
                        externalTextureBindings.Add(new ExternalTextureBinding(
                            materialIndex,
                            usage.Slot,
                            $"Textures/{outputFileName}",
                            usage.SamplerIndex));
                    }
                }
            }
            catch (Exception exception) when (exception is ArgumentException or ExternalException or InvalidDataException or NotSupportedException or IOException)
            {
                result.SkippedTextureCount++;
                result.Messages.Add($"건너뜀: {outputFileName} - {exception.Message}");
            }
        }

        if (options.WriteRenamedGlbCopy)
        {
            var externalizedTextures = document.ExternalizeTextures(externalTextureBindings);
            var renamedModelPath = Path.Combine(options.OutputDirectory, $"{modelName}_UE_Named.glb");
            document.Save(renamedModelPath);
            result.Messages.Add($"외부 텍스처 연결 GLB: {renamedModelPath}");
            result.Messages.Add($"연결: 외부 텍스처 {externalizedTextures.LinkedTextureCount}개 / 제거: 이미지 {externalizedTextures.ImageCount}개, bufferView {externalizedTextures.RemovedBufferViewCount}개, {externalizedTextures.RemovedByteCount:N0} bytes");
        }

        return result;
    }

    private static List<TextureUsage> GetTextureUsages(JsonObject root)
    {
        var usages = new List<TextureUsage>();
        var materials = root["materials"]?.AsArray();
        var textures = root["textures"]?.AsArray();
        if (textures is null)
        {
            return usages;
        }

        if (materials is not null)
        {
            for (var materialIndex = 0; materialIndex < materials.Count; materialIndex++)
            {
                if (materials[materialIndex] is not JsonObject material)
                {
                    continue;
                }

                var materialName = material["name"]?.GetValue<string>() ?? $"Material_{materialIndex:00}";
                if (material["pbrMetallicRoughness"] is JsonObject pbr)
                {
                    AddUsage(usages, pbr["baseColorTexture"]?.AsObject(), materialIndex, materialName, "BaseColor", textures);
                    AddUsage(usages, pbr["metallicRoughnessTexture"]?.AsObject(), materialIndex, materialName, "MetallicRoughness", textures);
                }

                AddUsage(usages, material["normalTexture"]?.AsObject(), materialIndex, materialName, "Normal", textures);
                AddUsage(usages, material["occlusionTexture"]?.AsObject(), materialIndex, materialName, "Occlusion", textures);
                AddUsage(usages, material["emissiveTexture"]?.AsObject(), materialIndex, materialName, "Emissive", textures);
            }
        }

        if (usages.Count > 0)
        {
            return usages;
        }

        var images = root["images"]?.AsArray();
        if (images is null)
        {
            return usages;
        }

        for (var imageIndex = 0; imageIndex < images.Count; imageIndex++)
        {
            usages.Add(new TextureUsage(imageIndex, null, "Unassigned", "Image", null));
        }

        return usages;
    }

    private static void AddUsage(List<TextureUsage> usages, JsonObject? textureInfo, int materialIndex, string materialName, string slot, JsonArray textures)
    {
        if (textureInfo?["index"] is not JsonValue textureIndexValue)
        {
            return;
        }

        var textureIndex = textureIndexValue.GetValue<int>();
        if (textureIndex < 0 || textureIndex >= textures.Count || textures[textureIndex] is not JsonObject texture)
        {
            throw new InvalidDataException($"{materialName}의 {slot} 텍스처 인덱스가 올바르지 않습니다.");
        }

        var imageIndexValue = texture["source"] as JsonValue;
        if (imageIndexValue is null && texture["extensions"]?.AsObject()?["KHR_texture_basisu"]?.AsObject()?["source"] is JsonValue basisuImageIndexValue)
        {
            imageIndexValue = basisuImageIndexValue;
        }

        if (imageIndexValue is null)
        {
            throw new NotSupportedException($"{materialName}의 {slot} 텍스처는 image source가 없습니다.");
        }

        usages.Add(new TextureUsage(
            imageIndexValue.GetValue<int>(),
            materialIndex,
            materialName,
            slot,
            texture["sampler"]?.GetValue<int>()));
    }

    private static ResizeResult ResampleToPng(byte[] sourceBytes, string outputPath, int configuredTargetLongestEdge)
    {
        using var inputStream = new MemoryStream(sourceBytes, writable: false);
        using var source = Image.FromStream(inputStream, useEmbeddedColorManagement: true, validateImageData: true);
        var targetLongestEdge = Math.Min(configuredTargetLongestEdge, Math.Max(source.Width, source.Height));
        var scale = (double)targetLongestEdge / Math.Max(source.Width, source.Height);
        var targetWidth = Math.Max(1, (int)Math.Round(source.Width * scale, MidpointRounding.AwayFromZero));
        var targetHeight = Math.Max(1, (int)Math.Round(source.Height * scale, MidpointRounding.AwayFromZero));

        using var target = new Bitmap(targetWidth, targetHeight, PixelFormat.Format32bppArgb);
        target.SetResolution(source.HorizontalResolution, source.VerticalResolution);
        using var graphics = Graphics.FromImage(target);
        graphics.Clear(Color.Transparent);
        graphics.CompositingMode = CompositingMode.SourceCopy;
        graphics.CompositingQuality = CompositingQuality.HighQuality;
        graphics.InterpolationMode = InterpolationMode.HighQualityBicubic;
        graphics.PixelOffsetMode = PixelOffsetMode.HighQuality;
        graphics.SmoothingMode = SmoothingMode.HighQuality;
        graphics.DrawImage(source, new Rectangle(0, 0, targetWidth, targetHeight));
        target.Save(outputPath, ImageFormat.Png);
        return new ResizeResult(source.Width, source.Height, targetWidth, targetHeight);
    }

    private static bool IsKtx2(ReadOnlySpan<byte> bytes)
    {
        ReadOnlySpan<byte> signature = [0xAB, 0x4B, 0x54, 0x58, 0x20, 0x32, 0x30, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A];
        return bytes.Length >= signature.Length && bytes[..signature.Length].SequenceEqual(signature);
    }

    private static string GetSlotSuffix(string slot) => slot switch
    {
        "BaseColor" => "BaseColor",
        "MetallicRoughness" => "MetalRough",
        "Normal" => "Normal",
        "Occlusion" => "Occlusion",
        "Emissive" => "Emissive",
        _ => "Image"
    };

    private static string SanitizeName(string input)
    {
        var invalidCharacters = Path.GetInvalidFileNameChars();
        var sanitized = new string(input.Select(character => invalidCharacters.Contains(character) || char.IsWhiteSpace(character) ? '_' : character).ToArray());
        return string.IsNullOrWhiteSpace(sanitized) ? "Model" : sanitized;
    }

    private sealed record TextureUsage(int ImageIndex, int? MaterialIndex, string MaterialName, string Slot, int? SamplerIndex);
    private sealed record ResizeResult(int SourceWidth, int SourceHeight, int OutputWidth, int OutputHeight);
}

internal sealed record ExportOptions(
    string InputPath,
    string OutputDirectory,
    bool WriteRenamedGlbCopy,
    int BaseColorTargetLongestEdge,
    int OtherTextureTargetLongestEdge);

internal sealed class ExportResult
{
    public List<string> Messages { get; } = [];
    public int ExportedTextureCount { get; set; }
    public int SkippedTextureCount { get; set; }
}
