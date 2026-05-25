using Microsoft.Win32;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Globalization;
using System.IO;
using System.Runtime.CompilerServices;
using System.Text.Json;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using Rectangle = System.Windows.Shapes.Rectangle;

namespace IconCropTool;

public partial class MainWindow : Window
{
    private const double MinCropSize = 4;

    private readonly ObservableCollection<IconPreviewItem> _iconPreviews = [];
    private CropDocument? _document;
    private string? _jsonPath;
    private string? _outputDirectory;
    private BitmapSource? _sheetBitmap;
    private IconEntry? _selectedIcon;
    private bool _updatingFields;

    private bool _isDragging;
    private DragMode _dragMode;
    private Point _dragStartPoint;
    private IconEntry? _dragIcon;
    private CropRect? _dragSelectedOriginalRect;
    private Dictionary<IconEntry, CropRect> _dragOriginalRects = [];

    public MainWindow()
    {
        InitializeComponent();
        IconGrid.ItemsSource = _iconPreviews;
        OverlayCanvas.MouseMove += OverlayCanvas_MouseMove;
        OverlayCanvas.MouseLeftButtonUp += OverlayCanvas_MouseLeftButtonUp;
        Loaded += MainWindow_Loaded;
    }

    private double Zoom => Math.Max(0.05, ZoomSlider.Value);

    private bool UseSharedMode => SharedModeCheckBox.IsChecked == true;

    private AutoTrimMode SelectedAutoTrimMode => AutoTrimModeComboBox.SelectedIndex switch
    {
        1 => AutoTrimMode.EdgeRecover,
        2 => AutoTrimMode.EdgeOff,
        _ => AutoTrimMode.CenterClean
    };

    private byte AlphaTrimThreshold
    {
        get
        {
            if (!TryParseNumber(AlphaThresholdText.Text, out double value))
            {
                return 8;
            }

            return (byte)Math.Clamp((int)Math.Round(value), 0, 254);
        }
    }

    private void MainWindow_Loaded(object sender, RoutedEventArgs e)
    {
        string? defaultDirectory = FindDefaultDataDirectory();
        if (defaultDirectory is null)
        {
            StatusText.Text = "No *_cropInfo.json found. Use Load JSON.";
            return;
        }

        string? firstJson = Directory.GetFiles(defaultDirectory, "*_cropInfo.json")
            .OrderBy(static path => path, StringComparer.OrdinalIgnoreCase)
            .FirstOrDefault();
        if (firstJson is not null)
        {
            LoadDocument(firstJson);
        }
    }

    private void LoadJson_Click(object sender, RoutedEventArgs e)
    {
        var dialog = new OpenFileDialog
        {
            Filter = "Crop info JSON (*_cropInfo.json)|*_cropInfo.json|JSON (*.json)|*.json|All files (*.*)|*.*",
            InitialDirectory = GetCurrentDocumentDirectory() ?? FindDefaultDataDirectory() ?? Directory.GetCurrentDirectory()
        };

        if (dialog.ShowDialog(this) == true)
        {
            LoadDocument(dialog.FileName);
        }
    }

    private void SaveJson_Click(object sender, RoutedEventArgs e)
    {
        if (_document is null || string.IsNullOrWhiteSpace(_jsonPath))
        {
            StatusText.Text = "No crop JSON is loaded.";
            return;
        }

        try
        {
            IconCropRenderer.NormalizeDocument(_document);
            string json = JsonSerializer.Serialize(_document, IconCropRenderer.JsonOptions);
            File.WriteAllText(_jsonPath, json);
            StatusText.Text = $"Saved {Path.GetFileName(_jsonPath)}";
            RefreshViews(refreshPreviews: true);
        }
        catch (Exception ex)
        {
            StatusText.Text = $"Save failed: {ex.Message}";
        }
    }

