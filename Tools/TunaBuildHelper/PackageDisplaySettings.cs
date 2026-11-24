using System.IO;

namespace TunaBuildHelper;

internal enum DisplayPreset { WindowedFhd, FullscreenFhd, FullscreenUhd }

internal static class PackageDisplaySettings
{
    internal static string SettingsPath => Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
        "TunaSweeper", "Saved", "Config", "Windows", "GameUserSettings.ini");

    internal static void Apply(string path, DisplayPreset preset)
    {
        const string section = "[/Script/TunaSweeper.TunaSweeperGameUserSettings]";
        bool uhd = preset == DisplayPreset.FullscreenUhd;
        string width = uhd ? "3840" : "1920";
        string height = uhd ? "2160" : "1080";
        string mode = preset == DisplayPreset.WindowedFhd ? "2" : "0";
        bool exists = File.Exists(path);
        var lines = exists ? File.ReadAllLines(path).ToList() : new List<string>();
        var values = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
        {
            ["ResolutionSizeX"] = width, ["ResolutionSizeY"] = height,
            ["LastUserConfirmedResolutionSizeX"] = width, ["LastUserConfirmedResolutionSizeY"] = height,
            ["FullscreenMode"] = mode, ["LastConfirmedFullscreenMode"] = mode, ["Version"] = "5"
        };
        if (preset != DisplayPreset.WindowedFhd)
        {
            values["PreferredFullscreenMode"] = "0";
            values["PreferredDLSSMode"] = uhd ? "Performance" : "Off";
            // Older profiles migrate this key over PreferredDLSSMode on their next launch.
            UpdateSection(lines, "[TunaSweeper.GraphicsSettings]", new(StringComparer.OrdinalIgnoreCase)
                { ["DLSSMode"] = uhd ? "3" : "0" }, updateExistingOnly: true);
        }
        if (!exists) values["GraphicsSettingsSchemaVersion"] = "1";
        UpdateSection(lines, section, values);
        string directory = Path.GetDirectoryName(path)!;
        Directory.CreateDirectory(directory);
        string temp = Path.Combine(directory, Path.GetRandomFileName());
        try
        {
            File.WriteAllLines(temp, lines);
            File.Move(temp, path, overwrite: true);
        }
        finally { File.Delete(temp); }
    }

    private static void UpdateSection(List<string> lines, string section,
        Dictionary<string, string> values, bool updateExistingOnly = false)
    {
        var missing = new Dictionary<string, string>(values, StringComparer.OrdinalIgnoreCase);
        bool inside = false;
        int insertAt = -1;
        for (int i = 0; i < lines.Count; ++i)
        {
            string line = lines[i].Trim();
            if (line.StartsWith('['))
                inside = line.Equals(section, StringComparison.OrdinalIgnoreCase);
            if (!inside) continue;
            insertAt = i + 1;
            int separator = line.IndexOf('=');
            if (separator < 1) continue;
            string key = line[..separator].Trim();
            if (values.TryGetValue(key, out string? value))
            {
                lines[i] = key + "=" + value;
                missing.Remove(key);
            }
        }
        if (updateExistingOnly || missing.Count == 0) return;
        if (insertAt < 0)
        {
            if (lines.Count > 0) lines.Add("");
            lines.Add(section);
            insertAt = lines.Count;
        }
        lines.InsertRange(insertAt, missing.Select(pair => pair.Key + "=" + pair.Value));
    }
}
