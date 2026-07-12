using System.Buffers.Binary;
using System.Text;
using System.Text.Json;
using System.Text.Json.Nodes;

namespace GlbTextureExtractor;

internal sealed class GlbDocument
{
    private const uint GlbMagic = 0x46546C67;
    private const uint JsonChunkType = 0x4E4F534A;
    private const uint BinaryChunkType = 0x004E4942;

    private readonly List<GlbChunk> _chunks;

    private GlbDocument(JsonObject root, List<GlbChunk> chunks)
    {
        Root = root;
        _chunks = chunks;
    }

    public JsonObject Root { get; }

    public static GlbDocument Load(string path)
    {
        var bytes = File.ReadAllBytes(path);
        if (bytes.Length < 20 || BinaryPrimitives.ReadUInt32LittleEndian(bytes) != GlbMagic)
        {
            throw new InvalidDataException("유효한 GLB 파일이 아닙니다.");
        }

        var version = BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(4));
        if (version != 2)
        {
            throw new InvalidDataException($"GLB 2.0만 지원합니다. 발견한 버전: {version}");
        }

        var declaredLength = BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(8));
        if (declaredLength != bytes.Length)
        {
            throw new InvalidDataException("GLB 헤더 길이가 실제 파일 길이와 다릅니다.");
        }

        var chunks = new List<GlbChunk>();
        for (var offset = 12; offset < bytes.Length;)
        {
            if (offset + 8 > bytes.Length)
            {
                throw new InvalidDataException("GLB 청크 헤더가 잘렸습니다.");
            }

            var length = checked((int)BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(offset)));
            var type = BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(offset + 4));
            offset += 8;
            if (length < 0 || offset + length > bytes.Length)
            {
                throw new InvalidDataException("GLB 청크 길이가 올바르지 않습니다.");
            }

            chunks.Add(new GlbChunk(type, bytes.AsSpan(offset, length).ToArray()));
            offset += length;
        }

        var jsonChunk = chunks.FirstOrDefault(chunk => chunk.Type == JsonChunkType)
            ?? throw new InvalidDataException("GLB JSON 청크가 없습니다.");
        var jsonText = Encoding.UTF8.GetString(jsonChunk.Data).TrimEnd(' ', '\0', '\r', '\n', '\t');
        var root = JsonNode.Parse(jsonText)?.AsObject()
            ?? throw new InvalidDataException("GLB JSON 청크를 해석할 수 없습니다.");
        return new GlbDocument(root, chunks);
    }

    public void Save(string path)
    {
        var json = Root.ToJsonString(new JsonSerializerOptions { WriteIndented = true });
        var jsonBytes = PadToFourBytes(Encoding.UTF8.GetBytes(json), (byte)' ');
        var chunks = _chunks.Select(chunk => chunk.Type == JsonChunkType
            ? new GlbChunk(JsonChunkType, jsonBytes)
            : chunk).ToList();
        var totalLength = checked(12 + chunks.Sum(chunk => 8 + chunk.Data.Length));

        Directory.CreateDirectory(Path.GetDirectoryName(path)!);
        using var stream = File.Create(path);
        using var writer = new BinaryWriter(stream, Encoding.UTF8, leaveOpen: false);
        writer.Write(GlbMagic);
        writer.Write(2u);
        writer.Write((uint)totalLength);
        foreach (var chunk in chunks)
        {
            writer.Write((uint)chunk.Data.Length);
            writer.Write(chunk.Type);
            writer.Write(chunk.Data);
        }
    }

    public byte[] GetImageBytes(JsonObject image, string inputPath)
    {
        if (image["bufferView"] is JsonValue bufferViewValue)
        {
            var bufferViewIndex = bufferViewValue.GetValue<int>();
            var bufferViews = Root["bufferViews"]?.AsArray()
                ?? throw new InvalidDataException("image.bufferView가 있지만 bufferViews가 없습니다.");
            var view = bufferViews[bufferViewIndex]?.AsObject()
                ?? throw new InvalidDataException($"존재하지 않는 bufferView {bufferViewIndex}입니다.");
            var bufferIndex = view["buffer"]?.GetValue<int>() ?? 0;
            if (bufferIndex != 0)
            {
                throw new NotSupportedException("여러 GLB 버퍼는 지원하지 않습니다.");
            }

            var binaryChunk = _chunks.FirstOrDefault(chunk => chunk.Type == BinaryChunkType)
                ?? throw new InvalidDataException("텍스처 bufferView에 대응하는 BIN 청크가 없습니다.");
            var offset = view["byteOffset"]?.GetValue<int>() ?? 0;
            var length = view["byteLength"]?.GetValue<int>()
                ?? throw new InvalidDataException("bufferView.byteLength가 없습니다.");
            if (offset < 0 || length <= 0 || offset + length > binaryChunk.Data.Length)
            {
                throw new InvalidDataException("텍스처 bufferView 범위가 BIN 청크 밖입니다.");
            }

            return binaryChunk.Data.AsSpan(offset, length).ToArray();
        }

        if (image["uri"] is not JsonValue uriValue)
        {
            throw new NotSupportedException("bufferView 또는 uri를 가진 이미지가 아닙니다.");
        }

        var uri = uriValue.GetValue<string>();
        if (uri.StartsWith("data:", StringComparison.OrdinalIgnoreCase))
        {
            var separator = uri.IndexOf(',');
            if (separator < 0 || !uri[..separator].Contains(";base64", StringComparison.OrdinalIgnoreCase))
            {
                throw new NotSupportedException("Base64 data URI만 지원합니다.");
            }

            return Convert.FromBase64String(uri[(separator + 1)..]);
        }

        var externalImagePath = Path.Combine(Path.GetDirectoryName(inputPath)!, Uri.UnescapeDataString(uri));
        return File.ReadAllBytes(externalImagePath);
    }

    public void RenameAssets(string modelName)
    {
        RenameNamedArray(Root["materials"]?.AsArray(), "M", modelName);
        RenameNamedArray(Root["meshes"]?.AsArray(), "SM", modelName);

        var meshes = Root["meshes"]?.AsArray();
        var nodes = Root["nodes"]?.AsArray();
        if (meshes is null || nodes is null)
        {
            return;
        }

        foreach (var node in nodes.OfType<JsonObject>())
        {
            if (node["mesh"] is not JsonValue meshIndexValue)
            {
                continue;
            }

            var meshIndex = meshIndexValue.GetValue<int>();
            if (meshIndex < 0 || meshIndex >= meshes.Count || meshes[meshIndex] is not JsonObject mesh)
            {
                continue;
            }

            node["name"] = mesh["name"]?.GetValue<string>() ?? $"SM_{modelName}";
        }
    }

    public TextureExternalizationResult ExternalizeTextures(IReadOnlyCollection<ExternalTextureBinding> bindings)
    {
        var images = Root["images"]?.AsArray();
        var imageCount = images?.Count ?? 0;
        var imageBufferViews = new HashSet<int>();
        if (images is not null)
        {
            foreach (var image in images.OfType<JsonObject>())
            {
                if (image["bufferView"] is JsonValue bufferViewValue)
                {
                    imageBufferViews.Add(bufferViewValue.GetValue<int>());
                }
            }
        }

        RemoveUnboundMaterialTextureSlots(bindings);
        Root.Remove("images");
        Root.Remove("textures");
        RemoveTextureExtensions();

        var removedBufferViewCount = 0;
        var removedBytes = 0;
        var remainingBinaryByteCount = GetBinaryChunk()?.Data.Length ?? 0;
        if (imageBufferViews.Count > 0)
        {
            var oldBufferViews = Root["bufferViews"]?.AsArray()
                ?? throw new InvalidDataException("이미지가 bufferView를 사용하지만 bufferViews 배열이 없습니다.");
            var binaryChunk = GetBinaryChunk()
                ?? throw new InvalidDataException("이미지가 bufferView를 사용하지만 BIN 청크가 없습니다.");

            using var compactedBuffer = new MemoryStream();
            var replacementBufferViews = new JsonArray();
            var remappedBufferViews = new Dictionary<int, int>();
            for (var index = 0; index < oldBufferViews.Count; index++)
            {
                if (oldBufferViews[index] is not JsonObject oldView)
                {
                    throw new InvalidDataException($"bufferView {index}이 올바른 객체가 아닙니다.");
                }

                var byteOffset = oldView["byteOffset"]?.GetValue<int>() ?? 0;
                var byteLength = oldView["byteLength"]?.GetValue<int>()
                    ?? throw new InvalidDataException($"bufferView {index}에 byteLength가 없습니다.");
                if (byteOffset < 0 || byteLength < 0 || byteOffset + byteLength > binaryChunk.Data.Length)
                {
                    throw new InvalidDataException($"bufferView {index}의 BIN 범위가 올바르지 않습니다.");
                }

                if (imageBufferViews.Contains(index))
                {
                    removedBytes += byteLength;
                    continue;
                }

                var newOffset = checked((int)compactedBuffer.Position);
                compactedBuffer.Write(binaryChunk.Data, byteOffset, byteLength);
                PadStreamToFourBytes(compactedBuffer);
                var replacementView = JsonNode.Parse(oldView.ToJsonString())?.AsObject()
                    ?? throw new InvalidDataException($"bufferView {index}을 복사할 수 없습니다.");
                replacementView["buffer"] = 0;
                replacementView["byteOffset"] = newOffset;
                remappedBufferViews[index] = replacementBufferViews.Count;
                replacementBufferViews.Add(replacementView);
            }

            Root["bufferViews"] = replacementBufferViews;
            RemapBufferViewReferences(Root, remappedBufferViews);
            var compactedBytes = compactedBuffer.ToArray();
            ReplaceBinaryChunk(compactedBytes);
            if (Root["buffers"]?.AsArray()?[0] is JsonObject buffer)
            {
                buffer["byteLength"] = compactedBytes.Length;
            }

            removedBufferViewCount = imageBufferViews.Count;
            remainingBinaryByteCount = compactedBytes.Length;
        }

        AddExternalTextureReferences(bindings);
        return new TextureExternalizationResult(imageCount, removedBufferViewCount, removedBytes, remainingBinaryByteCount, bindings.Count);
    }

    private static void RenameNamedArray(JsonArray? entries, string prefix, string modelName)
    {
        if (entries is null)
        {
            return;
        }

        var suffixRequired = entries.Count > 1;
        for (var index = 0; index < entries.Count; index++)
        {
            if (entries[index] is JsonObject entry)
            {
                entry["name"] = suffixRequired
                    ? $"{prefix}_{modelName}_{index + 1:00}"
                    : $"{prefix}_{modelName}";
            }
        }
    }

    private void RemoveUnboundMaterialTextureSlots(IReadOnlyCollection<ExternalTextureBinding> bindings)
    {
        if (Root["materials"]?.AsArray() is not JsonArray materials)
        {
            return;
        }

        var boundSlots = bindings.Select(binding => (binding.MaterialIndex, binding.Slot)).ToHashSet();
        for (var materialIndex = 0; materialIndex < materials.Count; materialIndex++)
        {
            if (materials[materialIndex] is not JsonObject material)
            {
                continue;
            }

            if (material["pbrMetallicRoughness"]?.AsObject() is JsonObject pbr)
            {
                if (!boundSlots.Contains((materialIndex, "BaseColor")))
                {
                    pbr.Remove("baseColorTexture");
                }

                if (!boundSlots.Contains((materialIndex, "MetallicRoughness")))
                {
                    pbr.Remove("metallicRoughnessTexture");
                }
            }

            if (!boundSlots.Contains((materialIndex, "Normal"))) material.Remove("normalTexture");
            if (!boundSlots.Contains((materialIndex, "Occlusion"))) material.Remove("occlusionTexture");
            if (!boundSlots.Contains((materialIndex, "Emissive"))) material.Remove("emissiveTexture");
        }
    }

    private void AddExternalTextureReferences(IReadOnlyCollection<ExternalTextureBinding> bindings)
    {
        if (bindings.Count == 0)
        {
            return;
        }

        var images = new JsonArray();
        var textures = new JsonArray();
        foreach (var binding in bindings)
        {
            var textureName = Path.GetFileNameWithoutExtension(binding.RelativeUri);
            var imageIndex = images.Count;
            images.Add(new JsonObject
            {
                ["name"] = textureName,
                ["uri"] = binding.RelativeUri
            });

            var texture = new JsonObject
            {
                ["name"] = textureName,
                ["source"] = imageIndex
            };
            if (binding.SamplerIndex is int samplerIndex)
            {
                texture["sampler"] = samplerIndex;
            }

            var textureIndex = textures.Count;
            textures.Add(texture);
            SetMaterialTextureIndex(binding.MaterialIndex, binding.Slot, textureIndex);
        }

        Root["images"] = images;
        Root["textures"] = textures;
    }

    private void SetMaterialTextureIndex(int materialIndex, string slot, int textureIndex)
    {
        var materials = Root["materials"]?.AsArray()
            ?? throw new InvalidDataException("외부 텍스처를 연결할 materials 배열이 없습니다.");
        var material = materials[materialIndex]?.AsObject()
            ?? throw new InvalidDataException($"외부 텍스처를 연결할 material {materialIndex}가 없습니다.");
        var (container, propertyName) = slot switch
        {
            "BaseColor" => (GetOrCreatePbr(material), "baseColorTexture"),
            "MetallicRoughness" => (GetOrCreatePbr(material), "metallicRoughnessTexture"),
            "Normal" => (material, "normalTexture"),
            "Occlusion" => (material, "occlusionTexture"),
            "Emissive" => (material, "emissiveTexture"),
            _ => throw new NotSupportedException($"외부 텍스처를 연결할 수 없는 슬롯입니다: {slot}")
        };

        if (container[propertyName]?.AsObject() is JsonObject textureInfo)
        {
            textureInfo["index"] = textureIndex;
        }
        else
        {
            container[propertyName] = new JsonObject { ["index"] = textureIndex };
        }
    }

    private static JsonObject GetOrCreatePbr(JsonObject material)
    {
        if (material["pbrMetallicRoughness"]?.AsObject() is JsonObject pbr)
        {
            return pbr;
        }

        pbr = new JsonObject();
        material["pbrMetallicRoughness"] = pbr;
        return pbr;
    }

    private void RemoveTextureExtensions()
    {
        foreach (var propertyName in new[] { "extensionsUsed", "extensionsRequired" })
        {
            if (Root[propertyName]?.AsArray() is not JsonArray extensions)
            {
                continue;
            }

            for (var index = extensions.Count - 1; index >= 0; index--)
            {
                var extension = extensions[index]?.GetValue<string>();
                if (extension is "KHR_texture_basisu" or "EXT_texture_webp" or "MSFT_texture_dds")
                {
                    extensions.RemoveAt(index);
                }
            }
        }
    }

    private GlbChunk? GetBinaryChunk() => _chunks.FirstOrDefault(chunk => chunk.Type == BinaryChunkType);

    private void ReplaceBinaryChunk(byte[] data)
    {
        for (var index = 0; index < _chunks.Count; index++)
        {
            if (_chunks[index].Type == BinaryChunkType)
            {
                _chunks[index] = new GlbChunk(BinaryChunkType, PadToFourBytes(data, 0));
                return;
            }
        }

        throw new InvalidDataException("교체할 BIN 청크가 없습니다.");
    }

    private static void RemapBufferViewReferences(JsonNode node, IReadOnlyDictionary<int, int> remappedBufferViews)
    {
        switch (node)
        {
            case JsonObject objectNode:
                foreach (var property in objectNode.ToList())
                {
                    if (property.Key == "bufferView" && property.Value is JsonValue bufferViewValue)
                    {
                        var oldIndex = bufferViewValue.GetValue<int>();
                        if (!remappedBufferViews.TryGetValue(oldIndex, out var newIndex))
                        {
                            throw new InvalidDataException($"제거한 이미지 bufferView {oldIndex}이 다른 GLB 데이터에서 참조됩니다.");
                        }

                        objectNode[property.Key] = newIndex;
                    }
                    else if (property.Value is not null)
                    {
                        RemapBufferViewReferences(property.Value, remappedBufferViews);
                    }
                }

                break;
            case JsonArray arrayNode:
                foreach (var child in arrayNode)
                {
                    if (child is not null)
                    {
                        RemapBufferViewReferences(child, remappedBufferViews);
                    }
                }

                break;
        }
    }

    private static void PadStreamToFourBytes(Stream stream)
    {
        var paddingLength = (int)((4 - stream.Length % 4) % 4);
        for (var index = 0; index < paddingLength; index++)
        {
            stream.WriteByte(0);
        }
    }

    private static byte[] PadToFourBytes(byte[] source, byte padding)
    {
        var paddingLength = (4 - source.Length % 4) % 4;
        if (paddingLength == 0)
        {
            return source;
        }

        var padded = new byte[source.Length + paddingLength];
        Buffer.BlockCopy(source, 0, padded, 0, source.Length);
        Array.Fill(padded, padding, source.Length, paddingLength);
        return padded;
    }

    private sealed record GlbChunk(uint Type, byte[] Data);
}

internal sealed record ExternalTextureBinding(int MaterialIndex, string Slot, string RelativeUri, int? SamplerIndex);

internal sealed record TextureExternalizationResult(int ImageCount, int RemovedBufferViewCount, int RemovedByteCount, int RemainingBinaryByteCount, int LinkedTextureCount);