    private void RunCrop_Click(object sender, RoutedEventArgs e)
    {
        if (_document is null || _jsonPath is null)
        {
            StatusText.Text = "No crop JSON is loaded.";
            return;
        }

        try
        {
            string documentDirectory = Path.GetDirectoryName(_jsonPath) ?? Directory.GetCurrentDirectory();
            string outputDirectory = string.IsNullOrWhiteSpace(_outputDirectory)
                ? documentDirectory
                : _outputDirectory;

            IconCropRenderer.NormalizeDocument(_document);
            int saved = IconCropRenderer.CropAll(_document, documentDirectory, outputDirectory);
            StatusText.Text = $"Cropped {saved} icons.";
            RefreshViews(refreshPreviews: true);
        }
        catch (Exception ex)
        {
            StatusText.Text = $"Crop failed: {ex.Message}";
        }
    }

    private void ZoomSlider_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
    {
        if (OverlayCanvas is null)
        {
            return;
        }

        RenderOverlays();
    }

    private void IconPreview_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
    {
        if (sender is FrameworkElement { DataContext: IconPreviewItem item })
        {
            SelectIcon(item.Icon);
            e.Handled = true;
        }
    }

    private void OverlayCanvas_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
    {
        Keyboard.ClearFocus();
    }

    private void OverlayElement_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
    {
        if (sender is not FrameworkElement { Tag: OverlayTag tag } || _document is null)
        {
            return;
        }

        SelectIcon(tag.Icon);
        Keyboard.ClearFocus();

        _isDragging = true;
        _dragMode = tag.Mode;
        _dragIcon = tag.Icon;
        _dragStartPoint = e.GetPosition(OverlayCanvas);
        _dragSelectedOriginalRect = tag.Icon.SourceCropRect.Clone();
        _dragOriginalRects = _document.Icons.ToDictionary(static icon => icon, static icon => icon.SourceCropRect.Clone());

        OverlayCanvas.CaptureMouse();
        e.Handled = true;
    }

    private void OverlayCanvas_MouseMove(object sender, MouseEventArgs e)
    {
        if (!_isDragging || _dragIcon is null || _dragSelectedOriginalRect is null || e.LeftButton != MouseButtonState.Pressed)
        {
            return;
        }

        Point current = e.GetPosition(OverlayCanvas);
        double dx = (current.X - _dragStartPoint.X) / Zoom;
        double dy = (current.Y - _dragStartPoint.Y) / Zoom;
        CropRect proposed = _dragSelectedOriginalRect.Clone();

        if (_dragMode == DragMode.Move)
        {
            proposed.X += dx;
            proposed.Y += dy;
        }
        else
        {
            proposed.Width = Math.Max(MinCropSize, proposed.Width + dx);
            proposed.Height = Math.Max(MinCropSize, proposed.Height + dy);
        }

        ApplyRectChange(_dragIcon, proposed, _dragOriginalRects, _dragSelectedOriginalRect);
        RefreshViews(refreshPreviews: true);
    }

    private void OverlayCanvas_MouseLeftButtonUp(object sender, MouseButtonEventArgs e)
    {
        if (!_isDragging)
        {
            return;
        }

        _isDragging = false;
        _dragIcon = null;
        _dragSelectedOriginalRect = null;
        _dragOriginalRects = [];
        OverlayCanvas.ReleaseMouseCapture();
        RefreshViews(refreshPreviews: true);
        e.Handled = true;
    }

    private void CropField_LostFocus(object sender, RoutedEventArgs e)
    {
        if (!_updatingFields)
        {
            ApplyCropFields();
        }
    }

    private void CropField_KeyDown(object sender, KeyEventArgs e)
    {
        if (e.Key == Key.Enter)
        {
            ApplyCropFields();
            Keyboard.ClearFocus();
            e.Handled = true;
        }
    }

    private void ApplyCropFields_Click(object sender, RoutedEventArgs e)
    {
        ApplyCropFields();
    }

    private void UseJsonFolderForOutput_Click(object sender, RoutedEventArgs e)
    {
        _outputDirectory = GetCurrentDocumentDirectory();
        OutputInfoText.Text = "JSON folder";
    }

    private void ChooseOutput_Click(object sender, RoutedEventArgs e)
    {
        var dialog = new OpenFolderDialog
        {
            Title = "Choose output folder",
            InitialDirectory = _outputDirectory ?? GetCurrentDocumentDirectory() ?? FindDefaultDataDirectory() ?? Directory.GetCurrentDirectory()
        };

        if (dialog.ShowDialog(this) == true)
        {
            _outputDirectory = dialog.FolderName;
            OutputInfoText.Text = "Custom folder";
        }
    }

