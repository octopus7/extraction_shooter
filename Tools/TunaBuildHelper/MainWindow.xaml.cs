using Microsoft.Win32;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Threading;

namespace TunaBuildHelper;

public partial class MainWindow : Window
{
    private const string StartupRegistryValueName = "TunaHelper";
    private const string StartupRegistryKeyPath = @"Software\Microsoft\Windows\CurrentVersion\Run";
    private const string StartMenuShortcutFileName = "Tuna Helper.lnk";
    private const string FfmpegFolderPath = @"D:\github\ffmpeg";
    private readonly DispatcherTimer movementCheatStateTimer = new() { Interval = TimeSpan.FromSeconds(1) };
    private bool movementCheatStateReadInProgress;
    private int movementCheatStateRevision;
    private Task<IReadOnlyList<MovementCheatTarget>>? movementCheatSearchTask;
    private int movementCheatSearchRevision;
    private bool isClosed;
    private CancellationTokenSource? movementCheatAutoCancellation;
    private CancellationTokenSource? movementCheatAutoTargetCancellation;
    private CancellationTokenSource? movementCheatManualCancellation;
    private CancellationTokenSource? tunaFillCancellation;

    // Only the explicit button path can publish a tuna request. AUTO never calls this action.
    private async void FillTuna_Click(object sender, RoutedEventArgs e)
    {
        if (isClosed || tunaFillCancellation is not null) return;
        if (MovementCheatTargets.SelectedItem is not MovementCheatTarget target)
        {
            StatusTextBlock.Text = (string)FindResource("MovementCheatSelectTarget");
            return;
        }
        using var cancellation = new CancellationTokenSource();
        tunaFillCancellation = cancellation;
        TunaFillButton.IsEnabled = false;
        try
        {
            StatusTextBlock.Text = (string)FindResource("TunaReading");
            var expected = await Task.Run(() => GateProgressProcess.ReadTuna(target), cancellation.Token);
            cancellation.Token.ThrowIfCancellationRequested();
            if (isClosed || !Equals(MovementCheatTargets.SelectedItem, target)) return;
            if (MessageBox.Show(this, string.Format((string)FindResource("TunaConfirm"), target.DisplayName, expected.Map, expected.Slot),
                (string)FindResource("TunaTitle"), MessageBoxButton.YesNo, MessageBoxImage.Warning, MessageBoxResult.No) != MessageBoxResult.Yes)
            {
                StatusTextBlock.Text = (string)FindResource("TunaCanceled");
                return;
            }
            cancellation.Token.ThrowIfCancellationRequested();
            if (isClosed || !Equals(MovementCheatTargets.SelectedItem, target)) return;
            StatusTextBlock.Text = (string)FindResource("TunaWorking");
            uint result = await Task.Run(() => GateProgressProcess.FillTuna(target, expected, cancellation.Token), cancellation.Token);
            if (!isClosed && !cancellation.IsCancellationRequested)
                StatusTextBlock.Text = (string)FindResource(result switch
                {
                    14 => "TunaSuccess", 3 => "TunaDisabled", 6 => "TunaTimeout",
                    7 or 8 or 9 => "TunaStale", _ => "TunaFailure"
                });
        }
        catch (OperationCanceledException)
        {
            if (!isClosed) StatusTextBlock.Text = (string)FindResource("TunaCanceled");
        }
        catch (Exception ex)
        {
            if (!isClosed && !cancellation.IsCancellationRequested)
                StatusTextBlock.Text = (string)FindResource(ex.Message switch
                {
                    "TunaDisabled" => "TunaDisabled", "TunaUnsupported" => "TunaUnsupported",
                    "TunaUnavailable" => "TunaUnavailable", "GateStale" => "TunaStale",
                    "GateBusy" => "TunaBusy", "GateTimeout" => "TunaTimeout", _ => "TunaFailure"
                });
        }
        finally
        {
            if (ReferenceEquals(tunaFillCancellation, cancellation)) tunaFillCancellation = null;
            if (!isClosed) TunaFillButton.IsEnabled = true;
        }
    }

