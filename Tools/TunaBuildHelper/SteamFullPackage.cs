using System.Globalization;
using System.IO;
using System.Text;

namespace TunaBuildHelper;

internal sealed class SteamFullPackage(string projectDirectory)
{
    internal const string AppIdFileName = "steam_appid.txt";
    private string GameExecutablePath => Path.Combine(AppIdDirectory, "TunaSweeper-Win64-Shipping.exe");

    internal string LauncherDirectory => Path.Combine(projectDirectory, "Builds", "Steam", "Full", "Windows");
    internal string LauncherPath => Path.Combine(LauncherDirectory, "TunaSweeper.exe");
    internal string AppIdDirectory => Path.Combine(LauncherDirectory, "TunaSweeper", "Binaries", "Win64");
    internal string AppIdPath => Path.Combine(AppIdDirectory, AppIdFileName);
    internal bool HasAppIdFile => File.Exists(AppIdPath);
    internal bool CanCreateAppIdFile => File.Exists(GameExecutablePath);

    internal void CreateAppIdFile()
    {
        if (!CanCreateAppIdFile)
            throw new FileNotFoundException(null, GameExecutablePath);

        const string key = "SteamFullGameAppId";
        string? configuredAppId = File.ReadLines(Path.Combine(projectDirectory, "Config", "DefaultGame.ini"))
            .Select(line => line.Split('=', 2))
            .FirstOrDefault(parts => parts.Length == 2 && parts[0].Trim() == key)?[1].Trim();
        if (!uint.TryParse(configuredAppId, NumberStyles.None, CultureInfo.InvariantCulture, out uint appId) || appId == 0)
            throw new InvalidDataException(key);

        string temporaryPath = Path.Combine(AppIdDirectory, Path.GetRandomFileName());
        var stream = new FileStream(temporaryPath, FileMode.CreateNew, FileAccess.Write);
        try
        {
            using (stream)
                stream.Write(Encoding.ASCII.GetBytes(appId.ToString(CultureInfo.InvariantCulture) + Environment.NewLine));
            File.Move(temporaryPath, AppIdPath);
        }
        finally
        {
            File.Delete(temporaryPath);
        }
    }

    internal void DeleteAppIdFile() => File.Delete(AppIdPath);
}