    private void AutoTrimSelected_Click(object sender, RoutedEventArgs e)
    {
        if (_document is null || _selectedIcon is null)
        {
            StatusText.Text = "No icon is selected.";
            return;
        }

        SharedModeCheckBox.IsChecked = false;
        var alphaMasks = new Dictionary<string, IconCropRenderer.AlphaMask>(StringComparer.OrdinalIgnoreCase);
        byte threshold = AlphaTrimThreshold;
        AutoTrimMode mode = SelectedAutoTrimMode;
        var trace = new List<string>
        {
            $"Auto trim trace {DateTime.Now:yyyy-MM-dd HH:mm:ss}",
            $"Json={Path.GetFileName(_jsonPath)}",
            $"Icon index={_selectedIcon.Index}",
            $"Icon file={_selectedIcon.IconFilename}",
            $"Icon row={_selectedIcon.Row}, column={_selectedIcon.Column}",
            $"Alpha threshold>{threshold}",
            $"Auto trim mode={GetAutoTrimModeLabel(mode)}",
            $"Shared mode after auto-disable={UseSharedMode}",
            $"Selected rect before={FormatCropRect(_selectedIcon.SourceCropRect)}"
        };

        try
        {
            bool trimmed = TryAutoTrimIcon(_selectedIcon, alphaMasks, mode, threshold, trace);
            trace.Add($"Selected rect after={FormatCropRect(_selectedIcon.SourceCropRect)}");
            trace.Add($"Trimmed={trimmed}");
            WriteTrimTrace(_selectedIcon, trace);

            if (trimmed)
            {
                IconCropRenderer.NormalizeDocument(_document);
                RefreshViews(refreshPreviews: true);
                StatusText.Text = $"Auto trimmed {_selectedIcon.IconFilename} with {GetAutoTrimModeLabel(mode)}. Trace written.";
            }
            else
            {
                StatusText.Text = $"No pixels above alpha {threshold} found. Trace written.";
            }
        }
        catch (Exception ex)
        {
            trace.Add($"Exception={ex}");
            TryWriteTrimTrace(_selectedIcon, trace);
            StatusText.Text = $"Auto trim failed: {ex.Message}. Trace written.";
        }
    }

    private void AutoTrimAll_Click(object sender, RoutedEventArgs e)
    {
        if (_document is null)
        {
            StatusText.Text = "No crop JSON is loaded.";
            return;
        }

        SharedModeCheckBox.IsChecked = false;
        var alphaMasks = new Dictionary<string, IconCropRenderer.AlphaMask>(StringComparer.OrdinalIgnoreCase);
        int trimmed = 0;
        int skipped = 0;

        try
        {
            byte threshold = AlphaTrimThreshold;
            AutoTrimMode mode = SelectedAutoTrimMode;
            foreach (IconEntry icon in _document.Icons)
            {
                if (TryAutoTrimIcon(icon, alphaMasks, mode, threshold))
                {
                    trimmed++;
                }
                else
                {
                    skipped++;
                }
            }

            IconCropRenderer.NormalizeDocument(_document);
            RefreshViews(refreshPreviews: true);
            StatusText.Text = $"Auto trimmed {trimmed} icons with {GetAutoTrimModeLabel(mode)}, skipped {skipped}.";
        }
        catch (Exception ex)
        {
            StatusText.Text = $"Auto trim failed: {ex.Message}";
        }
    }

