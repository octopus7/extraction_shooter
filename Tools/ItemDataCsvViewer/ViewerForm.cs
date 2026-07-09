using System.Data;
using System.Diagnostics;

namespace ItemDataCsvViewer;

internal sealed class ViewerForm : Form
{
	private readonly string projectRoot;
	private readonly TextBox searchTextBox = new();
	private readonly Label statusLabel = new();
	private readonly TextBox detailsTextBox = new();
	private readonly TabControl tabControl = new();
	private readonly RelationGraphControl graphControl = new();
	private readonly Dictionary<string, DataGridView> gridsByTab = new(StringComparer.OrdinalIgnoreCase);
	private CsvRelationDataStore? currentStore;
	private IReadOnlyList<Dictionary<string, string>> itemRows = [];
	private IReadOnlyList<Dictionary<string, string>> shopRows = [];
	private IReadOnlyList<Dictionary<string, string>> lootRows = [];
	private IReadOnlyList<Dictionary<string, string>> recipeRows = [];
	private IReadOnlyList<Dictionary<string, string>> dismantleRows = [];

	public ViewerForm(string projectRoot)
	{
		this.projectRoot = projectRoot;
		Text = "TunaSweeper Item Data CSV Viewer";
		ClientSize = new Size(1500, 860);
		MinimumSize = new Size(1100, 680);
		StartPosition = FormStartPosition.CenterScreen;

		InitializeLayout();
		EnsureCsvAndLoad();
	}

	private void InitializeLayout()
	{
		TableLayoutPanel root = new()
		{
			Dock = DockStyle.Fill,
			ColumnCount = 1,
			RowCount = 3,
			Padding = new Padding(8)
		};
		root.RowStyles.Add(new RowStyle(SizeType.Absolute, 40));
		root.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
		root.RowStyles.Add(new RowStyle(SizeType.Absolute, 24));
		Controls.Add(root);

		FlowLayoutPanel toolbar = new()
		{
			Dock = DockStyle.Fill,
			FlowDirection = FlowDirection.LeftToRight,
			WrapContents = false
		};
		root.Controls.Add(toolbar, 0, 0);

		Button regenerateButton = new()
		{
			Text = "Regenerate CSV",
			Width = 130,
			Height = 28
		};
		regenerateButton.Click += (_, _) => RegenerateAndReload();
		toolbar.Controls.Add(regenerateButton);

		Button reloadButton = new()
		{
			Text = "Reload",
			Width = 82,
			Height = 28
		};
		reloadButton.Click += (_, _) => LoadCsv();
		toolbar.Controls.Add(reloadButton);

		Button openExportButton = new()
		{
			Text = "Open Export Folder",
			Width = 140,
			Height = 28
		};
		openExportButton.Click += (_, _) => OpenFolder(ProjectPaths.ExportDirectory(projectRoot));
		toolbar.Controls.Add(openExportButton);

		Label searchLabel = new()
		{
			Text = "Search",
			AutoSize = true,
			TextAlign = ContentAlignment.MiddleLeft,
			Margin = new Padding(16, 7, 4, 0)
		};
		toolbar.Controls.Add(searchLabel);

		searchTextBox.Width = 280;
		searchTextBox.Height = 28;
		searchTextBox.TextChanged += (_, _) => ApplyFilter();
		toolbar.Controls.Add(searchTextBox);

		Label validationLegendLabel = new()
		{
			Text = "ERROR red / WARN amber",
			AutoSize = true,
			TextAlign = ContentAlignment.MiddleLeft,
			Margin = new Padding(16, 7, 4, 0)
		};
		toolbar.Controls.Add(validationLegendLabel);

		SplitContainer splitContainer = new()
		{
			Dock = DockStyle.Fill,
			Orientation = Orientation.Vertical,
			SplitterDistance = 1060
		};
		root.Controls.Add(splitContainer, 0, 1);

		tabControl.Dock = DockStyle.Fill;
		splitContainer.Panel1.Controls.Add(tabControl);

		detailsTextBox.Dock = DockStyle.Fill;
		detailsTextBox.Multiline = true;
		detailsTextBox.ReadOnly = true;
		detailsTextBox.ScrollBars = ScrollBars.Both;
		detailsTextBox.WordWrap = false;
		detailsTextBox.Font = new Font(FontFamily.GenericMonospace, 9.0f);
		splitContainer.Panel2.Controls.Add(detailsTextBox);

		statusLabel.Dock = DockStyle.Fill;
		statusLabel.TextAlign = ContentAlignment.MiddleLeft;
		root.Controls.Add(statusLabel, 0, 2);

		AddGridTab("Items");
		AddGraphTab();
		AddGridTab("Shops");
		AddGridTab("Loot");
		AddGridTab("Recipes");
		AddGridTab("Dismantle");
	}

