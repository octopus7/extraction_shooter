using Microsoft.VisualBasic.FileIO;

namespace TunaSweeper.SaveTool;
public sealed class Localization
{
    private readonly Dictionary<string, string> entries = new(StringComparer.Ordinal);
    public Localization(string? project, string language)
    {
        var path = project is null ? Path.Combine(AppContext.BaseDirectory, "UITextStrings.csv")
            : Path.Combine(Path.GetDirectoryName(project)!, "Content/Data/UITextStrings.csv");
        if (!File.Exists(path)) return;
        using var parser = new TextFieldParser(path, System.Text.Encoding.UTF8) { TextFieldType = FieldType.Delimited, HasFieldsEnclosedInQuotes = true };
        parser.SetDelimiters(",");
        var header = parser.ReadFields() ?? [];
        int column = Array.IndexOf(header, language);
        int english = Array.IndexOf(header, "en");
        while (!parser.EndOfData)
        {
            var row = parser.ReadFields();
            if (row is null || row.Length == 0) continue;
            string value = column >= 0 && column < row.Length ? row[column] : "";
            if (string.IsNullOrEmpty(value) && english >= 0 && english < row.Length) value = row[english];
            if (!string.IsNullOrEmpty(value)) entries[row[0]] = value;
        }
    }
    public string this[string key] => entries.GetValueOrDefault(key, key);
}