    private void LoadDocument(string path)
    {
        try
        {
            string json = File.ReadAllText(path);
            CropDocument document = JsonSerializer.Deserialize<CropDocument>(json, IconCropRenderer.JsonOptions)
                ?? throw new InvalidOperationException("The crop JSON file is empty.");
            if (document.Icons.Count == 0)
            {
                throw new InvalidOperationException("The crop JSON does not contain any icons.");
            }

            string documentDirectory = Path.GetDirectoryName(path) ?? Directory.GetCurrentDirectory();
            string sheetPath = Path.Combine(documentDirectory, document.SheetImageFilename);
            if (!File.Exists(sheetPath))
            {
                throw new FileNotFoundException("Sheet image not found.", sheetPath);
            }

            _document = document;
            _jsonPath = path;
            _sheetBitmap = IconCropRenderer.LoadBitmap(sheetPath);
            _document.SheetSize ??= new SheetSize();
            _document.SheetSize.Width = _sheetBitmap.PixelWidth;
            _document.SheetSize.Height = _sheetBitmap.PixelHeight;

            _outputDirectory = documentDirectory;
            Title = $"Icon Crop Tool - {Path.GetFileName(path)}";
            OutputInfoText.Text = "JSON folder";
            SheetImage.Source = _sheetBitmap;
            SheetInfoText.Text = $"{Path.GetFileName(sheetPath)} ({_sheetBitmap.PixelWidth}x{_sheetBitmap.PixelHeight})";

            _iconPreviews.Clear();
            foreach (IconEntry icon in _document.Icons.OrderBy(static icon => icon.Index))
            {
                _iconPreviews.Add(new IconPreviewItem(icon));
            }

            SelectIcon(_document.Icons.OrderBy(static icon => icon.Index).First());
            StatusText.Text = $"Loaded {Path.GetFileName(path)}";
            RefreshViews(refreshPreviews: true);
        }
        catch (Exception ex)
        {
            StatusText.Text = $"Load failed: {ex.Message}";
        }
    }

    private void SelectIcon(IconEntry? icon)
    {
        _selectedIcon = icon;
        foreach (IconPreviewItem item in _iconPreviews)
        {
            item.SetSelected(ReferenceEquals(item.Icon, icon));
        }

        UpdateSelectedFields();
        RenderOverlays();
    }

    private void ApplyCropFields()
    {
        if (_selectedIcon is null)
        {
            return;
        }

        if (!TryReadCropFields(out CropRect proposed))
        {
            StatusText.Text = "Crop fields must be numeric.";
            return;
        }

        Dictionary<IconEntry, CropRect> originals = _document?.Icons.ToDictionary(static icon => icon, static icon => icon.SourceCropRect.Clone()) ?? [];
        CropRect selectedOriginal = _selectedIcon.SourceCropRect.Clone();
        ApplyRectChange(_selectedIcon, proposed, originals, selectedOriginal);
        RefreshViews(refreshPreviews: true);
    }

    private void ApplyRectChange(
        IconEntry editedIcon,
        CropRect proposedRect,
        IReadOnlyDictionary<IconEntry, CropRect> originalRects,
        CropRect editedOriginalRect)
    {
        if (_document is null)
        {
            return;
        }

        CropRect clampedProposed = ClampRectToSheet(proposedRect);
        double dx = clampedProposed.X - editedOriginalRect.X;
        double dy = clampedProposed.Y - editedOriginalRect.Y;
        double dw = clampedProposed.Width - editedOriginalRect.Width;
        double dh = clampedProposed.Height - editedOriginalRect.Height;

        foreach (IconEntry icon in _document.Icons)
        {
            CropRect original = originalRects.TryGetValue(icon, out CropRect? rect)
                ? rect
                : icon.SourceCropRect.Clone();
            CropRect next = original.Clone();

            if (UseSharedMode)
            {
                if (icon.Column == editedIcon.Column)
                {
                    next.X = original.X + dx;
                    next.Width = original.Width + dw;
                }

                if (icon.Row == editedIcon.Row)
                {
                    next.Y = original.Y + dy;
                    next.Height = original.Height + dh;
                }
            }
            else if (ReferenceEquals(icon, editedIcon))
            {
                next = clampedProposed.Clone();
            }

            icon.SourceCropRect.CopyFrom(ClampRectToSheet(next));
        }

        IconCropRenderer.NormalizeDocument(_document);
    }