    private void OpenWorldProgress_Click(object sender, RoutedEventArgs e)
    {
        if (MovementCheatTargets.SelectedItem is not MovementCheatTarget target)
        {
            StatusTextBlock.Text = (string)FindResource("MovementCheatSelectTarget");
            return;
        }
        var popup = new WorldProgressWindow(target) { Owner = this };
        popup.Resources.Add(typeof(System.Windows.Controls.Button), FindResource(typeof(System.Windows.Controls.Button)));
        popup.Show();
    }

    public MainWindow()
    {
        InitializeComponent();
        RunAtStartupMenuItem.IsChecked = IsRunAtStartupEnabled();
        RegisterStartMenuMenuItem.IsChecked = IsStartMenuShortcutRegistered();
        movementCheatStateTimer.Tick += async (_, _) => await RefreshMovementCheatStateAsync();
        IsVisibleChanged += (_, _) => UpdateMovementCheatStatePolling();
        StateChanged += (_, _) => UpdateMovementCheatStatePolling();
    }

    private void BuildAndRunButton_Click(object sender, RoutedEventArgs e)
    {
        RunBatchScript("BuildAndRunTunaSweeper.bat", closeOnSuccess: true);
    }

    private async void RefreshMovementCheatTargets_Click(object sender, RoutedEventArgs e)
    {
        await RefreshMovementCheatTargetsAsync();
    }

    private async void HelperTabs_SelectionChanged(object sender, System.Windows.Controls.SelectionChangedEventArgs e)
    {
        if (!ReferenceEquals(e.Source, sender)) return;
        UpdateMovementCheatStatePolling();
        if (CheatTab?.IsSelected == true)
        {
            await RefreshMovementCheatTargetsAsync();
        }
        else
        {
            movementCheatSearchRevision++;
            if (movementCheatSearchTask is not null)
                StatusTextBlock.Text = (string)FindResource("ReadyStatus");
        }
    }

    private async void MovementCheatTargets_SelectionChanged(object sender, System.Windows.Controls.SelectionChangedEventArgs e)
    {
        if (MovementCheatStateText is null) return;
        movementCheatAutoTargetCancellation?.Cancel();
        movementCheatManualCancellation?.Cancel();
        tunaFillCancellation?.Cancel();
        movementCheatStateRevision++;
        MovementCheatStateText.Text = (string)FindResource(MovementCheatTargets.SelectedItem is MovementCheatTarget
            ? "MovementCheatStateReading" : "MovementCheatStateSelect");
        await RefreshMovementCheatStateAsync();
    }

    private void UpdateMovementCheatStatePolling()
    {
        movementCheatStateTimer.Stop();
        if (IsMovementCheatStateVisible)
        {
            movementCheatStateTimer.Start();
            _ = RefreshMovementCheatStateAsync();
        }
    }

    private bool IsMovementCheatStateVisible => IsVisible && WindowState != WindowState.Minimized && CheatTab?.IsSelected == true;

    private async Task RefreshMovementCheatStateAsync()
    {
        if (isClosed || !IsMovementCheatStateVisible || movementCheatStateReadInProgress || movementCheatSearchTask is not null) return;
        if (MovementCheatTargets.SelectedItem is not MovementCheatTarget target)
        {
            MovementCheatStateText.Text = (string)FindResource("MovementCheatStateSelect");
            return;
        }
        movementCheatStateReadInProgress = true;
        int revision = movementCheatStateRevision;
        string resourceKey;
        try
        {
            bool enabled = await Task.Run(() => MovementCheatProcess.IsEnabled(target));
            resourceKey = enabled ? "MovementCheatStateEnabled" : "MovementCheatStateDisabled";
        }
        catch (MovementCheatTargetExitedException)
        {
            resourceKey = "MovementCheatStateExited";
        }
        catch (Exception exception) when (exception is System.ComponentModel.Win32Exception or InvalidOperationException or ArgumentException)
        {
            resourceKey = "MovementCheatStateUnknown";
        }
        finally
        {
            movementCheatStateReadInProgress = false;
        }
        if (revision != movementCheatStateRevision)
        {
            await RefreshMovementCheatStateAsync();
            return;
        }
        if (IsMovementCheatStateVisible && Equals(MovementCheatTargets.SelectedItem, target))
            MovementCheatStateText.Text = (string)FindResource(resourceKey);
    }

