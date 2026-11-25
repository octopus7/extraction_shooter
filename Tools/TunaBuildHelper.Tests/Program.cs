using System.IO;
using System.Windows;
using System.Windows.Controls;
using TunaBuildHelper;

internal static class Program
{
    [STAThread]
    private static int Main()
    {
        void Check(bool condition, string message) { if (!condition) throw new Exception(message); }
        Check(ItemGrantCatalog.Featured.Count == 41, "Featured item count");
        Check(ItemGrantCatalog.Featured[0].Id == 6019, "Rocket first");
        Check(ItemGrantCatalog.Items.All(i => !i.Name.StartsWith("item.")), "Resolved item names");
        var window = new ItemGrantWindow(new(123, DateTime.Now, "synthetic test target"));
        var id = (TextBox)window.FindName("ItemIdBox");
        var quantity = (TextBox)window.FindName("QuantityBox");
        var list = (ListBox)window.FindName("FeaturedList");
        Check(id.Text == "6019" && quantity.Text == "1", "Defaults");
        Check(Grid.GetColumn((UIElement)list.Parent) == 2, "List in right column");
        Check(ScrollViewer.GetVerticalScrollBarVisibility(list) == ScrollBarVisibility.Auto, "Vertical scrolling");
        list.SelectedItem = ItemGrantCatalog.Featured.First(i => i.Id == 6033);
        Check(id.Text == "6033" && quantity.Text == "1", "Selection fills ID and retains quantity");
        Check(!((Button)window.FindName("GrantButton")).IsEnabled, "No grant without live context");
        Check(window.Title == ItemGrantCatalog.Text("ui.item_grant.title"), "Localized window title");
        window.Close();
        var kills = new KillCountWindow(new(123, DateTime.Now, "synthetic test target"));
        Check(kills.Title == ItemGrantCatalog.Text("ui.helper_kills.title"), "Kill count window title");
        Check(!((Button)kills.FindName("SaveButton")).IsEnabled, "Cannot save before reading the kill count");
        Check(((Button)kills.FindName("CloseButton")).IsCancel, "Close dismisses without saving");
        Check(KillCountWindow.TryParseCount("0", out _) && KillCountWindow.TryParseCount("9223372036854775807", out _), "Valid kill count bounds");
        foreach (string invalid in new[] { "", "-1", "1.5", "9223372036854775808" })
            Check(!KillCountWindow.TryParseCount(invalid, out _), "Invalid kill count: " + invalid);
        kills.Close();
        Console.WriteLine("Item grant helper tests passed (catalog, layout, selection, defaults, context guard, localization).");
        CheckSteamFullPackage();
        CheckDisplayPresets();
        CheckSteamButtonLayout();
        return 0;
    }

    private static void CheckDisplayPresets()
    {
        var directory = Directory.CreateTempSubdirectory();
        string path = Path.Combine(directory.FullName, "GameUserSettings.ini");
        try
        {
            PackageDisplaySettings.Apply(path, DisplayPreset.FullscreenUhd);
            foreach (string key in new[] { "Version=5", "GraphicsSettingsSchemaVersion=1", "ResolutionSizeX=3840",
                "LastUserConfirmedResolutionSizeY=2160", "FullscreenMode=0", "PreferredDLSSMode=Performance" })
                if (!File.ReadAllLines(path).Contains(key)) throw new Exception(key);
            File.AppendAllText(path, "[TunaSweeper.GraphicsSettings]\nDLSSMode=3\n[Other]\nFullscreenMode=99\n");
            PackageDisplaySettings.Apply(path, DisplayPreset.WindowedHd);
            var hdSettings = File.ReadAllLines(path);
            foreach (string key in new[] { "ResolutionSizeX=1280", "ResolutionSizeY=720",
                "LastUserConfirmedResolutionSizeX=1280", "LastUserConfirmedResolutionSizeY=720",
                "FullscreenMode=2", "LastConfirmedFullscreenMode=2", "PreferredDLSSMode=Performance", "DLSSMode=3" })
                if (!hdSettings.Contains(key)) throw new Exception(key);
            PackageDisplaySettings.Apply(path, DisplayPreset.FullscreenFhd);
            PackageDisplaySettings.Apply(path, DisplayPreset.WindowedFhd);
            foreach (string key in new[] { "ResolutionSizeX=1920", "ResolutionSizeY=1080", "LastConfirmedFullscreenMode=2",
                "FullscreenMode=2", "PreferredDLSSMode=Off", "DLSSMode=0", "FullscreenMode=99" })
                if (File.ReadAllLines(path).Count(line => line == key) != 1) throw new Exception(key);
        }
        finally { directory.Delete(true); }
    }