    private bool TryAutoTrimIcon(
        IconEntry icon,
        Dictionary<string, IconCropRenderer.AlphaMask> alphaMasks,
        AutoTrimMode mode,
        byte threshold,
        IList<string>? trace = null)
    {
        trace?.Add($"TryAutoTrimIcon begin rect={FormatCropRect(icon.SourceCropRect)}");
        trace?.Add($"Source sheet={icon.SourceSheetFilename}");
        trace?.Add($"Mode={GetAutoTrimModeLabel(mode)}");
        IconCropRenderer.AlphaMask alphaMask = GetAlphaMaskForIcon(icon, alphaMasks);
        CropRect? trimmedRect = mode switch
        {
            AutoTrimMode.CenterClean => IconCropRenderer.FindCenterOutCleanTrimRect(
                alphaMask,
                icon.SourceCropRect,
                threshold,
                trace),
            AutoTrimMode.EdgeRecover => IconCropRenderer.FindAlphaTrimRect(
                alphaMask,
                icon.SourceCropRect,
                trimFromFirstEmptyAfterEdgeContent: true,
                threshold,
                trace),
            AutoTrimMode.EdgeOff => IconCropRenderer.FindAlphaTrimRect(
                alphaMask,
                icon.SourceCropRect,
                trimFromFirstEmptyAfterEdgeContent: false,
                threshold,
                trace),
            _ => throw new InvalidOperationException($"Unknown auto trim mode: {mode}")
        };
        if (trimmedRect is null)
        {
            trace?.Add("Renderer returned null trim rect.");
            return false;
        }

        CropRect clampedRect = ClampRectToSheet(trimmedRect);
        trace?.Add($"Renderer raw rect={FormatCropRect(trimmedRect)}");
        trace?.Add($"Clamped rect={FormatCropRect(clampedRect)}");
        icon.SourceCropRect.CopyFrom(clampedRect);
        return true;
    }

    private void WriteTrimTrace(IconEntry icon, IReadOnlyList<string> trace)
    {
        string directory = GetTrimLogDirectory();
        Directory.CreateDirectory(directory);

        string timestamp = DateTime.Now.ToString("yyyyMMdd_HHmmss", CultureInfo.InvariantCulture);
        string iconName = Path.GetFileNameWithoutExtension(icon.IconFilename);
        if (string.IsNullOrWhiteSpace(iconName))
        {
            iconName = $"icon_{icon.Index:000}";
        }

        string filename = $"{timestamp}_{icon.Index:000}_{SanitizeFileName(iconName)}.log";
        File.WriteAllLines(Path.Combine(directory, filename), trace);
    }

    private void TryWriteTrimTrace(IconEntry icon, IReadOnlyList<string> trace)
    {
        try
        {
            WriteTrimTrace(icon, trace);
        }
        catch
        {
            // Avoid hiding the original trim failure behind a logging failure.
        }
    }

    private string GetTrimLogDirectory()
    {
        string documentDirectory = GetCurrentDocumentDirectory() ?? Directory.GetCurrentDirectory();
        DirectoryInfo? directory = new(documentDirectory);
        if (string.Equals(directory.Name, "Icons", StringComparison.OrdinalIgnoreCase) && directory.Parent is not null)
        {
            return Path.Combine(directory.Parent.FullName, "TrimLogs");
        }

        return Path.Combine(documentDirectory, "TrimLogs");
    }

    private static string SanitizeFileName(string value)
    {
        foreach (char invalidChar in Path.GetInvalidFileNameChars())
        {
            value = value.Replace(invalidChar, '_');
        }

        return value;
    }

    private static string FormatCropRect(CropRect rect)
    {
        return $"x={IconCropRenderer.FormatNumber(rect.X)}, y={IconCropRenderer.FormatNumber(rect.Y)}, w={IconCropRenderer.FormatNumber(rect.Width)}, h={IconCropRenderer.FormatNumber(rect.Height)}, r={IconCropRenderer.FormatNumber(rect.X + rect.Width)}, b={IconCropRenderer.FormatNumber(rect.Y + rect.Height)}";
    }

    private static string GetAutoTrimModeLabel(AutoTrimMode mode)
    {
        return mode switch
        {
            AutoTrimMode.CenterClean => "1 Center clean",
            AutoTrimMode.EdgeRecover => "2 Edge recover",
            AutoTrimMode.EdgeOff => "3 Edge off",
            _ => mode.ToString()
        };
    }