    protected override void OnClosed(EventArgs e)
    {
        isClosed = true;
        movementCheatSearchRevision++;
        movementCheatStateRevision++;
        movementCheatStateTimer.Stop();
        movementCheatAutoCancellation?.Cancel();
        movementCheatAutoTargetCancellation?.Cancel();
        movementCheatManualCancellation?.Cancel();
        tunaFillCancellation?.Cancel();
        base.OnClosed(e);
    }

    private async Task<IReadOnlyList<MovementCheatTarget>> GetMovementCheatTargetsAsync()
    {
        var search = movementCheatSearchTask ??= Task.Run(MovementCheatProcess.FindTargets);
        try
        {
            return await search;
        }
        finally
        {
            if (ReferenceEquals(movementCheatSearchTask, search)) movementCheatSearchTask = null;
        }
    }

    private void ApplyMovementCheatTargets(IReadOnlyList<MovementCheatTarget> targets)
    {
        MovementCheatTarget? previous = MovementCheatTargets.SelectedItem as MovementCheatTarget;
        var current = MovementCheatTargets.ItemsSource as IReadOnlyList<MovementCheatTarget>;
        if (current is null || !current.SequenceEqual(targets))
        {
            MovementCheatTargets.ItemsSource = targets;
            MovementCheatTargets.SelectedItem = targets.FirstOrDefault(target => target == previous);
        }
        if (MovementCheatTargets.SelectedItem is null && targets.Count == 1) MovementCheatTargets.SelectedIndex = 0;
    }

    private void ToggleMovementCheatAuto_Click(object sender, RoutedEventArgs e)
    {
        if (isClosed) return;
        if (movementCheatAutoCancellation is not null)
        {
            movementCheatAutoCancellation.Cancel();
            movementCheatAutoTargetCancellation?.Cancel();
            movementCheatAutoCancellation = null;
            MovementCheatAutoButton.Content = FindResource("MovementCheatAutoOff");
            MovementCheatAutoStatus.Text = (string)FindResource("MovementCheatAutoIdle");
            return;
        }
        var cancellation = new CancellationTokenSource();
        movementCheatAutoCancellation = cancellation;
        MovementCheatAutoButton.Content = FindResource("MovementCheatAutoOn");
        MovementCheatAutoStatus.Text = (string)FindResource("MovementCheatAutoWaiting");
        _ = MonitorMovementCheatAutoAsync(cancellation);
    }