    private static void CheckSteamButtonLayout()
    {
        var window = new MainWindow();
        try
        {
            static TabItem? FindTab(DependencyObject element)
            {
                for (DependencyObject? parent = element; parent is not null; parent = LogicalTreeHelper.GetParent(parent))
                    if (parent is TabItem tab)
                        return tab;
                return null;
            }

            var packagingTab = (TabItem?)window.FindName("PackagingTab")
                ?? throw new Exception("Expected a Packaging tab");
            var mainTab = (TabItem)window.FindName("MainTab");
            if (!Equals(packagingTab.Header, "Packaging"))
                throw new Exception("Expected the English Packaging tab title");
            foreach (string controlName in new[] { "BuildSteamFullButton", "BuildStoveFullButton", "RunSteamFullButton",
                "SteamAppIdFileName", "SteamAppIdIndicator",
                "SteamAppIdActionButton", "OpenSteamAchievementCommandsButton", "OpenSteamConsoleButton", "OpenSteamUploadScriptsFolderButton", "KillPackagingButton" })
            {
                if (window.FindName(controlName) is not DependencyObject control || FindTab(control) != packagingTab)
                    throw new Exception($"{controlName} must be in the Packaging tab");
            }
            var killBuild = (Button)window.FindName("KillBuildButton");
            var killPackaging = (Button)window.FindName("KillPackagingButton");
            if (FindTab(killBuild) != mainTab || !Equals(killBuild.Content, "빌드 강제 종료") ||
                !Equals(killPackaging.Content, "패키징 강제 종료"))
                throw new Exception("Expected separate build and packaging cancellation buttons");

            var launch = (Button)window.FindName("RunSteamFullButton");
            var build = (Button)window.FindName("BuildSteamFullButton");
            var available = new Size(400, double.PositiveInfinity);
            launch.Measure(available);
            build.Measure(available);
            static double ButtonHeight(Button button) =>
                button.DesiredSize.Height - button.Margin.Top - button.Margin.Bottom;
            if (ButtonHeight(launch) != 1.5 * ButtonHeight(build))
                throw new Exception("Expected Steam launch button to measure 1.5 times the build button height");
            var presets = (ComboBox)window.FindName("DisplayPresetBox");
            if (FindTab(presets) != packagingTab || !presets.Items.OfType<ComboBoxItem>().Any(item =>
                Equals(item.Tag, nameof(DisplayPreset.WindowedHd)) && item.Content?.ToString()?.Contains("1280×720") == true))
                throw new Exception("Expected the 720p windowed preset in the Packaging dropdown");
            var uploadFolder = (Button)window.FindName("OpenSteamUploadScriptsFolderButton");
            var upload = (Button)window.FindName("RunSteamUploadButton");
            if (upload.Parent != uploadFolder.Parent || Grid.GetColumn(upload) <= Grid.GetColumn(uploadFolder) ||
                !Equals(upload.Content, ItemGrantCatalog.Text("ui.helper_steam.run_upload")))
                throw new Exception("Expected the upload action beside its folder button");
            if (!Equals(uploadFolder.Content, ItemGrantCatalog.Text("ui.helper_steam.upload_scripts_folder")))
                throw new Exception("Upload scripts folder button must use localized text");
            var launcherFolder = (Button?)window.FindName("OpenSteamLauncherFolderButton")
                ?? throw new Exception("Expected a button to open the Steam launcher folder");
            var appIdFileName = (TextBlock)window.FindName("SteamAppIdFileName");
            var appIdFolder = (Button)window.FindName("OpenSteamAppIdFolderButton");
            var indicator = (TextBlock)window.FindName("SteamAppIdIndicator");
            var action = (Button)window.FindName("SteamAppIdActionButton");
            var indicatorArea = (Border)indicator.Parent;
            var trailerName = (TextBlock)window.FindName("TrailerModeFileName");
            var trailerIndicator = (TextBlock)window.FindName("TrailerModeIndicator");
            var trailerAction = (Button)window.FindName("TrailerModeActionButton");
            if (FindTab(trailerAction) != packagingTab || trailerName.Text != "trailer_mode.txt" ||
                trailerName.Parent != appIdFileName.Parent || Grid.GetRow(trailerName) <= Grid.GetRow(appIdFileName) ||
                Grid.GetRow(trailerName) != Grid.GetRow((Border)trailerIndicator.Parent) ||
                Grid.GetRow(trailerName) != Grid.GetRow(trailerAction) ||
                !Equals(trailerAction.Content, ItemGrantCatalog.Text(File.Exists(trailerIndicator.ToolTip as string)
                    ? "ui.helper_steam.app_id_delete" : "ui.helper_steam.app_id_create")))
                throw new Exception("Expected the trailer marker state and action below Steam App ID");
            var folderTab = (TabItem?)window.FindName("FolderTab")
                ?? throw new Exception("Expected a Folder tab");
            if (FindTab(launcherFolder) != folderTab || FindTab(appIdFolder) != folderTab ||
                !Equals(launcherFolder.Content, "Steam 빌드 런처 폴더 열기") ||
                !Equals(appIdFolder.Content, "steam_appid.txt 폴더 열기"))
                throw new Exception("The Folder tab must identify the Steam launcher and App ID folder destinations");
            var appIdRow = new UIElement[] { appIdFileName, indicatorArea, action };
            if (appIdFileName.Text != "steam_appid.txt" ||
                appIdRow.Any(element => Grid.GetRow(element) <= Grid.GetRow(launch)) ||
                !appIdRow.Select(Grid.GetColumn).SequenceEqual(new[] { 0, 1, 2 }))
                throw new Exception("The second row must contain the filename, indicator, and action");
            string expectedAction = indicator.Text == "Y" ? "삭제" : "생성";
            if (!Equals(action.Content, expectedAction))
                throw new Exception("The visible App ID action must be labeled Create or Delete");
            if (indicator.ToolTip is string filePath && Path.IsPathFullyQualified(filePath))
            {
                string appIdDirectory = Path.GetDirectoryName(filePath)!;
                string launcherDirectory = Path.GetFullPath(Path.Combine(appIdDirectory, "..", "..", ".."));
                if (!Equals(appIdFolder.ToolTip, appIdDirectory) ||
                    !Equals(launcherFolder.ToolTip, launcherDirectory))
                    throw new Exception("The folder buttons must target the respective launcher and App ID directories");
            }
            var commands = (Button?)window.FindName("OpenSteamAchievementCommandsButton")
                ?? throw new Exception("Expected an achievement commands file button");
            var console = (Button?)window.FindName("OpenSteamConsoleButton")
                ?? throw new Exception("Expected a Steam console button");
            if (Equals(commands.Parent, console.Parent) is false ||
                Grid.GetColumn(commands) >= Grid.GetColumn(console) ||
                !Equals(console.ToolTip, "steam://open/console"))
                throw new Exception("Expected separate actions for the commands file and the Steam console");
            if (launcherFolder.ToolTip is string launcherPath && Path.IsPathFullyQualified(launcherPath))
            {
                string projectDirectory = Path.GetFullPath(Path.Combine(launcherPath, "..", "..", "..", ".."));
                if (!Equals(uploadFolder.ToolTip, Path.Combine(projectDirectory, "Builds")))
                    throw new Exception("Upload scripts folder must target Builds, not the packaged Windows folder");
                string uploadPath = Path.Combine(projectDirectory, "Builds", "UploadSteamFull.bat");
                if (!Equals(upload.ToolTip, uploadPath) || upload.IsEnabled != File.Exists(uploadPath))
                    throw new Exception("Upload action must target the Steam Full batch");
                string guide = Path.Combine(projectDirectory, "External", "MainPayload", "Tools", "Steam",
                    "Achievements", "5137900-achievements-reset-commands.ko.txt");
                if (!Equals(commands.ToolTip, guide))
                    throw new Exception("The commands button must use the local Full payload document");
            }
        }
        finally
        {
            window.Close();
        }
        Console.WriteLine("Steam launcher and App ID button layout test passed.");
    }