    private IconCropRenderer.AlphaMask GetAlphaMaskForIcon(
        IconEntry icon,
        Dictionary<string, IconCropRenderer.AlphaMask> alphaMasks)
    {
        string sheetPath = GetSheetPathForIcon(icon);
        if (alphaMasks.TryGetValue(sheetPath, out IconCropRenderer.AlphaMask? alphaMask))
        {
            return alphaMask;
        }

        BitmapSource sheet = IsLoadedDocumentSheet(icon, sheetPath) && _sheetBitmap is not null
            ? _sheetBitmap
            : IconCropRenderer.LoadBitmap(sheetPath);
        alphaMask = IconCropRenderer.CreateAlphaMask(sheet);
        alphaMasks[sheetPath] = alphaMask;
        return alphaMask;
    }

    private string GetSheetPathForIcon(IconEntry icon)
    {
        if (_document is null || _jsonPath is null)
        {
            throw new InvalidOperationException("No crop JSON is loaded.");
        }

        string documentDirectory = Path.GetDirectoryName(_jsonPath) ?? Directory.GetCurrentDirectory();
        string sheetFilename = string.IsNullOrWhiteSpace(icon.SourceSheetFilename)
            ? _document.SheetImageFilename
            : icon.SourceSheetFilename!;
        return Path.GetFullPath(Path.Combine(documentDirectory, sheetFilename));
    }

    private bool IsLoadedDocumentSheet(IconEntry icon, string sheetPath)
    {
        if (_document is null || _jsonPath is null)
        {
            return false;
        }

        string sheetFilename = string.IsNullOrWhiteSpace(icon.SourceSheetFilename)
            ? _document.SheetImageFilename
            : icon.SourceSheetFilename!;
        string documentDirectory = Path.GetDirectoryName(_jsonPath) ?? Directory.GetCurrentDirectory();
        string documentSheetPath = Path.GetFullPath(Path.Combine(documentDirectory, _document.SheetImageFilename));
        string iconSheetPath = Path.GetFullPath(Path.Combine(documentDirectory, sheetFilename));
        return string.Equals(sheetPath, documentSheetPath, StringComparison.OrdinalIgnoreCase) &&
               string.Equals(iconSheetPath, documentSheetPath, StringComparison.OrdinalIgnoreCase);
    }

    private void RefreshViews(bool refreshPreviews)
    {
        UpdateSelectedFields();
        RenderOverlays();
        if (refreshPreviews)
        {
            RenderAllPreviews();
        }
    }

    private void RenderAllPreviews()
    {
        if (_document is null || _sheetBitmap is null)
        {
            return;
        }

        try
        {
            foreach (IconPreviewItem item in _iconPreviews)
            {
                item.Preview = IconCropRenderer.RenderPreview(_document, _sheetBitmap, item.Icon);
            }

            PreviewInfoText.Text = $"{_iconPreviews.Count} icons";
        }
        catch (Exception ex)
        {
            StatusText.Text = $"Preview failed: {ex.Message}";
        }
    }

    private void RenderOverlays()
    {
        OverlayCanvas.Children.Clear();

        if (_document is null || _sheetBitmap is null)
        {
            return;
        }

        double scaledWidth = _sheetBitmap.PixelWidth * Zoom;
        double scaledHeight = _sheetBitmap.PixelHeight * Zoom;
        SheetHost.Width = scaledWidth;
        SheetHost.Height = scaledHeight;
        SheetImage.Width = scaledWidth;
        SheetImage.Height = scaledHeight;
        OverlayCanvas.Width = scaledWidth;
        OverlayCanvas.Height = scaledHeight;

        foreach (IconEntry icon in _document.Icons.Where(icon => !ReferenceEquals(icon, _selectedIcon)))
        {
            AddOverlayForIcon(icon, isSelected: false);
        }

        if (_selectedIcon is not null)
        {
            AddOverlayForIcon(_selectedIcon, isSelected: true);
            AddResizeHandle(_selectedIcon);
        }
    }

