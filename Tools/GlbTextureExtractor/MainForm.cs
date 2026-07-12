namespace GlbTextureExtractor;

internal sealed class MainForm : Form
{
    private readonly TextBox _inputPathTextBox = new() { Dock = DockStyle.Fill };
    private readonly TextBox _outputPathTextBox = new() { Dock = DockStyle.Fill };
    private readonly CheckBox _writeRenamedGlbCheckBox = new()
    {
        Text = "M_/SM_ 이름 적용 및 외부 텍스처 연결 GLB 저장",
        Checked = true,
        AutoSize = true
    };
    private readonly ComboBox _baseColorResolutionComboBox = new()
    {
        DropDownStyle = ComboBoxStyle.DropDownList,
        Dock = DockStyle.Left,
        Width = 180
    };
    private readonly ComboBox _otherTextureResolutionComboBox = new()
    {
        DropDownStyle = ComboBoxStyle.DropDownList,
        Dock = DockStyle.Left,
        Width = 180
    };
    private readonly Button _runButton = new() { Text = "언팩 실행", AutoSize = true };
    private readonly TextBox _logTextBox = new()
    {
        Dock = DockStyle.Fill,
        Multiline = true,
        ReadOnly = true,
        ScrollBars = ScrollBars.Vertical,
        BackColor = SystemColors.Window
    };

    public MainForm()
    {
        Text = "GLB Texture Extractor";
        MinimumSize = new Size(760, 520);
        StartPosition = FormStartPosition.CenterScreen;

        var layout = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            Padding = new Padding(12),
            ColumnCount = 3,
            RowCount = 8
        };
        layout.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
        layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        layout.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
        layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        layout.RowStyles.Add(new RowStyle(SizeType.Percent, 100));

        var inputButton = new Button { Text = "GLB 선택", AutoSize = true };
        inputButton.Click += (_, _) => SelectInputFile();
        var outputButton = new Button { Text = "폴더 선택", AutoSize = true };
        outputButton.Click += (_, _) => SelectOutputFolder();
        _runButton.Click += async (_, _) => await RunExportAsync();
        AddResolutionOptions(_baseColorResolutionComboBox, 1024, include2048: true);
        AddResolutionOptions(_otherTextureResolutionComboBox, 512, include2048: false);

        layout.Controls.Add(new Label { Text = "입력 GLB", AutoSize = true, Anchor = AnchorStyles.Left }, 0, 0);
        layout.Controls.Add(_inputPathTextBox, 1, 0);
        layout.Controls.Add(inputButton, 2, 0);
        layout.Controls.Add(new Label { Text = "출력 폴더", AutoSize = true, Anchor = AnchorStyles.Left }, 0, 1);
        layout.Controls.Add(_outputPathTextBox, 1, 1);
        layout.Controls.Add(outputButton, 2, 1);

        layout.Controls.Add(new Label { Text = "Base Color", AutoSize = true, Anchor = AnchorStyles.Left }, 0, 2);
        layout.Controls.Add(_baseColorResolutionComboBox, 1, 2);
        layout.Controls.Add(new Label { Text = "긴 변 목표 해상도", AutoSize = true, Anchor = AnchorStyles.Left }, 2, 2);

        layout.Controls.Add(new Label { Text = "기타 텍스처", AutoSize = true, Anchor = AnchorStyles.Left }, 0, 3);
        layout.Controls.Add(_otherTextureResolutionComboBox, 1, 3);
        layout.Controls.Add(new Label { Text = "긴 변 목표 해상도", AutoSize = true, Anchor = AnchorStyles.Left }, 2, 3);

        layout.Controls.Add(new Label { Text = "안내", AutoSize = true, Anchor = AnchorStyles.Left }, 0, 4);
        layout.Controls.Add(new Label
        {
            Text = "권장값이 기본 선택됩니다. 작은 원본 텍스처는 확대하지 않고 원본 크기를 유지합니다.",
            AutoSize = true,
            Anchor = AnchorStyles.Left
        }, 1, 4);
        layout.SetColumnSpan(layout.GetControlFromPosition(1, 4)!, 2);

