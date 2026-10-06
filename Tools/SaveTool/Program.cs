using System.Text;
using System.Text.Json;
using System.Text.Json.Nodes;
using TunaSweeper.SaveTool;

Console.OutputEncoding = new UTF8Encoding(false);
var jsonOptions = new JsonSerializerOptions { WriteIndented = true };
bool json = args.Contains("--json", StringComparer.Ordinal);
var strings = new Localization(null, "ko");
try
{
    var options = Cli.Parse(args.Length == 0 ? ["help"] : args);
    var project = Cli.FindProject(options.GetValueOrDefault("project"));
    strings = new Localization(project, options.GetValueOrDefault("lang", "ko"));
    if (args.Length == 0 || args[0] is "help" or "--help" || options.ContainsKey("help"))
    {
        Console.WriteLine(strings["tool.save.help"].Replace(" | ", Environment.NewLine));
        return 0;
    }
    var request = Cli.BuildRequest(args);
    if (options.TryGetValue("output", out var exportPath) && File.Exists(Path.GetFullPath(exportPath))) throw new IOException($"output_exists: {exportPath}");
    var editor = CommandletClient.FindEditor(options.GetValueOrDefault("editor"));
    using var cancel = new CancellationTokenSource();
    Console.CancelKeyPress += (_, e) => { e.Cancel = true; cancel.Cancel(); };
    if (!json) Console.Error.WriteLine(strings["tool.save.running"]);
    var response = await CommandletClient.Run(editor, project, request, TimeSpan.FromSeconds(int.Parse(options.GetValueOrDefault("timeout", "180"))), cancel.Token);
    bool ok = response["ok"]!.GetValue<bool>();
    if (ok && exportPath is not null)
    {
        using var output = new StreamWriter(new FileStream(Path.GetFullPath(exportPath), FileMode.CreateNew, FileAccess.Write), new UTF8Encoding(false));
        await output.WriteAsync(response["preset"]!.ToJsonString(jsonOptions));
    }
    if (!json)
    {
        if (response["items"] is JsonArray items && args[0] == "catalog")
        {
            Console.WriteLine(strings["tool.save.catalog_header"]);
            foreach (var item in items)
                Console.WriteLine($"{item!["itemId"],6}  {item["names"]?[options.GetValueOrDefault("lang", "ko")] ?? item["nameStringKey"]}  {item["category"]}  {item["maxStack"]}");
            return ok ? 0 : 1;
        }
        Console.Write(HumanOutput.Render(response, strings, options.GetValueOrDefault("lang", "ko")));
        if (ok && exportPath is not null) Console.WriteLine($"{strings["tool.save.exportPath"]}: {Path.GetFullPath(exportPath)}");
        return ok ? 0 : 1;
    }
    Console.WriteLine(response.ToJsonString(jsonOptions));
    return ok ? 0 : 1;
}
catch (Exception ex) when (ex is ArgumentException or IOException or JsonException or InvalidOperationException or System.ComponentModel.Win32Exception or UnauthorizedAccessException)
{
    var error = new JsonObject { ["version"] = 1, ["ok"] = false, ["code"] = "cli_error", ["detail"] = ex.Message };
    if (json) Console.WriteLine(error.ToJsonString(jsonOptions));
    else Console.Error.WriteLine($"{strings["tool.save.failure"]}\n{ex.Message}");
    return 2;
}