    private async Task MonitorMovementCheatAutoAsync(CancellationTokenSource cancellation)
    {
        CancellationToken token = cancellation.Token;
        try
        {
            while (!token.IsCancellationRequested)
            {
                string statusKey;
                try
                {
                    var targets = await GetMovementCheatTargetsAsync();
                    token.ThrowIfCancellationRequested();
                    if (isClosed) return;
                    ApplyMovementCheatTargets(targets);
                    if (MovementCheatTargets.SelectedItem is MovementCheatTarget target)
                    {
                        using var targetCancellation = CancellationTokenSource.CreateLinkedTokenSource(token);
                        movementCheatAutoTargetCancellation = targetCancellation;
                        try
                        {
                            await Task.Run(() =>
                            {
                                targetCancellation.Token.ThrowIfCancellationRequested();
                                if (!MovementCheatProcess.IsEnabled(target))
                                    MovementCheatProcess.SetEnabled(target, true, targetCancellation.Token);
                            }, targetCancellation.Token);
                        }
                        finally
                        {
                            if (ReferenceEquals(movementCheatAutoTargetCancellation, targetCancellation)) movementCheatAutoTargetCancellation = null;
                        }
                        statusKey = "MovementCheatAutoActive";
                        movementCheatStateRevision++;
                        await RefreshMovementCheatStateAsync();
                    }
                    else
                    {
                        statusKey = targets.Count == 0 ? "MovementCheatAutoWaiting" : "MovementCheatAutoSelect";
                    }
                }
                catch (OperationCanceledException) when (!token.IsCancellationRequested)
                {
                    statusKey = "MovementCheatAutoSelect";
                }
                catch (Exception exception) when (exception is System.ComponentModel.Win32Exception or InvalidOperationException or ArgumentException or NotSupportedException)
                {
                    statusKey = "MovementCheatAutoRetry";
                }
                token.ThrowIfCancellationRequested();
                if (isClosed) return;
                MovementCheatAutoStatus.Text = (string)FindResource(statusKey);
                await Task.Delay(TimeSpan.FromSeconds(1), token);
            }
        }
        catch (OperationCanceledException) when (token.IsCancellationRequested)
        {
        }
        catch (Exception)
        {
            if (!isClosed && ReferenceEquals(movementCheatAutoCancellation, cancellation))
                MovementCheatAutoStatus.Text = (string)FindResource("MovementCheatAutoFailed");
        }
        finally
        {
            if (ReferenceEquals(movementCheatAutoCancellation, cancellation))
            {
                movementCheatAutoCancellation = null;
                if (!isClosed) MovementCheatAutoButton.Content = FindResource("MovementCheatAutoOff");
            }
            cancellation.Dispose();
        }
    }

    private async Task RefreshMovementCheatTargetsAsync()
    {
        if (isClosed || CheatTab?.IsSelected != true) return;
        int revision = ++movementCheatSearchRevision;
        movementCheatStateRevision++;
        MovementCheatRefreshButton.IsEnabled = false;
        StatusTextBlock.Text = (string)FindResource("MovementCheatSearching");
        // Reentry shares an in-flight scan. All process/module access runs off the UI thread.
        try
        {
            var targets = await GetMovementCheatTargetsAsync();
            if (isClosed || revision != movementCheatSearchRevision || CheatTab.IsSelected != true) return;
            ApplyMovementCheatTargets(targets);
            string statusKey = targets.Count switch
            {
                0 => "MovementCheatNoTarget",
                1 => "MovementCheatTargetSelected",
                _ => "MovementCheatSelectTarget"
            };
            StatusTextBlock.Text = (string)FindResource(statusKey);
        }
        catch (Exception exception) when (exception is System.ComponentModel.Win32Exception or InvalidOperationException or ArgumentException or NotSupportedException)
        {
            if (!isClosed && revision == movementCheatSearchRevision && CheatTab.IsSelected == true)
                StatusTextBlock.Text = (string)FindResource("MovementCheatSearchFailed");
        }
        finally
        {
            if (!isClosed && revision == movementCheatSearchRevision)
                MovementCheatRefreshButton.IsEnabled = true;
        }
        await RefreshMovementCheatStateAsync();
    }

    private void EnableMovementCheat_Click(object sender, RoutedEventArgs e) => SetMovementCheat(true);

    private void DisableMovementCheat_Click(object sender, RoutedEventArgs e) => SetMovementCheat(false);

