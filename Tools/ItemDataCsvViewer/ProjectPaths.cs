namespace ItemDataCsvViewer;

internal static class ProjectPaths
{
	public static string ResolveProjectRoot(string? explicitRoot)
	{
		if (!string.IsNullOrWhiteSpace(explicitRoot))
		{
			string fullPath = Path.GetFullPath(explicitRoot);
			if (!IsProjectRoot(fullPath))
			{
				throw new InvalidOperationException($"Project root does not look like extraction_shooter: {fullPath}");
			}

			return fullPath;
		}

		foreach (string candidate in EnumerateRootCandidates(Directory.GetCurrentDirectory())
			.Concat(EnumerateRootCandidates(AppContext.BaseDirectory)))
		{
			if (IsProjectRoot(candidate))
			{
				return candidate;
			}
		}

		throw new InvalidOperationException("Could not locate project root. Pass --project-root <path>.");
	}

	public static string DataDirectory(string projectRoot)
	{
		return Path.Combine(projectRoot, "TunaSweeper", "Content", "Data");
	}

	public static string ExportDirectory(string projectRoot)
	{
		return Path.Combine(DataDirectory(projectRoot), "ExcelExport");
	}

	public static string RelationDocumentPath(string projectRoot)
	{
		return Path.Combine(projectRoot, "Docs", "item_json_excel_relations.md");
	}

	private static IEnumerable<string> EnumerateRootCandidates(string startPath)
	{
		DirectoryInfo? directory = new(Path.GetFullPath(startPath));
		while (directory is not null)
		{
			yield return directory.FullName;
			directory = directory.Parent;
		}
	}

	private static bool IsProjectRoot(string path)
	{
		return File.Exists(Path.Combine(path, "AGENTS.md")) &&
			File.Exists(Path.Combine(path, "TunaSweeper", "TunaSweeper.uproject"));
	}
}
