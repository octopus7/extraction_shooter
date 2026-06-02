using System.Diagnostics;
using System.IO;
using System.Windows;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Threading;

namespace TunaBuildHelper;

public partial class MainWindow : Window
{
    private const double DragBandThickness = 8.0;
    private static readonly TimeSpan CloseHoverDelay = TimeSpan.FromMilliseconds(650);
    private static readonly TimeSpan CloseHideDelay = TimeSpan.FromSeconds(1);

    private readonly DispatcherTimer closeHoverTimer;
    private readonly DispatcherTimer closeHideTimer;
    private readonly Brush normalBorderBrush;
    private readonly Brush hoverBorderBrush;
    private bool closeButtonHovered;

    public MainWindow()
    {
        InitializeComponent();

        normalBorderBrush = (Brush)FindResource("BorderBrush");
        hoverBorderBrush = (Brush)FindResource("BorderHoverBrush");

        closeHoverTimer = new DispatcherTimer
        {
            Interval = CloseHoverDelay
        };
        closeHoverTimer.Tick += CloseHoverTimer_Tick;

        closeHideTimer = new DispatcherTimer
        {
            Interval = CloseHideDelay
        };
        closeHideTimer.Tick += CloseHideTimer_Tick;
    }

    private void Window_Loaded(object sender, RoutedEventArgs e)
    {
        var area = SystemParameters.WorkArea;
        Left = area.Right - ActualWidth - 18;
        Top = area.Top + 86;
    }

    private void KillButton_Click(object sender, RoutedEventArgs e)
    {
        RunBatch("KillTunaSweeperEditor.bat", closeAlways: true);
    }

    private void BuildButton_Click(object sender, RoutedEventArgs e)
    {
        RunBatch("BuildAndRunTunaSweeper.bat", closeOnSuccess: true);
    }

    private void CloseButton_Click(object sender, RoutedEventArgs e)
    {
        Close();
    }

    private void ChromeBorder_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
    {
        if (e.ButtonState != MouseButtonState.Pressed || !IsMouseOnDragBand())
        {
            return;
        }

        try
        {
            DragMove();
        }
        catch (InvalidOperationException)
        {
            // DragMove can throw if the mouse state changes between the hit test and call.
        }
    }

    private void Window_MouseMove(object sender, MouseEventArgs e)
    {
        if (IsMouseOnDragBand() || closeButtonHovered)
        {
            closeHideTimer.Stop();
            ChromeBorder.BorderBrush = hoverBorderBrush;
            if (!closeHoverTimer.IsEnabled && CloseButton.Visibility != Visibility.Visible)
            {
                closeHoverTimer.Start();
            }

            return;
        }

        ScheduleHideCloseButton();
    }

    private void Window_MouseLeave(object sender, MouseEventArgs e)
    {
        if (!closeButtonHovered)
        {
            ScheduleHideCloseButton();
        }
    }

    private void CloseButton_MouseEnter(object sender, MouseEventArgs e)
    {
        closeButtonHovered = true;
        closeHoverTimer.Stop();
        closeHideTimer.Stop();
        CloseButton.Visibility = Visibility.Visible;
        ChromeBorder.BorderBrush = hoverBorderBrush;
    }

    private void CloseButton_MouseLeave(object sender, MouseEventArgs e)
    {
        closeButtonHovered = false;
        if (!IsMouseOnDragBand())
        {
            ScheduleHideCloseButton();
        }
    }

    private void CloseHoverTimer_Tick(object? sender, EventArgs e)
    {
        closeHoverTimer.Stop();
        if (IsMouseOnDragBand() || closeButtonHovered)
        {
            closeHideTimer.Stop();
            CloseButton.Visibility = Visibility.Visible;
        }
    }

    private void CloseHideTimer_Tick(object? sender, EventArgs e)
    {
        closeHideTimer.Stop();
        if (IsMouseOnDragBand() || closeButtonHovered)
        {
            ChromeBorder.BorderBrush = hoverBorderBrush;
            return;
        }

        HideCloseButton();
    }

    private void ScheduleHideCloseButton()
    {
        closeHoverTimer.Stop();
        if (!closeHideTimer.IsEnabled)
        {
            closeHideTimer.Start();
        }
    }

    private void HideCloseButton()
    {
        closeHoverTimer.Stop();
        closeHideTimer.Stop();
        CloseButton.Visibility = Visibility.Collapsed;
        ChromeBorder.BorderBrush = normalBorderBrush;
    }

    private bool IsMouseOnDragBand()
    {
        Point position = Mouse.GetPosition(ChromeBorder);
        double width = ChromeBorder.ActualWidth;
        double height = ChromeBorder.ActualHeight;

        if (position.X < 0 || position.Y < 0 || position.X > width || position.Y > height)
        {
            return false;
        }

        return position.X <= DragBandThickness ||
               position.Y <= DragBandThickness ||
               position.X >= width - DragBandThickness ||
               position.Y >= height - DragBandThickness;
    }

    private static void RunBatch(string scriptName, bool closeOnSuccess = false, bool closeAlways = false)
    {
        string? scriptPath = FindScript(scriptName);
        if (scriptPath is null)
        {
            MessageBox.Show(
                $"Cannot find {scriptName}.",
                "Tuna Build Helper",
                MessageBoxButton.OK,
                MessageBoxImage.Error);
            return;
        }

        var startInfo = new ProcessStartInfo
        {
            FileName = "cmd.exe",
            Arguments = GetCmdArguments(scriptPath, closeOnSuccess, closeAlways),
            WorkingDirectory = Path.GetDirectoryName(scriptPath) ?? AppContext.BaseDirectory,
            UseShellExecute = true
        };

        Process.Start(startInfo);
    }

    private static string GetCmdArguments(string scriptPath, bool closeOnSuccess, bool closeAlways)
    {
        if (closeAlways)
        {
            return $"/c \"call \"\"{scriptPath}\"\"\"";
        }

        if (closeOnSuccess)
        {
            return $"/c \"call \"\"{scriptPath}\"\" & if errorlevel 1 pause\"";
        }

        return $"/k \"\"{scriptPath}\"\"";
    }

    private static string? FindScript(string scriptName)
    {
        DirectoryInfo? directory = new(AppContext.BaseDirectory);
        while (directory is not null)
        {
            string candidate = Path.Combine(directory.FullName, scriptName);
            if (File.Exists(candidate))
            {
                return candidate;
            }

            directory = directory.Parent;
        }

        return null;
    }
}
