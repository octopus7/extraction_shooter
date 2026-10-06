using System.Diagnostics;
using System.Text;
using System.Text.Json.Nodes;

namespace TunaSweeper.SaveTool;
public static class CommandletClient
{
    public static string FindEditor(string? configured)
    {
        var candidates = new[] { configured, Environment.GetEnvironmentVariable("TUNASWEEPER_EDITOR"),
            Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles), "Epic Games/UE_5.7/Engine/Binaries/Win64/UnrealEditor-Cmd.exe") };
        foreach (var path in candidates)
            if (!string.IsNullOrWhiteSpace(path))
            {
                if (File.Exists(path)) return Path.GetFullPath(path);
                if (path == configured) throw new FileNotFoundException("editor", path);
            }
        throw new FileNotFoundException("editor");
    }

    public static async Task<JsonObject> Run(string editor, string project, JsonObject request, TimeSpan timeout, CancellationToken cancellation)
    {
        string dir = Path.Combine(Path.GetDirectoryName(project)!, "Saved/SaveTool/Runs", Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(dir);
        string input = Path.Combine(dir, "request.json"), output = Path.Combine(dir, "response.json");
        await File.WriteAllTextAsync(input, request.ToJsonString(), new UTF8Encoding(false), cancellation);
        var start = new ProcessStartInfo(editor) { UseShellExecute = false, CreateNoWindow = true, RedirectStandardOutput = true, RedirectStandardError = true };
        foreach (var arg in new[] { project, "-run=TunaSweeperEditor.TunaSweeperSaveTool", $"-Request={input}", $"-Response={output}", "-unattended", "-nop4", "-nosplash", "-nullrhi", $"-abslog={Path.Combine(dir, "engine.log")}" }) start.ArgumentList.Add(arg);
        using var process = Process.Start(start) ?? throw new IOException("process_start");
        // Drain both pipes concurrently; never mix Unreal logs into JSON stdout.
        using var stdout = File.Create(Path.Combine(dir, "stdout.log"));
        using var stderr = File.Create(Path.Combine(dir, "stderr.log"));
        var drain = Task.WhenAll(process.StandardOutput.BaseStream.CopyToAsync(stdout), process.StandardError.BaseStream.CopyToAsync(stderr));
        using var limit = CancellationTokenSource.CreateLinkedTokenSource(cancellation);
        limit.CancelAfter(timeout);
        try { await process.WaitForExitAsync(limit.Token); }
        catch (OperationCanceledException)
        {
            if (!process.HasExited) process.Kill(entireProcessTree: true);
            await process.WaitForExitAsync(CancellationToken.None);
            await drain;
            throw new IOException($"process_interrupted: {dir}");
        }
        await drain;
        if (!File.Exists(output)) throw new IOException($"response_missing: {process.ExitCode}: {dir}");
        var response = JsonNode.Parse(await File.ReadAllTextAsync(output, cancellation)) as JsonObject ?? throw new IOException($"response_invalid: {dir}");
        if (response["version"]?.GetValue<int>() != 1 || response["ok"] is not JsonValue ok || !ok.TryGetValue<bool>(out bool success)) throw new IOException($"response_invalid: {dir}");
        if (process.ExitCode != 0 && success) throw new IOException($"exit_mismatch: {process.ExitCode}: {dir}");
        response["runDirectory"] = dir;
        return response;
    }
}
