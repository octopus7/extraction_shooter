using Microsoft.VisualBasic.FileIO;
using System.IO;
using System.Reflection;
using System.Text.Json;

namespace TunaBuildHelper;

internal sealed record GrantItem(int Id, string Name)
{
    public string Label => $"{Id}  ·  {Name}";
}

internal static class ItemGrantCatalog
{
    private static readonly Dictionary<string, string> Strings = LoadStrings();
    internal static string Text(string key) => Strings.GetValueOrDefault(key, key);
    internal static readonly IReadOnlyList<GrantItem> Items = LoadItems();
    internal static bool IsFeatured(int id) => id is >= 6001 and <= 6035 or >= 7002 and <= 7004 or 3002 or 3004 or 3005;
    internal static IReadOnlyList<GrantItem> Featured => Items.Where(i => IsFeatured(i.Id)).OrderBy(i => i.Id == 6019 ? 0 : 1).ThenBy(i => i.Id).ToArray();
    private static Stream Resource(string name) => Assembly.GetExecutingAssembly().GetManifestResourceStream(name)!;
    private static Dictionary<string, string> LoadStrings()
    {
        Dictionary<string, string> result = new(StringComparer.Ordinal);
        foreach (var name in new[] { "UITextStrings.csv", "ItemNameStrings.csv" })
        {
            using var parser = new TextFieldParser(Resource(name), System.Text.Encoding.UTF8) { TextFieldType = FieldType.Delimited, HasFieldsEnclosedInQuotes = true };
            parser.SetDelimiters(",");
            int column = Array.IndexOf(parser.ReadFields()!, "ko");
            while (!parser.EndOfData)
            {
                var row = parser.ReadFields();
                if (row != null && column >= 0 && row.Length > column) result[row[0]] = row[column];
            }
        }
        return result;
    }
    private static IReadOnlyList<GrantItem> LoadItems()
    {
        using var stream = Resource("ItemTable.json");
        using var data = JsonDocument.Parse(stream);
        return data.RootElement.EnumerateArray().Select(i => new GrantItem(i.GetProperty("id").GetInt32(), Text(i.GetProperty("name_string_key").GetString()!))).ToArray();
    }
}
