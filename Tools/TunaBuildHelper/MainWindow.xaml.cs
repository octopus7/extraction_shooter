using Microsoft.Win32;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Windows;

namespace TunaBuildHelper;

public partial class MainWindow : Window
{
    private const string StartupRegistryValueName = "TunaHelper";
    private const string StartupRegistryKeyPath = @"Software\Microsoft\Windows\CurrentVersion\Run";
    private const string StartMenuShortcutFileName = "Tuna Helper.lnk";

    public MainWindow()
    {
        InitializeComponent();
        RunAtStartupMenuItem.IsChecked = IsRunAtStartupEnabled();
        RegisterStartMenuMenuItem.IsChecked = IsStartMenuShortcutRegistered();
    }

    private void BuildAndRunButton_Click(object sender, RoutedEventArgs e)
    {
        RunBatchScript("BuildAndRunTunaSweeper.bat", closeOnSuccess: true);
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

    private void RunBatchScript(string scriptName, bool closeOnSuccess = false, bool closeAlways = false)
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

        string arguments = closeAlways
            ? $"/c \"call \"\"{scriptPath}\"\"\""
            : closeOnSuccess
                ? $"/c \"call \"\"{scriptPath}\"\" & if errorlevel 1 pause\""
                : $"/k \"\"{scriptPath}\"\"";

        try
        {
            Process.Start(new ProcessStartInfo
            {
                FileName = "cmd.exe",
                Arguments = arguments,
                WorkingDirectory = Path.GetDirectoryName(scriptPath) ?? AppContext.BaseDirectory,
                UseShellExecute = true
            });
            StatusTextBlock.Text = closeAlways
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