    private void AddOverlayForIcon(IconEntry icon, bool isSelected)
    {
        CropRect rect = icon.SourceCropRect;
        var border = new Rectangle
        {
            Width = Math.Max(1, rect.Width * Zoom),
            Height = Math.Max(1, rect.Height * Zoom),
            StrokeThickness = isSelected ? 2.5 : 1.4,
            Stroke = isSelected ? Brushes.Orange : Brushes.DeepSkyBlue,
            Fill = isSelected
                ? new SolidColorBrush(Color.FromArgb(26, 255, 174, 42))
                : new SolidColorBrush(Color.FromArgb(18, 0, 191, 255)),
            Cursor = Cursors.SizeAll,
            Tag = new OverlayTag(icon, DragMode.Move),
            ToolTip = $"{icon.Index}: {icon.IconFilename}"
        };
        border.MouseLeftButtonDown += OverlayElement_MouseLeftButtonDown;
        Canvas.SetLeft(border, rect.X * Zoom);
        Canvas.SetTop(border, rect.Y * Zoom);
        OverlayCanvas.Children.Add(border);

        var label = new TextBlock
        {
            Text = icon.Index.ToString(CultureInfo.InvariantCulture),
            Foreground = Brushes.White,
            Background = isSelected ? Brushes.OrangeRed : Brushes.DodgerBlue,
            FontSize = 11,
            FontWeight = FontWeights.SemiBold,
            Padding = new Thickness(3, 1, 3, 1),
            IsHitTestVisible = false
        };
        Canvas.SetLeft(label, rect.X * Zoom + 3);
        Canvas.SetTop(label, rect.Y * Zoom + 3);
        OverlayCanvas.Children.Add(label);
    }

    private void AddResizeHandle(IconEntry icon)
    {
        const double handleSize = 13;
        CropRect rect = icon.SourceCropRect;
        var handle = new Rectangle
        {
            Width = handleSize,
            Height = handleSize,
            Fill = Brushes.Orange,
            Stroke = Brushes.White,
            StrokeThickness = 1,
            Cursor = Cursors.SizeNWSE,
            Tag = new OverlayTag(icon, DragMode.Resize)
        };
        handle.MouseLeftButtonDown += OverlayElement_MouseLeftButtonDown;
        Canvas.SetLeft(handle, ((rect.X + rect.Width) * Zoom) - (handleSize / 2));
        Canvas.SetTop(handle, ((rect.Y + rect.Height) * Zoom) - (handleSize / 2));
        OverlayCanvas.Children.Add(handle);
    }

    private void UpdateSelectedFields()
    {
        _updatingFields = true;
        try
        {
            if (_selectedIcon is null)
            {
                CropXText.Text = "";
                CropYText.Text = "";
                CropWidthText.Text = "";
                CropHeightText.Text = "";
                SelectedInfoText.Text = "";
                return;
            }

            CropRect rect = _selectedIcon.SourceCropRect;
            CropXText.Text = IconCropRenderer.FormatNumber(rect.X);
            CropYText.Text = IconCropRenderer.FormatNumber(rect.Y);
            CropWidthText.Text = IconCropRenderer.FormatNumber(rect.Width);
            CropHeightText.Text = IconCropRenderer.FormatNumber(rect.Height);
            SelectedInfoText.Text = $"{_selectedIcon.Index}  row {_selectedIcon.Row}, col {_selectedIcon.Column}  {_selectedIcon.IconFilename}";
        }
        finally
        {
            _updatingFields = false;
        }
    }

    private bool TryReadCropFields(out CropRect rect)
    {
        rect = new CropRect();
        if (!TryParseNumber(CropXText.Text, out double x) ||
            !TryParseNumber(CropYText.Text, out double y) ||
            !TryParseNumber(CropWidthText.Text, out double width) ||
            !TryParseNumber(CropHeightText.Text, out double height))
        {
            return false;
        }

        rect.X = x;
        rect.Y = y;
        rect.Width = Math.Max(MinCropSize, width);
        rect.Height = Math.Max(MinCropSize, height);
        return true;
    }

    private CropRect ClampRectToSheet(CropRect rect)
    {
        double maxWidth = _sheetBitmap?.PixelWidth ?? _document?.SheetSize?.Width ?? Math.Max(rect.X + rect.Width, 1);
        double maxHeight = _sheetBitmap?.PixelHeight ?? _document?.SheetSize?.Height ?? Math.Max(rect.Y + rect.Height, 1);

        double width = Math.Clamp(rect.Width, MinCropSize, Math.Max(MinCropSize, maxWidth));
        double height = Math.Clamp(rect.Height, MinCropSize, Math.Max(MinCropSize, maxHeight));
        double x = Math.Clamp(rect.X, 0, Math.Max(0, maxWidth - width));
        double y = Math.Clamp(rect.Y, 0, Math.Max(0, maxHeight - height));

        return new CropRect
        {
            X = x,
            Y = y,
            Width = width,
            Height = height
        };
    }

