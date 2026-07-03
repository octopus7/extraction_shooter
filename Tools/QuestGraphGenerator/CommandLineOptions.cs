namespace QuestGraphGenerator;

public sealed class CommandLineOptions
{
    public string? ProjectRoot { get; private init; }
    public string? QuestJsonPath { get; private init; }
    public string? QuestTextCsvPath { get; private init; }
    public string? OutputPath { get; private init; }
    public string Language { get; private init; } = "ko";
    public bool FailOnWarning { get; private init; }
    public bool ShowHelp { get; private init; }

    public const string Usage =
        """
        Usage:
          dotnet run --project Tools/QuestGraphGenerator -- [options]

        Options:
          --project-root <path>     extraction_shooter root. Auto-detected by default.
          --quest-json <path>       QuestDefinitions.json path. Defaults to TunaSweeper/Content/Data/QuestDefinitions.json.
          --quest-text-csv <path>   QuestTextStrings.csv path. Defaults to TunaSweeper/Content/Data/QuestTextStrings.csv.
          --output <path>           Markdown output path. Defaults to Docs/quest_graph.md.
          --language <ko|en|ja>     Text language for labels. Defaults to ko.
          --fail-on-warning         Return exit code 1 when warnings are found.
          -h, --help                Show this help.
        """;

    public static CommandLineOptions Parse(string[] args)
    {
        string? projectRoot = null;
        string? questJsonPath = null;
        string? questTextCsvPath = null;
        string? outputPath = null;
        string language = "ko";
        bool failOnWarning = false;
        bool showHelp = false;

        for (int index = 0; index < args.Length; index++)
        {
            string arg = args[index];
            switch (arg)
            {
                case "-h":
                case "--help":
                    showHelp = true;
                    break;
                case "--project-root":
                    projectRoot = ReadValue(args, ref index, arg);
                    break;
                case "--quest-json":
                    questJsonPath = ReadValue(args, ref index, arg);
                    break;
                case "--quest-text-csv":
                    questTextCsvPath = ReadValue(args, ref index, arg);
                    break;
                case "--output":
                    outputPath = ReadValue(args, ref index, arg);
                    break;
                case "--language":
                    language = NormalizeLanguage(ReadValue(args, ref index, arg));
                    break;
                case "--fail-on-warning":
                    failOnWarning = true;
                    break;
                default:
                    throw new ArgumentException($"Unknown argument: {arg}");
            }
        }

        return new CommandLineOptions
        {
            ProjectRoot = projectRoot,
            QuestJsonPath = questJsonPath,
            QuestTextCsvPath = questTextCsvPath,
            OutputPath = outputPath,
            Language = language,
            FailOnWarning = failOnWarning,
            ShowHelp = showHelp
        };
    }

    private static string ReadValue(string[] args, ref int index, string optionName)
    {
        if (index + 1 >= args.Length || args[index + 1].StartsWith("--", StringComparison.Ordinal))
        {
            throw new ArgumentException($"{optionName} requires a value.");
        }

        index++;
        return args[index];
    }

    private static string NormalizeLanguage(string language)
    {
        string normalized = language.Trim().ToLowerInvariant();
        return normalized switch
        {
            "ko" or "kr" or "korean" => "ko",
            "en" or "english" => "en",
            "ja" or "jp" or "japanese" => "ja",
            _ => throw new ArgumentException($"Unsupported language: {language}")
        };
    }
}
