using System.Text;

namespace ItemDataCsvViewer;

internal sealed class CsvTable
{
	public CsvTable(string name, IReadOnlyList<string> columns, IReadOnlyList<Dictionary<string, string>> rows)
	{
		Name = name;
		Columns = columns;
		Rows = rows;
	}

	public string Name { get; }

	public IReadOnlyList<string> Columns { get; }

	public IReadOnlyList<Dictionary<string, string>> Rows { get; }
}

internal static class Csv
{
	public static void Write(string path, IReadOnlyList<string> columns, IEnumerable<IReadOnlyDictionary<string, string>> rows)
	{
		Directory.CreateDirectory(Path.GetDirectoryName(path) ?? ".");
		StringBuilder builder = new();
		builder.AppendLine(string.Join(",", columns.Select(Escape)));
		foreach (IReadOnlyDictionary<string, string> row in rows)
		{
			builder.AppendLine(string.Join(",", columns.Select(column =>
			{
				return row.TryGetValue(column, out string? value) ? Escape(value) : string.Empty;
			})));
		}

		File.WriteAllText(path, builder.ToString(), new UTF8Encoding(encoderShouldEmitUTF8Identifier: true));
	}

	public static CsvTable Read(string path, string name)
	{
		if (!File.Exists(path))
		{
			return new CsvTable(name, Array.Empty<string>(), Array.Empty<Dictionary<string, string>>());
		}

		string[] lines = File.ReadAllLines(path, Encoding.UTF8);
		if (lines.Length <= 0)
		{
			return new CsvTable(name, Array.Empty<string>(), Array.Empty<Dictionary<string, string>>());
		}

		List<string> columns = ParseLine(lines[0]);
		if (columns.Count > 0)
		{
			columns[0] = columns[0].TrimStart('\uFEFF');
		}

		List<Dictionary<string, string>> rows = [];
		for (int lineIndex = 1; lineIndex < lines.Length; ++lineIndex)
		{
			if (string.IsNullOrWhiteSpace(lines[lineIndex]))
			{
				continue;
			}

			List<string> values = ParseLine(lines[lineIndex]);
			Dictionary<string, string> row = new(StringComparer.OrdinalIgnoreCase);
			for (int columnIndex = 0; columnIndex < columns.Count; ++columnIndex)
			{
				row[columns[columnIndex]] = columnIndex < values.Count ? values[columnIndex] : string.Empty;
			}

			rows.Add(row);
		}

		return new CsvTable(name, columns, rows);
	}

	private static string Escape(string? value)
	{
		if (string.IsNullOrEmpty(value))
		{
			return string.Empty;
		}

		bool needsQuoting = value.Contains(',') || value.Contains('"') || value.Contains('\r') || value.Contains('\n');
		string escaped = value.Replace("\"", "\"\"", StringComparison.Ordinal);
		return needsQuoting ? $"\"{escaped}\"" : escaped;
	}

	private static List<string> ParseLine(string line)
	{
		List<string> values = [];
		StringBuilder current = new();
		bool inQuotes = false;
		for (int index = 0; index < line.Length; ++index)
		{
			char character = line[index];
			if (inQuotes)
			{
				if (character == '"')
				{
					if (index + 1 < line.Length && line[index + 1] == '"')
					{
						current.Append('"');
						++index;
					}
					else
					{
						inQuotes = false;
					}
				}
				else
				{
					current.Append(character);
				}
			}
			else if (character == ',')
			{
				values.Add(current.ToString());
				current.Clear();
			}
			else if (character == '"')
			{
				inQuotes = true;
			}
			else
			{
				current.Append(character);
			}
		}

		values.Add(current.ToString());
		return values;
	}
}