    private async void SetMovementCheat(bool enabled)
    {
        if (isClosed) return;
        if (MovementCheatTargets.SelectedItem is not MovementCheatTarget target)
        {
            StatusTextBlock.Text = (string)FindResource("MovementCheatSelectTarget");
            return;
        }
        movementCheatManualCancellation?.Cancel();
        using var cancellation = new CancellationTokenSource();
        movementCheatManualCancellation = cancellation;
        try
        {
            await Task.Run(() => MovementCheatProcess.SetEnabled(target, enabled, cancellation.Token), cancellation.Token);
            if (!isClosed && !cancellation.IsCancellationRequested)
                StatusTextBlock.Text = (string)FindResource(enabled ? "MovementCheatEnabled" : "MovementCheatDisabled");
        }
        catch (OperationCanceledException) when (cancellation.IsCancellationRequested)
        {
        }
        catch (Exception exception) when (exception is System.ComponentModel.Win32Exception or InvalidOperationException or ArgumentException)
        {
            if (!isClosed && !cancellation.IsCancellationRequested) StatusTextBlock.Text = (string)FindResource("MovementCheatFailed");
        }
        finally
        {
            if (ReferenceEquals(movementCheatManualCancellation, cancellation)) movementCheatManualCancellation = null;
        }
        movementCheatStateRevision++;
        await RefreshMovementCheatStateAsync();
    }

    private void KillBuildButton_Click(object sender, RoutedEventArgs e)
    {
        RunBatchScript("KillTunaSweeperBuild.bat", closeAlways: true, statusResourceKey: "KillBuildStartedStatus");
    }

    private void BuildSteamFullButton_Click(object sender, RoutedEventArgs e)
    {
        RunBatchScript("PackageTunaSweeperSteamFullWin64.bat", statusResourceKey: "PackageBuildStartedStatus");
    }

    private void BuildStoveFullButton_Click(object sender, RoutedEventArgs e)
    {
        RunBatchScript("PackageTunaSweeperStoveFullWin64.bat", statusResourceKey: "PackageBuildStartedStatus");
    }

    private void KillEditorButton_Click(object sender, RoutedEventArgs e)
    {
        RunBatchScript("KillTunaSweeperEditor.bat", closeAlways: true);
    }

    private void OpenUnrealProjectButton_Click(object sender, RoutedEventArgs e)
    {
        string? projectFilePath = FindUnrealProjectFile();
        if (projectFilePath is null)
        {
            ShowProjectNotFoundMessage();
            return;
        }

        OpenPath(projectFilePath, "Unreal 프로젝트를 여는 중입니다.");
    }

    private void OpenProjectFolderButton_Click(object sender, RoutedEventArgs e)
    {
        string? projectFilePath = FindUnrealProjectFile();
        if (projectFilePath is null)
        {
            ShowProjectNotFoundMessage();
            return;
        }

        OpenPath(Path.GetDirectoryName(projectFilePath)!, "Unreal 프로젝트 폴더를 여는 중입니다.");
    }