    private string? GetCurrentDocumentDirectory()
    {
        return string.IsNullOrWhiteSpace(_jsonPath) ? null : Path.GetDirectoryName(_jsonPath);
    }

    private static bool TryParseNumber(string text, out double value)
    {
        return double.TryParse(text, NumberStyles.Float, CultureInfo.InvariantCulture, out value) ||
               double.TryParse(text, NumberStyles.Float, CultureInfo.CurrentCulture, out value);
    }

    private static string? FindDefaultDataDirectory()
    {
        var candidates = new List<string>
        {
            Directory.GetCurrentDirectory(),
            Path.Combine(Directory.GetCurrentDirectory(), "Tools", "IconCrop", "Icons"),
            Path.Combine(Directory.GetCurrentDirectory(), "Tools", "IconCrop"),
            AppContext.BaseDirectory
        };

        candidates.AddRange(EnumerateParents(AppContext.BaseDirectory).Select(static path => Path.Combine(path, "Tools", "IconCrop", "Icons")));
        candidates.AddRange(EnumerateParents(AppContext.BaseDirectory).Select(static path => Path.Combine(path, "Tools", "IconCrop")));
        candidates.AddRange(EnumerateParents(AppContext.BaseDirectory));
        candidates.AddRange(EnumerateParents(Directory.GetCurrentDirectory()).Select(static path => Path.Combine(path, "Tools", "IconCrop", "Icons")));
        candidates.AddRange(EnumerateParents(Directory.GetCurrentDirectory()).Select(static path => Path.Combine(path, "Tools", "IconCrop")));

        return candidates
            .Where(Directory.Exists)
            .Distinct(StringComparer.OrdinalIgnoreCase)
            .FirstOrDefault(static path => Directory.GetFiles(path, "*_cropInfo.json").Length > 0);
    }

    private static IEnumerable<string> EnumerateParents(string path)
    {
        DirectoryInfo? directory = new DirectoryInfo(path);
        while (directory is not null)
        {
            yield return directory.FullName;
            directory = directory.Parent;
        }
    }

    private sealed record OverlayTag(IconEntry Icon, DragMode Mode);

    private enum DragMode
    {
        Move,
        Resize
    }

    private enum AutoTrimMode
    {
        CenterClean = 1,
        EdgeRecover = 2,
        EdgeOff = 3
    }
}

public sealed class IconPreviewItem : INotifyPropertyChanged
{
    private static readonly Brush NormalBackground = CreateBrush("#1d232b");
    private static readonly Brush SelectedBackground = CreateBrush("#2d3a4a");
    private static readonly Brush NormalBorder = CreateBrush("#303846");
    private static readonly Brush SelectedBorder = CreateBrush("#ffb24a");

    private BitmapSource? _preview;
    private Brush _backgroundBrush = NormalBackground;
    private Brush _borderBrush = NormalBorder;

    public IconPreviewItem(IconEntry icon)
    {
        Icon = icon;
    }

    public event PropertyChangedEventHandler? PropertyChanged;

    public IconEntry Icon { get; }

    public string Label => $"{Icon.Index}: {Icon.IconFilename}";

    public BitmapSource? Preview
    {
        get => _preview;
        set
        {
            _preview = value;
            OnPropertyChanged();
        }
    }

    public Brush BackgroundBrush
    {
        get => _backgroundBrush;
        private set
        {
            _backgroundBrush = value;
            OnPropertyChanged();
        }
    }

    public Brush BorderBrush
    {
        get => _borderBrush;
        private set
        {
            _borderBrush = value;
            OnPropertyChanged();
        }
    }

    public void SetSelected(bool selected)
    {
        BackgroundBrush = selected ? SelectedBackground : NormalBackground;
        BorderBrush = selected ? SelectedBorder : NormalBorder;
    }

    private static Brush CreateBrush(string hex)
    {
        var brush = (SolidColorBrush)new BrushConverter().ConvertFromString(hex)!;
        brush.Freeze();
        return brush;
    }

    private void OnPropertyChanged([CallerMemberName] string? propertyName = null)
    {
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
    }
}
