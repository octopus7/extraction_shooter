using System.Text;
using System.Text.Json;

namespace QuestGraphGenerator;

public static class QuestGraphGeneratorApp
{
    private static readonly JsonSerializerOptions JsonOptions = new()
    {
        AllowTrailingCommas = true,
        PropertyNameCaseInsensitive = true,
        ReadCommentHandling = JsonCommentHandling.Skip
    };

    public static int Run(string[] args)
    {
        CommandLineOptions options;
        try
        {
            options = CommandLineOptions.Parse(args);
        }
        catch (ArgumentException exception)
        {
            Console.Error.WriteLine(exception.Message);
            Console.Error.WriteLine(CommandLineOptions.Usage);
            return 2;
        }

        if (options.ShowHelp)
        {
            Console.WriteLine(CommandLineOptions.Usage);
            return 0;
        }

        try
        {
            string projectRoot = ResolveProjectRoot(options.ProjectRoot);
            string questJsonPath = ResolvePath(
                options.QuestJsonPath,
                projectRoot,
                Path.Combine("TunaSweeper", "Content", "Data", "QuestDefinitions.json"));
            string questTextCsvPath = ResolvePath(
                options.QuestTextCsvPath,
                projectRoot,
                Path.Combine("TunaSweeper", "Content", "Data", "QuestTextStrings.csv"));
            string outputPath = ResolvePath(
                options.OutputPath,
                projectRoot,
                Path.Combine("Docs", "quest_graph.md"));

            IReadOnlyList<QuestDefinition> quests = LoadQuestDefinitions(questJsonPath);
            IReadOnlyDictionary<string, QuestTextString> textStrings = LoadQuestTextStrings(questTextCsvPath);
            IReadOnlyList<ValidationMessage> validationMessages = ValidateQuests(quests, textStrings);

            string report = BuildReport(
                quests,
                textStrings,
                validationMessages,
                projectRoot,
                questJsonPath,
                questTextCsvPath,
                options.Language);

            string? outputDirectory = Path.GetDirectoryName(outputPath);
            if (!string.IsNullOrWhiteSpace(outputDirectory))
            {
                Directory.CreateDirectory(outputDirectory);
            }

            File.WriteAllText(outputPath, report, new UTF8Encoding(encoderShouldEmitUTF8Identifier: false));

            int errorCount = validationMessages.Count(message => message.Severity == "ERROR");
            int warningCount = validationMessages.Count(message => message.Severity == "WARN");
            Console.WriteLine($"Loaded quests: {quests.Count}");
            Console.WriteLine($"Validation: {errorCount} error(s), {warningCount} warning(s)");
            Console.WriteLine($"Wrote: {outputPath}");

            return errorCount > 0 || (options.FailOnWarning && warningCount > 0) ? 1 : 0;
        }
        catch (Exception exception) when (exception is IOException or JsonException or InvalidOperationException)
        {
            Console.Error.WriteLine(exception.Message);
            return 1;
        }
    }