    private void OpenDownloadsFolderButton_Click(object sender, RoutedEventArgs e)
    {
        string downloadsPath = Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.UserProfile),
            "Downloads");

        if (!Directory.Exists(downloadsPath))
        {
            MessageBox.Show(
                "다운로드 폴더를 찾을 수 없습니다.",
                Title,
                MessageBoxButton.OK,
                MessageBoxImage.Error);
            return;
        }

        OpenPath(downloadsPath, "다운로드 폴더를 여는 중입니다.");
    }

    private void OpenFfmpegFolderButton_Click(object sender, RoutedEventArgs e)
    {
        if (!Directory.Exists(FfmpegFolderPath))
        {
            MessageBox.Show(
                $"FFmpeg 폴더를 찾을 수 없습니다.\n\n{FfmpegFolderPath}",
                Title,
                MessageBoxButton.OK,
                MessageBoxImage.Error);
            return;
        }

        OpenPath(FfmpegFolderPath, "FFmpeg 폴더를 여는 중입니다.");
    }

    private void RunAtStartupMenuItem_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            SetRunAtStartup(RunAtStartupMenuItem.IsChecked);
            StatusTextBlock.Text = RunAtStartupMenuItem.IsChecked
                ? "Windows 시작 시 자동 실행을 등록했습니다."
                : "Windows 시작 시 자동 실행을 해제했습니다.";
        }
        catch (Exception exception)
        {
            RunAtStartupMenuItem.IsChecked = IsRunAtStartupEnabled();
            MessageBox.Show(
                $"자동 실행 설정을 변경하지 못했습니다.\n\n{exception.Message}",
                Title,
                MessageBoxButton.OK,
                MessageBoxImage.Error);
        }
    }

    private void RegisterStartMenuMenuItem_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            SetStartMenuShortcut(RegisterStartMenuMenuItem.IsChecked);
            StatusTextBlock.Text = RegisterStartMenuMenuItem.IsChecked
                ? "시작 메뉴에 Tuna Helper를 등록했습니다."
                : "시작 메뉴에서 Tuna Helper를 제거했습니다.";
        }
        catch (Exception exception)
        {
            RegisterStartMenuMenuItem.IsChecked = IsStartMenuShortcutRegistered();
            MessageBox.Show(
                $"시작 메뉴 등록을 변경하지 못했습니다.\n\n{exception.Message}",
                Title,
                MessageBoxButton.OK,
                MessageBoxImage.Error);
        }
    }

    private void OpenPath(string path, string statusMessage)
    {
        try
        {
            Process.Start(new ProcessStartInfo
            {
                FileName = path,
                UseShellExecute = true
            });
            StatusTextBlock.Text = statusMessage;
        }
        catch (Exception exception)
        {
            MessageBox.Show(
                $"경로를 열지 못했습니다.\n\n{exception.Message}",
                Title,
                MessageBoxButton.OK,
                MessageBoxImage.Error);
        }
    }

    private void RunBatchScript(
        string scriptName,
        bool closeOnSuccess = false,
        bool closeAlways = false,
        string? statusResourceKey = null)
    {
        string? scriptPath = FindBatchScript(scriptName);
        if (scriptPath is null)
        {
            MessageBox.Show(
                $"{scriptName}을(를) 찾을 수 없습니다.",
                Title,
                MessageBoxButton.OK,
                MessageBoxImage.Error);
            return;
        }

        // cmd /s removes the outer quotes; CALL needs one remaining pair around the path.
        string arguments = closeAlways
            ? $"/d /s /c \"call \"{scriptPath}\"\""
            : closeOnSuccess
                ? $"/d /s /c \"call \"{scriptPath}\" & if errorlevel 1 pause\""
                : $"/d /s /k \"\"{scriptPath}\"\"";

        try
        {
            Process.Start(new ProcessStartInfo
            {
                FileName = "cmd.exe",
                Arguments = arguments,
                WorkingDirectory = Path.GetDirectoryName(scriptPath) ?? AppContext.BaseDirectory,
                UseShellExecute = true
            });
            StatusTextBlock.Text = statusResourceKey is not null
                ? (string)FindResource(statusResourceKey)
                : closeAlways
                    ? "Unreal Editor 종료를 요청했습니다."
                    : "프로젝트 빌드 및 실행을 시작했습니다.";
        }
        catch (Exception exception)
        {
            MessageBox.Show(
                $"작업을 시작하지 못했습니다.\n\n{exception.Message}",
                Title,
                MessageBoxButton.OK,
                MessageBoxImage.Error);
        }
    }

    private static string? FindUnrealProjectFile()
    {
        DirectoryInfo? directory = new(AppContext.BaseDirectory);
        while (directory is not null)
        {
            string candidate = Path.Combine(directory.FullName, "TunaSweeper", "TunaSweeper.uproject");
            if (File.Exists(candidate))
            {
                return candidate;
            }

            directory = directory.Parent;
        }

        return null;
    }

    private static string? FindBatchScript(string scriptName)
    {
        string? projectFilePath = FindUnrealProjectFile();
        if (projectFilePath is null)
        {
            return null;
        }

        string scriptPath = Path.Combine(
            Path.GetDirectoryName(projectFilePath)!,
            "BatchScripts",
            scriptName);
        return File.Exists(scriptPath) ? scriptPath : null;
    }

    private static bool IsRunAtStartupEnabled()
    {
        using RegistryKey? key = Registry.CurrentUser.OpenSubKey(StartupRegistryKeyPath, writable: false);
        return key?.GetValue(StartupRegistryValueName) is string value &&
               !string.IsNullOrWhiteSpace(value);
    }

    private static void SetRunAtStartup(bool enabled)
    {
        using RegistryKey? key = Registry.CurrentUser.OpenSubKey(StartupRegistryKeyPath, writable: true);
        if (key is null)
        {
            throw new InvalidOperationException("Windows 자동 실행 레지스트리 키에 접근할 수 없습니다.");
        }

        if (enabled)
        {
            string executablePath = GetExecutablePath();
            key.SetValue(StartupRegistryValueName, $"\"{executablePath}\"");
        }
        else
        {
            key.DeleteValue(StartupRegistryValueName, throwOnMissingValue: false);
        }
    }

    private static bool IsStartMenuShortcutRegistered()
    {
        return File.Exists(GetStartMenuShortcutPath());
    }

    private static void SetStartMenuShortcut(bool enabled)
    {
        string shortcutPath = GetStartMenuShortcutPath();
        if (!enabled)
        {
            File.Delete(shortcutPath);
            return;
        }

        Directory.CreateDirectory(Path.GetDirectoryName(shortcutPath)!);

        Type shellType = Type.GetTypeFromProgID("WScript.Shell")
            ?? throw new InvalidOperationException("Windows 바로가기 서비스를 불러올 수 없습니다.");
        object shell = Activator.CreateInstance(shellType)
            ?? throw new InvalidOperationException("Windows 바로가기 서비스를 시작할 수 없습니다.");
        object? shortcut = null;

        try
        {
            shortcut = shellType.InvokeMember(
                "CreateShortcut",
                BindingFlags.InvokeMethod,
                binder: null,
                target: shell,
                args: [shortcutPath])
                ?? throw new InvalidOperationException("시작 메뉴 바로가기를 만들 수 없습니다.");

            Type shortcutType = shortcut.GetType();
            string executablePath = GetExecutablePath();
            shortcutType.InvokeMember("TargetPath", BindingFlags.SetProperty, null, shortcut, [executablePath]);
            shortcutType.InvokeMember(
                "WorkingDirectory",
                BindingFlags.SetProperty,
                null,
                shortcut,
                [Path.GetDirectoryName(executablePath)!]);
            shortcutType.InvokeMember("Description", BindingFlags.SetProperty, null, shortcut, ["Launch Tuna Helper"]);
            shortcutType.InvokeMember("Save", BindingFlags.InvokeMethod, null, shortcut, args: null);
        }
        finally
        {
            ReleaseComObject(shortcut);
            ReleaseComObject(shell);
        }
    }

    private static string GetStartMenuShortcutPath()
    {
        return Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.Programs),
            StartMenuShortcutFileName);
    }

    private static string GetExecutablePath()
    {
        return Environment.ProcessPath ?? Process.GetCurrentProcess().MainModule?.FileName
            ?? throw new InvalidOperationException("실행 파일 경로를 확인할 수 없습니다.");
    }

    private static void ReleaseComObject(object? comObject)
    {
        if (comObject is not null && Marshal.IsComObject(comObject))
        {
            Marshal.FinalReleaseComObject(comObject);
        }
    }

    private void ShowProjectNotFoundMessage()
    {
        MessageBox.Show(
            "TunaSweeper.uproject를 찾을 수 없습니다.\n앱을 프로젝트 저장소 내부에서 실행해 주세요.",
            Title,
            MessageBoxButton.OK,
            MessageBoxImage.Error);
    }
}