	private void AddGridTab(string tabName)
	{
		TabPage tabPage = new(tabName);
		DataGridView grid = new()
		{
			Dock = DockStyle.Fill,
			ReadOnly = true,
			AllowUserToAddRows = false,
			AllowUserToDeleteRows = false,
			SelectionMode = DataGridViewSelectionMode.FullRowSelect,
			MultiSelect = false,
			AutoSizeColumnsMode = DataGridViewAutoSizeColumnsMode.DisplayedCells,
			AutoSizeRowsMode = DataGridViewAutoSizeRowsMode.None,
			RowHeadersVisible = false
		};
		grid.SelectionChanged += (_, _) => UpdateDetails(tabName, grid);
		grid.DataBindingComplete += (_, _) => ApplyValidationHighlighting(grid);
		tabPage.Controls.Add(grid);
		tabControl.TabPages.Add(tabPage);
		gridsByTab[tabName] = grid;
	}

	private void AddGraphTab()
	{
		TabPage tabPage = new("Relation Graph");
		graphControl.Dock = DockStyle.Fill;
		tabPage.Controls.Add(graphControl);
		tabControl.TabPages.Add(tabPage);
	}

	private void EnsureCsvAndLoad()
	{
		string itemDefinitionsPath = Path.Combine(ProjectPaths.ExportDirectory(projectRoot), "item_definitions.csv");
		if (!File.Exists(itemDefinitionsPath))
		{
			ItemDataCsvExporter.Export(projectRoot);
		}

		LoadCsv();
	}

	private void RegenerateAndReload()
	{
		try
		{
			UseWaitCursor = true;
			ItemDataCsvExportResult result = ItemDataCsvExporter.Export(projectRoot);
			LoadCsv();
			statusLabel.Text = $"Regenerated {result.WrittenFiles.Count} CSV files. Export: {result.ExportDirectory}";
			if (result.Warnings.Count > 0)
			{
				statusLabel.Text += $" Warnings: {result.Warnings.Count}";
			}
		}
		catch (Exception exception) when (exception is IOException or FormatException or InvalidOperationException)
		{
			MessageBox.Show(exception.Message, Text, MessageBoxButtons.OK, MessageBoxIcon.Error);
		}
		finally
		{
			UseWaitCursor = false;
		}
	}

	private void LoadCsv()
	{
		try
		{
			currentStore = CsvRelationDataStore.Load(projectRoot);
			itemRows = currentStore.ItemRows;
			shopRows = currentStore.ShopRows;
			lootRows = currentStore.LootRows;
			recipeRows = currentStore.RecipeRows;
			dismantleRows = currentStore.DismantleRows;
			ApplyFilter();
			statusLabel.Text =
				$"Loaded CSV from {ProjectPaths.ExportDirectory(projectRoot)}. " +
				$"Validation: {currentStore.ValidationErrorCount} error row(s), {currentStore.ValidationWarningCount} warning row(s).";
		}
		catch (Exception exception) when (exception is IOException or FormatException or InvalidOperationException)
		{
			MessageBox.Show(exception.Message, Text, MessageBoxButtons.OK, MessageBoxIcon.Error);
		}
	}

	private void ApplyFilter()
	{
		string filter = searchTextBox.Text.Trim();
		SetGrid("Items", FilterRows(itemRows, filter));
		SetGrid("Shops", FilterRows(shopRows, filter));
		SetGrid("Loot", FilterRows(lootRows, filter));
		SetGrid("Recipes", FilterRows(recipeRows, filter));
		SetGrid("Dismantle", FilterRows(dismantleRows, filter));
	}