    private static void CheckSteamFullPackage()
    {
        void Check(bool condition, string message) { if (!condition) throw new Exception(message); }
        using var fixture = new SteamPackageFixture();
        string launcher = Path.Combine(fixture.WindowsDirectory, "TunaSweeper.exe");
        string rootAppId = Path.Combine(fixture.WindowsDirectory, "steam_appid.txt");
        string binaries = Path.Combine(fixture.WindowsDirectory, "TunaSweeper", "Binaries", "Win64");
        string innerAppId = Path.Combine(binaries, "steam_appid.txt");
        Directory.CreateDirectory(binaries);
        File.WriteAllText(launcher, "");
        File.WriteAllText(Path.Combine(binaries, "TunaSweeper-Win64-Shipping.exe"), "");
        File.WriteAllText(rootAppId, "42");
        Directory.CreateDirectory(Path.Combine(fixture.ProjectDirectory, "Config"));
        File.WriteAllText(Path.Combine(fixture.ProjectDirectory, "Config", "DefaultGame.ini"), "SteamFullGameAppId=1234567\n");

        var package = new SteamFullPackage(fixture.ProjectDirectory);
        Check(package.LauncherPath == launcher, "Launches the outer package executable");
        Check(package.AppIdDirectory == binaries, "The folder button targets the inner executable directory");
        Check(!package.HasAppIdFile, "Root App ID does not count");
        var window = new MainWindow();
        try
        {
            typeof(MainWindow).GetField("steamFullPackage",
                System.Reflection.BindingFlags.Instance | System.Reflection.BindingFlags.NonPublic)!.SetValue(window, package);
            ((Button)window.FindName("RunSteamFullButton")).RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
            Check(((TextBlock)window.FindName("StatusTextBlock")).Text == (string)window.FindResource("SteamAppIdRequired"),
                "Clicking Run without the inner App ID must stop and explain how to create it");
        }
        finally { window.Close(); }
        package.CreateAppIdFile();
        Check(package.HasAppIdFile && File.ReadAllText(innerAppId).Trim() == "1234567", "Creates the App ID beside the internal executable using the Full ID");
        File.WriteAllText(innerAppId, "7654321");
        try
        {
            package.CreateAppIdFile();
            throw new Exception("Expected creation to reject an existing App ID file");
        }
        catch (IOException)
        {
            Check(File.ReadAllText(innerAppId) == "7654321", "Creation leaves the existing App ID file intact");
        }
        package.DeleteAppIdFile();
        string marker = Path.Combine(binaries, "trailer_mode.txt");
        Check(package.TrailerModePath == marker && !File.Exists(marker), "Trailer mode is initially off");
        package.CreateTrailerModeFile();
        Check(new FileInfo(marker).Length == 0 && !package.HasAppIdFile, "Empty trailer marker is independent from App ID");
        package.DeleteTrailerModeFile();
        Check(!File.Exists(marker), "Deleting the trailer marker disables trailer mode");
        Check(!package.HasAppIdFile && !File.Exists(innerAppId) && File.ReadAllText(rootAppId) == "42", "Removes only the internal App ID");
        Console.WriteLine("Steam Full package tests passed (launcher path, App ID detection, creation, deletion).");
    }

    private sealed class SteamPackageFixture : IDisposable
    {
        private readonly DirectoryInfo root = Directory.CreateTempSubdirectory("TunaHelperSteamTest-");
        public string ProjectDirectory => root.FullName;
        public string WindowsDirectory => Path.Combine(ProjectDirectory, "Builds", "Steam", "Full", "Windows");
        public void Dispose() => root.Delete(recursive: true);
    }
}
