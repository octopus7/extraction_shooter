using System.Windows;

namespace YTThumbnailComposer;

public partial class App : Application
{
    protected override void OnStartup(StartupEventArgs e)
    {
        base.OnStartup(e);

        if (e.Args.Any(arg => string.Equals(arg, "--render-all", StringComparison.OrdinalIgnoreCase)))
        {
            ThumbnailRenderer.RenderAllToFiles();
            Shutdown();
            return;
        }

        var window = new MainWindow();
        window.Show();
    }
}