	private static IEnumerable<Dictionary<string, string>> FilterRows(IEnumerable<Dictionary<string, string>> rows, string filter)
	{
		if (string.IsNullOrWhiteSpace(filter))
		{
			return rows;
		}

		return rows.Where(row => row.Values.Any(value => value.Contains(filter, StringComparison.OrdinalIgnoreCase)));
	}

	private void SetGrid(string name, IEnumerable<Dictionary<string, string>> rows)
	{
		if (!gridsByTab.TryGetValue(name, out DataGridView? grid))
		{
			return;
		}

		DataTable table = CsvRelationDataStore.ToDataTable(rows);
		grid.DataSource = table;
		ApplyValidationHighlighting(grid);
	}

	private static void ApplyValidationHighlighting(DataGridView grid)
	{
		if (!grid.Columns.Contains("validation_severity"))
		{
			return;
		}

		foreach (DataGridViewRow row in grid.Rows)
		{
			string severity = Convert.ToString(row.Cells["validation_severity"].Value, System.Globalization.CultureInfo.InvariantCulture) ?? string.Empty;
			if (string.Equals(severity, "ERROR", StringComparison.OrdinalIgnoreCase))
			{
				row.DefaultCellStyle.BackColor = Color.FromArgb(94, 38, 42);
				row.DefaultCellStyle.ForeColor = Color.FromArgb(255, 242, 242);
				row.DefaultCellStyle.SelectionBackColor = Color.FromArgb(130, 48, 54);
				row.DefaultCellStyle.SelectionForeColor = Color.White;
			}
			else if (string.Equals(severity, "WARN", StringComparison.OrdinalIgnoreCase))
			{
				row.DefaultCellStyle.BackColor = Color.FromArgb(92, 76, 38);
				row.DefaultCellStyle.ForeColor = Color.FromArgb(255, 249, 230);
				row.DefaultCellStyle.SelectionBackColor = Color.FromArgb(130, 102, 42);
				row.DefaultCellStyle.SelectionForeColor = Color.White;
			}
			else
			{
				row.DefaultCellStyle.BackColor = grid.DefaultCellStyle.BackColor;
				row.DefaultCellStyle.ForeColor = grid.DefaultCellStyle.ForeColor;
				row.DefaultCellStyle.SelectionBackColor = grid.DefaultCellStyle.SelectionBackColor;
				row.DefaultCellStyle.SelectionForeColor = grid.DefaultCellStyle.SelectionForeColor;
			}
		}

	}

	private void UpdateDetails(string tabName, DataGridView grid)
	{
		if (grid.CurrentRow is null || grid.CurrentRow.DataBoundItem is not DataRowView rowView)
		{
			detailsTextBox.Clear();
			if (string.Equals(tabName, "Items", StringComparison.OrdinalIgnoreCase))
			{
				graphControl.SetGraph(RelationGraph.Empty);
			}
			return;
		}

		List<string> lines = [];
		foreach (DataColumn column in rowView.Row.Table.Columns)
		{
			string value = Convert.ToString(rowView.Row[column], System.Globalization.CultureInfo.InvariantCulture) ?? string.Empty;
			if (!string.IsNullOrWhiteSpace(value))
			{
				lines.Add($"{column.ColumnName}: {value}");
			}
		}

		detailsTextBox.Text = string.Join(Environment.NewLine, lines);
		if (string.Equals(tabName, "Items", StringComparison.OrdinalIgnoreCase) &&
			currentStore is not null &&
			rowView.Row.Table.Columns.Contains("id"))
		{
			string itemId = Convert.ToString(rowView.Row["id"], System.Globalization.CultureInfo.InvariantCulture) ?? string.Empty;
			graphControl.SetGraph(currentStore.BuildItemGraph(itemId));
		}
	}

	private static void OpenFolder(string folder)
	{
		Directory.CreateDirectory(folder);
		ProcessStartInfo startInfo = new()
		{
			FileName = folder,
			UseShellExecute = true
		};
		Process.Start(startInfo);
	}
}
