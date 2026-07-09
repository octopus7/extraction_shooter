namespace ItemDataCsvViewer;

internal static class Program
{
	[STAThread]
	private static int Main(string[] args)
	{
		try
		{
			string projectRoot = ProjectPaths.ResolveProjectRoot(GetOptionValue(args, "--project-root"));
			if (args.Any(argument => string.Equals(argument, "--export-only", StringComparison.OrdinalIgnoreCase)))
			{
				ItemDataCsvExporter.Export(projectRoot);
				return 0;
			}
			if (args.Any(argument => string.Equals(argument, "--check", StringComparison.OrdinalIgnoreCase)))
			{
				VerifyExportAndViewerData(projectRoot);
				return 0;
			}

			ApplicationConfiguration.Initialize();
			Application.Run(new ViewerForm(projectRoot));
			return 0;
		}
		catch (Exception exception) when (exception is IOException or InvalidOperationException or FormatException)
		{
			MessageBox.Show(exception.Message, "Item Data CSV Viewer", MessageBoxButtons.OK, MessageBoxIcon.Error);
			return 1;
		}
	}

	private static string? GetOptionValue(IReadOnlyList<string> args, string optionName)
	{
		for (int index = 0; index < args.Count - 1; ++index)
		{
			if (string.Equals(args[index], optionName, StringComparison.OrdinalIgnoreCase))
			{
				return args[index + 1];
			}
		}

		return null;
	}

	private static void VerifyExportAndViewerData(string projectRoot)
	{
		ItemDataCsvExporter.Export(projectRoot);
		CsvRelationDataStore store = CsvRelationDataStore.Load(projectRoot);
		Dictionary<string, string> firstItem = store.ItemRows.FirstOrDefault()
			?? throw new InvalidOperationException("No item rows were loaded from generated CSV.");
		if (!firstItem.TryGetValue("id", out string? itemId) || string.IsNullOrWhiteSpace(itemId))
		{
			throw new InvalidOperationException("The first item row has no id.");
		}

		RelationGraph graph = store.BuildItemGraph(itemId);
		if (graph.Nodes.Count <= 0)
		{
			throw new InvalidOperationException($"Relation graph was empty for item {itemId}.");
		}
	}
}
