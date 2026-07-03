using System.Text.Json;
using System.Text.Json.Serialization;

namespace QuestGraphGenerator;

public sealed class QuestDefinition
{
    [JsonPropertyName("quest_id")]
    public string QuestId { get; set; } = "";

    [JsonPropertyName("provider_id")]
    public string? ProviderId { get; set; }

    [JsonPropertyName("sort_order")]
    public int SortOrder { get; set; }

    [JsonPropertyName("required_completed_quest_ids")]
    public List<string>? RequiredCompletedQuestIds { get; set; }

    [JsonPropertyName("title_string_key")]
    public string? TitleStringKey { get; set; }

    [JsonPropertyName("description_string_key")]
    public string? DescriptionStringKey { get; set; }

    [JsonPropertyName("auto_track_on_accept")]
    public bool AutoTrackOnAccept { get; set; }

    [JsonPropertyName("objectives")]
    public List<QuestObjective> Objectives { get; set; } = [];

    [JsonPropertyName("rewards")]
    public QuestRewards? Rewards { get; set; }

    [JsonExtensionData]
    public Dictionary<string, JsonElement>? ExtensionData { get; set; }

    public IReadOnlyList<string> Prerequisites =>
        RequiredCompletedQuestIds?
            .Where(value => !string.IsNullOrWhiteSpace(value))
            .Select(value => value.Trim())
            .Distinct(StringComparer.Ordinal)
            .ToArray()
        ?? [];

    public string ProviderKey =>
        string.IsNullOrWhiteSpace(ProviderId) ? "(no provider)" : ProviderId.Trim();
}

public sealed class QuestObjective
{
    [JsonPropertyName("objective_id")]
    public string ObjectiveId { get; set; } = "";

    [JsonPropertyName("type")]
    public string Type { get; set; } = "";

    [JsonPropertyName("text_string_key")]
    public string? TextStringKey { get; set; }

    [JsonPropertyName("required_count")]
    public int RequiredCount { get; set; } = 1;

    [JsonPropertyName("source_level")]
    public string? SourceLevel { get; set; }

    [JsonPropertyName("target_level")]
    public string? TargetLevel { get; set; }

    [JsonPropertyName("enemy_id")]
    public string? EnemyId { get; set; }

    [JsonPropertyName("interaction_event_id")]
    public string? InteractionEventId { get; set; }

    [JsonPropertyName("interaction_type")]
    public string? InteractionType { get; set; }

    [JsonPropertyName("warp_point_id")]
    public string? WarpPointId { get; set; }

    [JsonPropertyName("target_warp_point_id")]
    public string? TargetWarpPointId { get; set; }

    [JsonPropertyName("item_id")]
    public int? ItemId { get; set; }

    [JsonExtensionData]
    public Dictionary<string, JsonElement>? ExtensionData { get; set; }
}

public sealed class QuestRewards
{
    [JsonPropertyName("coins")]
    public int Coins { get; set; }

    [JsonPropertyName("items")]
    public List<ItemReward>? Items { get; set; }

    [JsonPropertyName("housing_facilities")]
    public List<string>? HousingFacilities { get; set; }

    [JsonPropertyName("housing_facility_unlocks")]
    public List<string>? HousingFacilityUnlocks { get; set; }

    [JsonPropertyName("workbench_recipes")]
    public List<string>? WorkbenchRecipes { get; set; }

    [JsonPropertyName("workbench_recipe_unlocks")]
    public List<string>? WorkbenchRecipeUnlocks { get; set; }

    [JsonExtensionData]
    public Dictionary<string, JsonElement>? ExtensionData { get; set; }
}

public sealed class ItemReward
{
    [JsonPropertyName("item_id")]
    public int ItemId { get; set; }

    [JsonPropertyName("quantity")]
    public int Quantity { get; set; } = 1;

    [JsonExtensionData]
    public Dictionary<string, JsonElement>? ExtensionData { get; set; }
}

public sealed class QuestTextString
{
    public string StringKey { get; init; } = "";
    public string Korean { get; init; } = "";
    public string English { get; init; } = "";
    public string Japanese { get; init; } = "";

    public string GetText(string language)
    {
        return language switch
        {
            "en" => English,
            "ja" => Japanese,
            _ => Korean
        };
    }
}

public sealed record ValidationMessage(string Severity, string Code, string Message);
