using System.Windows;
using System.Windows.Media.Imaging;

namespace YTThumbnailComposer;

public partial class MainWindow : Window
{
    public MainWindow()
    {
        InitializeComponent();
        BackgroundSelector.ItemsSource = ThumbnailRenderer.Backgrounds;
        BackgroundSelector.SelectedIndex = 0;
        EpisodeNumberText.Text = "1";
        RenderPreview();
    }

    private void BackgroundSelector_SelectionChanged(object sender, System.Windows.Controls.SelectionChangedEventArgs e)
    {
        RenderPreview();
    }

    private void EpisodeNumberText_TextChanged(object sender, System.Windows.Controls.TextChangedEventArgs e)
    {
        RenderPreview();
    }

    private void RenderSelected_Click(object sender, RoutedEventArgs e)
    {
        if (BackgroundSelector.SelectedItem is not BackgroundOption option)
        {
            return;
        }

        string path = ThumbnailRenderer.RenderToFile(option, GetEpisodeNumber());
        StatusText.Text = $"Saved {path}";
        PreviewImage.Source = LoadPreview(path);
    }

    private void RenderAll_Click(object sender, RoutedEventArgs e)
    {
        string[] paths = ThumbnailRenderer.RenderAllToFiles(GetEpisodeNumber());
        StatusText.Text = $"Saved {paths.Length} thumbnails";
        if (BackgroundSelector.SelectedItem is BackgroundOption option)
        {
            PreviewImage.Source = ThumbnailRenderer.Render(option, GetEpisodeNumber());
        }
    }

    private void RenderPreview()
    {
        if (!IsLoaded && BackgroundSelector.SelectedItem is null)
        {
            return;
        }

        if (BackgroundSelector.SelectedItem is not BackgroundOption option)
        {
            return;
        }

        try
        {
            PreviewImage.Source = ThumbnailRenderer.Render(option, GetEpisodeNumber());
            StatusText.Text = "Preview";
        }
        catch (Exception ex)
        {
            StatusText.Text = ex.Message;
        }
    }

    private string GetEpisodeNumber()
    {
        string value = EpisodeNumberText.Text.Trim();
        return value.Length == 0 ? "1" : value;
    }

    private static BitmapImage LoadPreview(string path)
    {
        var image = new BitmapImage();
        image.BeginInit();
        image.CacheOption = BitmapCacheOption.OnLoad;
        image.UriSource = new Uri(path, UriKind.Absolute);
        image.EndInit();
        image.Freeze();
        return image;
    }
}
