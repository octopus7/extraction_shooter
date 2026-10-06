using System.Text.Json.Nodes;
namespace TunaSweeper.SaveTool;
public static class Cli
{
    private static readonly HashSet<string> Switches = ["json", "commit", "help"];
    private static readonly HashSet<string> Common = ["project", "editor", "lang", "json", "timeout", "help"];
    public static Dictionary<string, string> Parse(string[] args)
    {
        var options = new Dictionary<string, string>(StringComparer.Ordinal);
        if (args.Length == 0) return options;
        for (int i = 1; i < args.Length; i++)
        {
            if (!args[i].StartsWith("--", StringComparison.Ordinal)) throw new ArgumentException(args[i]);
            var key = args[i][2..];
            var value = "true";
            if (!Switches.Contains(key))
            {
                if (++i >= args.Length || args[i].StartsWith("--", StringComparison.Ordinal)) throw new ArgumentException(key);
                value = args[i];
            }
            if (!options.TryAdd(key, value)) throw new ArgumentException(key);
        }
        return options;
    }

    public static JsonObject BuildRequest(string[] args)
    {
        if (args.Length == 0) throw new ArgumentException("operation");
        var op = args[0];
        var o = Parse(args);
        HashSet<string> allowed = new(Common);
        bool editing = op is "add" or "preset-apply";
        if (op is "inspect" or "validate" or "add" or "preset-export" or "preset-apply")
            allowed.UnionWith(["save", "flavor", "slot"]);
        else if (op == "list") allowed.Add("directory");
        else if (op != "catalog") throw new ArgumentException("operation");
        if (editing) allowed.UnionWith(["commit", "expected-hash"]);
        if (op == "add") allowed.UnionWith(["item", "quantity", "ammo", "rounds", "items"]);
        if (op == "preset-apply") allowed.Add("preset");
        if (op == "preset-export") allowed.Add("output");
        foreach (var key in o.Keys) if (!allowed.Contains(key)) throw new ArgumentException(key);
        string Required(string key) => o.TryGetValue(key, out var v) && !string.IsNullOrWhiteSpace(v) ? v : throw new ArgumentException(key);
        int Number(string key, int min, int max, int? fallback = null)
        {
            if (!o.ContainsKey(key) && fallback.HasValue) return fallback.Value;
            if (!int.TryParse(Required(key), System.Globalization.NumberStyles.None, System.Globalization.CultureInfo.InvariantCulture, out int n) || n < min || n > max)
                throw new ArgumentException(key);
            return n;
        }
        if (o.ContainsKey("timeout")) Number("timeout", 1, 3600);
        if (o.TryGetValue("lang", out var lang) && lang is not ("ko" or "en" or "ja")) throw new ArgumentException("lang");
        var r = new JsonObject { ["version"] = 1, ["operation"] = op };
        if (op == "list") r["directory"] = Path.GetFullPath(Required("directory"));
        else if (op != "catalog")
        {
            r["savePath"] = Path.GetFullPath(Required("save"));
            var flavor = Required("flavor");
            if (flavor is not ("Demo" or "Main")) throw new ArgumentException("flavor");
            r["flavor"] = flavor;
            r["slot"] = Number("slot", 1, flavor == "Demo" ? 1 : 3);
        }
        if (editing)
        {
            r["commit"] = o.ContainsKey("commit");
            if (o.TryGetValue("expected-hash", out var hash)) r["expectedHash"] = hash;
        }
        if (op == "add")
        {
            if (o.TryGetValue("items", out var items))
            {
                if (o.Keys.Any(k => k is "item" or "quantity" or "ammo" or "rounds")) throw new ArgumentException("items");
                r["items"] = JsonNode.Parse(File.ReadAllText(items)) as JsonArray ?? throw new ArgumentException("items");
            }
            else
            {
                var item = new JsonObject { ["itemId"] = Number("item", 1, int.MaxValue), ["quantity"] = Number("quantity", 1, 1000000, 1) };
                if (o.ContainsKey("ammo"))
                {
                    item["loadedAmmoItemId"] = Number("ammo", 1, int.MaxValue);
                    item["loadedAmmoCount"] = Number("rounds", 0, 1000000, 0);
                }
                else if (o.ContainsKey("rounds")) throw new ArgumentException("ammo");
                r["items"] = new JsonArray(item);
            }
        }
        if (op == "preset-apply") r["preset"] = JsonNode.Parse(File.ReadAllText(Required("preset"))) as JsonObject ?? throw new ArgumentException("preset");
        return r;
    }

    public static string FindProject(string? configured)
    {
        if (configured is not null)
        {
            var path = Path.GetFullPath(configured);
            return File.Exists(path) && Path.GetFileName(path).Equals("TunaSweeper.uproject", StringComparison.OrdinalIgnoreCase)
                ? path : throw new FileNotFoundException("project", path);
        }
        foreach (var start in new[] { Environment.CurrentDirectory, AppContext.BaseDirectory })
            for (var dir = new DirectoryInfo(start); dir is not null; dir = dir.Parent)
                foreach (var relative in new[] { "TunaSweeper.uproject", "TunaSweeper/TunaSweeper.uproject" })
                    if (File.Exists(Path.Combine(dir.FullName, relative))) return Path.Combine(dir.FullName, relative);
        throw new FileNotFoundException("project");
    }
}