        layout.Controls.Add(new Label { Text = "옵션", AutoSize = true, Anchor = AnchorStyles.Left }, 0, 5);
        layout.Controls.Add(_writeRenamedGlbCheckBox, 1, 5);
        layout.SetColumnSpan(_writeRenamedGlbCheckBox, 2);

        layout.Controls.Add(_runButton, 2, 6);
        layout.Controls.Add(_logTextBox, 0, 7);
        layout.SetColumnSpan(_logTextBox, 3);
        Controls.Add(layout);
    }

    private void SelectInputFile()
    {
        using var dialog = new OpenFileDialog
        {
            Filter = "Binary glTF (*.glb)|*.glb",
            CheckFileExists = true,
            Multiselect = false
        };

        if (dialog.ShowDialog(this) != DialogResult.OK)
        {
            return;
        }

        _inputPathTextBox.Text = dialog.FileName;
        _outputPathTextBox.Text = Path.GetDirectoryName(dialog.FileName)!;
    }

    private void SelectOutputFolder()
    {
        using var dialog = new FolderBrowserDialog
        {
            Description = "언팩 결과를 저장할 폴더를 선택하세요."
        };

        if (dialog.ShowDialog(this) == DialogResult.OK)
        {
            _outputPathTextBox.Text = dialog.SelectedPath;
        }
    }

    private async Task RunExportAsync()
    {
        var inputPath = _inputPathTextBox.Text.Trim();
        var outputPath = _outputPathTextBox.Text.Trim();
        if (!File.Exists(inputPath))
        {
            MessageBox.Show(this, "유효한 입력 GLB 파일을 선택하세요.", Text, MessageBoxButtons.OK, MessageBoxIcon.Warning);
            return;
        }

        if (string.IsNullOrWhiteSpace(outputPath))
        {
            MessageBox.Show(this, "출력 폴더를 선택하세요.", Text, MessageBoxButtons.OK, MessageBoxIcon.Warning);
            return;
        }

        _runButton.Enabled = false;
        _logTextBox.Clear();
        AppendLog($"입력: {inputPath}");
        try
        {
            var options = new ExportOptions(
                InputPath: inputPath,
                OutputDirectory: outputPath,
                WriteRenamedGlbCopy: _writeRenamedGlbCheckBox.Checked,
                BaseColorTargetLongestEdge: GetSelectedResolution(_baseColorResolutionComboBox),
                OtherTextureTargetLongestEdge: GetSelectedResolution(_otherTextureResolutionComboBox));

            var result = await Task.Run(() => new GlbTextureExportService().Export(options));
            foreach (var message in result.Messages)
            {
                AppendLog(message);
            }

            AppendLog($"완료: {result.ExportedTextureCount}개 텍스처를 추출했습니다.");
            if (result.SkippedTextureCount > 0)
            {
                AppendLog($"건너뜀: {result.SkippedTextureCount}개 (manifest를 확인하세요.)");
            }
        }
        catch (Exception exception)
        {
            AppendLog($"실패: {exception.Message}");
            MessageBox.Show(this, exception.Message, "언팩 실패", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
        finally
        {
            _runButton.Enabled = true;
        }
    }

    private void AppendLog(string message)
    {
        _logTextBox.AppendText($"[{DateTime.Now:HH:mm:ss}] {message}{Environment.NewLine}");
    }

    private static void AddResolutionOptions(ComboBox comboBox, int recommendedPixels, bool include2048)
    {
        var resolutions = include2048 ? new[] { 2048, 1024, 512, 256, 128, 64 } : new[] { 1024, 512, 256, 128, 64 };
        foreach (var resolution in resolutions)
        {
            comboBox.Items.Add(new ResolutionOption(resolution, resolution == recommendedPixels));
        }

        comboBox.SelectedItem = comboBox.Items.OfType<ResolutionOption>().Single(option => option.Pixels == recommendedPixels);
    }

    private static int GetSelectedResolution(ComboBox comboBox) => ((ResolutionOption)comboBox.SelectedItem!).Pixels;

    private sealed record ResolutionOption(int Pixels, bool IsRecommended)
    {
        public override string ToString() => IsRecommended ? $"{Pixels}px (권장)" : $"{Pixels}px";
    }
}