    private static string ResolveProjectRoot(string? explicitRoot)
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
        return File.Exists(Path.Combine(path, "TunaSweeper", "TunaSweeper.uproject")) &&
            Directory.Exists(Path.Combine(path, "Docs")) &&
            Directory.Exists(Path.Combine(path, "Tools"));
    }

    private static string ResolvePath(string? explicitPath, string projectRoot, string defaultRelativePath)
    {
        string path = string.IsNullOrWhiteSpace(explicitPath)
            ? Path.Combine(projectRoot, defaultRelativePath)
            : explicitPath;
        return Path.GetFullPath(Path.IsPathRooted(path) ? path : Path.Combine(projectRoot, path));
    }

    private static IReadOnlyList<QuestDefinition> LoadQuestDefinitions(string questJsonPath)
    {
        if (!File.Exists(questJsonPath))
        {
            throw new IOException($"Quest definition JSON not found: {questJsonPath}");
        }

        string json = File.ReadAllText(questJsonPath, Encoding.UTF8);
        List<QuestDefinition>? quests = JsonSerializer.Deserialize<List<QuestDefinition>>(json, JsonOptions);
        if (quests is null)
        {
            throw new JsonException($"Quest definition JSON is empty or invalid: {questJsonPath}");
        }

        return quests;
    }

    private static IReadOnlyDictionary<string, QuestTextString> LoadQuestTextStrings(string questTextCsvPath)
    {
        Dictionary<string, QuestTextString> result = new(StringComparer.Ordinal);
        if (!File.Exists(questTextCsvPath))
        {
            return result;
        }

        string csv = File.ReadAllText(questTextCsvPath, Encoding.UTF8);
        List<string[]> rows = ParseCsv(csv);
        if (rows.Count <= 1)
        {
            return result;
        }

        for (int rowIndex = 1; rowIndex < rows.Count; rowIndex++)
        {
            string[] row = rows[rowIndex];
            if (row.Length < 4 || string.IsNullOrWhiteSpace(row[0]))
            {
                continue;
            }

            result[row[0].Trim()] = new QuestTextString
            {
                StringKey = row[0].Trim(),
                Korean = row[1].Trim(),
                English = row[2].Trim(),
                Japanese = row[3].Trim()
            };
        }

        return result;
    }

    private static List<string[]> ParseCsv(string csv)
    {
        List<string[]> rows = [];
        List<string> row = [];
        StringBuilder cell = new();
        bool inQuotes = false;

        for (int index = 0; index < csv.Length; index++)
        {
            char current = csv[index];
            if (inQuotes)
            {
                if (current == '"')
                {
                    if (index + 1 < csv.Length && csv[index + 1] == '"')
                    {
                        cell.Append('"');
                        index++;
                    }
                    else
                    {
                        inQuotes = false;
                    }
                }
                else
                {
                    cell.Append(current);
                }

                continue;
            }

            switch (current)
            {
                case '"':
                    inQuotes = true;
                    break;
                case ',':
                    row.Add(cell.ToString());
                    cell.Clear();
                    break;
                case '\r':
                    break;
                case '\n':
                    row.Add(cell.ToString());
                    cell.Clear();
                    rows.Add(row.ToArray());
                    row.Clear();
                    break;
                default:
                    cell.Append(current);
                    break;
            }
        }

        if (cell.Length > 0 || row.Count > 0)
        {
            row.Add(cell.ToString());
            rows.Add(row.ToArray());
        }

        return rows;
    }

    private static IReadOnlyList<ValidationMessage> ValidateQuests(
        IReadOnlyList<QuestDefinition> quests,
        IReadOnlyDictionary<string, QuestTextString> textStrings)
    {
        List<ValidationMessage> messages = [];
        Dictionary<string, List<QuestDefinition>> questsById = quests
            .Where(quest => !string.IsNullOrWhiteSpace(quest.QuestId))
            .GroupBy(quest => quest.QuestId.Trim(), StringComparer.Ordinal)
            .ToDictionary(group => group.Key, group => group.ToList(), StringComparer.Ordinal);

        for (int index = 0; index < quests.Count; index++)
        {
            QuestDefinition quest = quests[index];
            if (string.IsNullOrWhiteSpace(quest.QuestId))
            {
                messages.Add(new ValidationMessage("ERROR", "QUEST_ID_EMPTY", $"Quest entry #{index + 1} has no quest_id."));
                continue;
            }

            if (string.IsNullOrWhiteSpace(quest.TitleStringKey))
            {
                messages.Add(new ValidationMessage("WARN", "TITLE_KEY_EMPTY", $"{quest.QuestId} has no title_string_key."));
            }
            else if (textStrings.Count > 0 && !textStrings.ContainsKey(quest.TitleStringKey.Trim()))
            {
                messages.Add(new ValidationMessage("WARN", "TITLE_KEY_MISSING", $"{quest.QuestId} title key is missing from QuestTextStrings.csv: {quest.TitleStringKey}"));
            }

            if (quest.Objectives.Count == 0)
            {
                messages.Add(new ValidationMessage("ERROR", "OBJECTIVES_EMPTY", $"{quest.QuestId} has no objectives."));
            }

            foreach (QuestObjective objective in quest.Objectives)
            {
                if (string.IsNullOrWhiteSpace(objective.ObjectiveId))
                {
                    messages.Add(new ValidationMessage("ERROR", "OBJECTIVE_ID_EMPTY", $"{quest.QuestId} has an objective with no objective_id."));
                }

                if (string.IsNullOrWhiteSpace(objective.Type))
                {
                    messages.Add(new ValidationMessage("ERROR", "OBJECTIVE_TYPE_EMPTY", $"{quest.QuestId}.{objective.ObjectiveId} has no objective type."));
                }

                if (!string.IsNullOrWhiteSpace(objective.TextStringKey) &&
                    textStrings.Count > 0 &&
                    !textStrings.ContainsKey(objective.TextStringKey.Trim()))
                {
                    messages.Add(new ValidationMessage("WARN", "OBJECTIVE_TEXT_KEY_MISSING", $"{quest.QuestId}.{objective.ObjectiveId} text key is missing from QuestTextStrings.csv: {objective.TextStringKey}"));
                }
            }

            if (quest.Rewards is null)
            {
                messages.Add(new ValidationMessage("WARN", "REWARDS_EMPTY", $"{quest.QuestId} has no rewards object."));
            }

            foreach (string prerequisite in quest.Prerequisites)
            {
                if (!questsById.ContainsKey(prerequisite))
                {
                    messages.Add(new ValidationMessage("ERROR", "PREREQUISITE_MISSING", $"{quest.QuestId} requires missing quest: {prerequisite}"));
                }
            }
        }

        foreach ((string questId, List<QuestDefinition> duplicates) in questsById.Where(pair => pair.Value.Count > 1))
        {
            messages.Add(new ValidationMessage("ERROR", "QUEST_ID_DUPLICATE", $"{questId} appears {duplicates.Count} times."));
        }

        foreach (IGrouping<string, QuestDefinition> providerGroup in quests
            .Where(quest => !string.IsNullOrWhiteSpace(quest.ProviderId))
            .GroupBy(quest => quest.ProviderId!.Trim(), StringComparer.Ordinal))
        {
            foreach (IGrouping<int, QuestDefinition> sortGroup in providerGroup.GroupBy(quest => quest.SortOrder))
            {
                if (sortGroup.Count() > 1)
                {
                    string ids = string.Join(", ", sortGroup.Select(quest => quest.QuestId).Order(StringComparer.Ordinal));
                    messages.Add(new ValidationMessage("WARN", "PROVIDER_SORT_DUPLICATE", $"{providerGroup.Key} has duplicate sort_order {sortGroup.Key}: {ids}"));
                }
            }
        }

        foreach (string cycle in FindPrerequisiteCycles(questsById.Keys, questsById.ToDictionary(
            pair => pair.Key,
            pair => pair.Value[0].Prerequisites,
            StringComparer.Ordinal)))
        {
            messages.Add(new ValidationMessage("ERROR", "PREREQUISITE_CYCLE", $"Quest prerequisite cycle detected: {cycle}"));
        }

        return messages
            .OrderByDescending(message => message.Severity == "ERROR")
            .ThenByDescending(message => message.Severity == "WARN")
            .ThenBy(message => message.Code, StringComparer.Ordinal)
            .ThenBy(message => message.Message, StringComparer.Ordinal)
            .ToArray();
    }

    private static IEnumerable<string> FindPrerequisiteCycles(
        IEnumerable<string> questIds,
        IReadOnlyDictionary<string, IReadOnlyList<string>> prerequisitesByQuestId)
    {
        HashSet<string> permanentMarks = new(StringComparer.Ordinal);
        HashSet<string> temporaryMarks = new(StringComparer.Ordinal);
        Stack<string> path = new();
        HashSet<string> reportedCycles = new(StringComparer.Ordinal);

        foreach (string questId in questIds.Order(StringComparer.Ordinal))
        {
            foreach (string cycle in Visit(questId))
            {
                if (reportedCycles.Add(cycle))
                {
                    yield return cycle;
                }
            }
        }

        IEnumerable<string> Visit(string questId)
        {
            if (permanentMarks.Contains(questId))
            {
                yield break;
            }

            if (temporaryMarks.Contains(questId))
            {
                string[] pathArray = path.Reverse().ToArray();
                int cycleStart = Array.IndexOf(pathArray, questId);
                if (cycleStart >= 0)
                {
                    yield return string.Join(" -> ", pathArray.Skip(cycleStart).Append(questId));
                }

                yield break;
            }

            temporaryMarks.Add(questId);
            path.Push(questId);

            if (prerequisitesByQuestId.TryGetValue(questId, out IReadOnlyList<string>? prerequisites))
            {
                foreach (string prerequisite in prerequisites)
                {
                    if (prerequisitesByQuestId.ContainsKey(prerequisite))
                    {
                        foreach (string cycle in Visit(prerequisite))
                        {
                            yield return cycle;
                        }
                    }
                }
            }

            path.Pop();
            temporaryMarks.Remove(questId);
            permanentMarks.Add(questId);
        }
    }

    private static string BuildReport(
        IReadOnlyList<QuestDefinition> quests,
        IReadOnlyDictionary<string, QuestTextString> textStrings,
        IReadOnlyList<ValidationMessage> validationMessages,
        string projectRoot,
        string questJsonPath,
        string questTextCsvPath,
        string language)
    {
        List<QuestDefinition> sortedQuests = quests
            .OrderBy(quest => quest.ProviderKey, StringComparer.Ordinal)
            .ThenBy(quest => quest.SortOrder)
            .ThenBy(quest => quest.QuestId, StringComparer.Ordinal)
            .ToList();

        StringBuilder builder = new();
        builder.AppendLine("# Quest Graph");
        builder.AppendLine();
        builder.AppendLine("Generated by `Tools/QuestGraphGenerator`.");
        builder.AppendLine();
        builder.AppendLine($"- Source: `{ToProjectRelativePath(projectRoot, questJsonPath)}`");
        builder.AppendLine($"- Text: `{ToProjectRelativePath(projectRoot, questTextCsvPath)}`");
        builder.AppendLine($"- Language: `{language}`");
        builder.AppendLine($"- Generated: {DateTime.Now:yyyy-MM-dd HH:mm:ss}");
        builder.AppendLine();

        AppendSummary(builder, quests, validationMessages);
        AppendMermaidGraph(builder, sortedQuests, textStrings, language);
        AppendProviderTables(builder, sortedQuests, textStrings, language);
        AppendObjectiveSummary(builder, sortedQuests, textStrings, language);
        AppendValidation(builder, validationMessages);

        return builder.ToString();
    }

    private static void AppendSummary(
        StringBuilder builder,
        IReadOnlyList<QuestDefinition> quests,
        IReadOnlyList<ValidationMessage> validationMessages)
    {
        int providerCount = quests
            .Where(quest => !string.IsNullOrWhiteSpace(quest.ProviderId))
            .Select(quest => quest.ProviderId!.Trim())
            .Distinct(StringComparer.Ordinal)
            .Count();

        builder.AppendLine("## Summary");
        builder.AppendLine();
        builder.AppendLine($"- Quests: {quests.Count}");
        builder.AppendLine($"- Providers: {providerCount}");
        builder.AppendLine($"- Providerless quests: {quests.Count(quest => string.IsNullOrWhiteSpace(quest.ProviderId))}");
        builder.AppendLine($"- Prerequisite edges: {quests.Sum(quest => quest.Prerequisites.Count)}");
        builder.AppendLine($"- Validation errors: {validationMessages.Count(message => message.Severity == "ERROR")}");
        builder.AppendLine($"- Validation warnings: {validationMessages.Count(message => message.Severity == "WARN")}");
        builder.AppendLine();
    }

    private static void AppendMermaidGraph(
        StringBuilder builder,
        IReadOnlyList<QuestDefinition> quests,
        IReadOnlyDictionary<string, QuestTextString> textStrings,
        string language)
    {
        builder.AppendLine("## Prerequisite Graph");
        builder.AppendLine();
        builder.AppendLine("```mermaid");
        builder.AppendLine("flowchart LR");

        foreach (IGrouping<string, QuestDefinition> providerGroup in quests.GroupBy(quest => quest.ProviderKey, StringComparer.Ordinal))
        {
            string providerId = $"P_{MakeMermaidId(providerGroup.Key)}";
            builder.AppendLine($"  subgraph {providerId}[\"{EscapeMermaidLabel(providerGroup.Key)}\"]");
            foreach (QuestDefinition quest in providerGroup)
            {
                builder.AppendLine($"    {MakeQuestNodeId(quest.QuestId)}[\"{EscapeMermaidLabel(BuildQuestNodeLabel(quest, textStrings, language))}\"]");
            }

            builder.AppendLine("  end");
        }

        foreach (QuestDefinition quest in quests)
        {
            foreach (string prerequisite in quest.Prerequisites)
            {
                builder.AppendLine($"  {MakeQuestNodeId(prerequisite)} --> {MakeQuestNodeId(quest.QuestId)}");
            }
        }

        builder.AppendLine("```");
        builder.AppendLine();
    }

    private static void AppendProviderTables(
        StringBuilder builder,
        IReadOnlyList<QuestDefinition> quests,
        IReadOnlyDictionary<string, QuestTextString> textStrings,
        string language)
    {
        builder.AppendLine("## Provider Chains");
        builder.AppendLine();

        foreach (IGrouping<string, QuestDefinition> providerGroup in quests.GroupBy(quest => quest.ProviderKey, StringComparer.Ordinal))
        {
            builder.AppendLine($"### {EscapeMarkdown(providerGroup.Key)}");
            builder.AppendLine();
            builder.AppendLine("| Sort | Quest | Title | Prerequisites | Rewards | Auto Track |");
            builder.AppendLine("| ---: | --- | --- | --- | --- | --- |");

            foreach (QuestDefinition quest in providerGroup.OrderBy(quest => quest.SortOrder).ThenBy(quest => quest.QuestId, StringComparer.Ordinal))
            {
                builder.AppendLine(
                    $"| {quest.SortOrder} | `{EscapeMarkdown(quest.QuestId)}` | {EscapeMarkdown(ResolveText(quest.TitleStringKey, textStrings, language, quest.QuestId))} | {FormatPrerequisites(quest)} | {EscapeMarkdown(FormatRewards(quest.Rewards))} | {(quest.AutoTrackOnAccept ? "Yes" : "No")} |");
            }

            builder.AppendLine();
        }
    }

    private static void AppendObjectiveSummary(
        StringBuilder builder,
        IReadOnlyList<QuestDefinition> quests,
        IReadOnlyDictionary<string, QuestTextString> textStrings,
        string language)
    {
        builder.AppendLine("## Objectives");
        builder.AppendLine();
        builder.AppendLine("| Quest | Objective | Type | Text | Required | Filters |");
        builder.AppendLine("| --- | --- | --- | --- | ---: | --- |");

        foreach (QuestDefinition quest in quests)
        {
            foreach (QuestObjective objective in quest.Objectives)
            {
                string text = ResolveText(objective.TextStringKey, textStrings, language, objective.ObjectiveId);
                builder.AppendLine(
                    $"| `{EscapeMarkdown(quest.QuestId)}` | `{EscapeMarkdown(objective.ObjectiveId)}` | `{EscapeMarkdown(objective.Type)}` | {EscapeMarkdown(text)} | {Math.Max(1, objective.RequiredCount)} | {EscapeMarkdown(FormatObjectiveFilters(objective))} |");
            }
        }

        builder.AppendLine();
    }

    private static void AppendValidation(StringBuilder builder, IReadOnlyList<ValidationMessage> validationMessages)
    {
        builder.AppendLine("## Validation");
        builder.AppendLine();

        if (validationMessages.Count == 0)
        {
            builder.AppendLine("No validation issues.");
            builder.AppendLine();
            return;
        }

        builder.AppendLine("| Severity | Code | Message |");
        builder.AppendLine("| --- | --- | --- |");
        foreach (ValidationMessage message in validationMessages)
        {
            builder.AppendLine($"| {message.Severity} | `{message.Code}` | {EscapeMarkdown(message.Message)} |");
        }

        builder.AppendLine();
    }

    private static string BuildQuestNodeLabel(
        QuestDefinition quest,
        IReadOnlyDictionary<string, QuestTextString> textStrings,
        string language)
    {
        string title = ResolveText(quest.TitleStringKey, textStrings, language, quest.QuestId);
        return title == quest.QuestId ? quest.QuestId : $"{quest.QuestId}<br/>{title}";
    }

    private static string ResolveText(
        string? stringKey,
        IReadOnlyDictionary<string, QuestTextString> textStrings,
        string language,
        string fallback)
    {
        if (!string.IsNullOrWhiteSpace(stringKey) && textStrings.TryGetValue(stringKey.Trim(), out QuestTextString? textString))
        {
            string text = textString.GetText(language);
            if (!string.IsNullOrWhiteSpace(text))
            {
                return text;
            }
        }

        return fallback;
    }

    private static string FormatPrerequisites(QuestDefinition quest)
    {
        return quest.Prerequisites.Count == 0
            ? "-"
            : string.Join("<br>", quest.Prerequisites.Select(prerequisite => $"`{EscapeMarkdown(prerequisite)}`"));
    }

    private static string FormatRewards(QuestRewards? rewards)
    {
        if (rewards is null)
        {
            return "-";
        }

        List<string> parts = [];
        if (rewards.Coins > 0)
        {
            parts.Add($"{rewards.Coins} coins");
        }

        if (rewards.Items is { Count: > 0 })
        {
            parts.AddRange(rewards.Items.Select(item => $"item {item.ItemId} x{Math.Max(1, item.Quantity)}"));
        }

        foreach (string facilityId in MergeStringLists(rewards.HousingFacilities, rewards.HousingFacilityUnlocks))
        {
            parts.Add($"facility {facilityId}");
        }

        foreach (string recipeId in MergeStringLists(rewards.WorkbenchRecipes, rewards.WorkbenchRecipeUnlocks))
        {
            parts.Add($"recipe {recipeId}");
        }

        return parts.Count == 0 ? "-" : string.Join("<br>", parts);
    }

    private static IEnumerable<string> MergeStringLists(params List<string>?[] lists)
    {
        return lists
            .Where(list => list is not null)
            .SelectMany(list => list!)
            .Where(value => !string.IsNullOrWhiteSpace(value))
            .Select(value => value.Trim())
            .Distinct(StringComparer.Ordinal)
            .Order(StringComparer.Ordinal);
    }

    private static string FormatObjectiveFilters(QuestObjective objective)
    {
        List<string> filters = [];
        AddFilter(filters, "source", objective.SourceLevel);
        AddFilter(filters, "target", objective.TargetLevel);
        AddFilter(filters, "enemy", objective.EnemyId);
        AddFilter(filters, "interaction_event", objective.InteractionEventId);
        AddFilter(filters, "interaction_type", objective.InteractionType);
        AddFilter(filters, "warp", objective.WarpPointId);
        AddFilter(filters, "target_warp", objective.TargetWarpPointId);
        if (objective.ItemId is not null)
        {
            filters.Add($"item={objective.ItemId}");
        }

        return filters.Count == 0 ? "-" : string.Join(", ", filters);
    }

    private static void AddFilter(List<string> filters, string name, string? value)
    {
        if (!string.IsNullOrWhiteSpace(value))
        {
            filters.Add($"{name}={value.Trim()}");
        }
    }

    private static string MakeQuestNodeId(string questId)
    {
        return $"Q_{MakeMermaidId(questId)}";
    }

    private static string MakeMermaidId(string value)
    {
        StringBuilder builder = new();
        foreach (char character in value)
        {
            builder.Append(char.IsAsciiLetterOrDigit(character) ? character : '_');
        }

        string id = builder.ToString().Trim('_');
        return string.IsNullOrWhiteSpace(id) ? "none" : id;
    }

    private static string EscapeMermaidLabel(string value)
    {
        return value
            .Replace("\\", "\\\\", StringComparison.Ordinal)
            .Replace("\"", "\\\"", StringComparison.Ordinal)
            .Replace("[", "(", StringComparison.Ordinal)
            .Replace("]", ")", StringComparison.Ordinal);
    }

    private static string EscapeMarkdown(string value)
    {
        return value
            .Replace("|", "\\|", StringComparison.Ordinal)
            .Replace("\r", " ", StringComparison.Ordinal)
            .Replace("\n", " ", StringComparison.Ordinal);
    }

    private static string ToProjectRelativePath(string projectRoot, string path)
    {
        return Path.GetRelativePath(projectRoot, path).Replace('\\', '/');
    }
}
