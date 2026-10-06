using System.Text.Json.Nodes;
using System.Text;
namespace TunaSweeper.SaveTool;
public static class HumanOutput
{
    public static string Render(JsonObject response, Localization strings, string language)
    {
        var text = new StringBuilder();
        void Line(string key, object? value) => text.AppendLine($"{strings[$"tool.save.{key}"]}: {value}");
        bool ok = response["ok"]?.GetValue<bool>() == true;
        text.AppendLine(strings[ok ? "tool.save.success" : "tool.save.failure"]);
        if (!ok) Line("code", response["code"]);
        if (response["committed"]?.GetValue<bool>() == false) text.AppendLine(strings["tool.save.preview"]);
        foreach (var key in new[] { "hash", "sourceHash", "validationCode", "backupPath", "runDirectory" })
            if (response[key] is { } value) Line(key, value);
        if (response["slots"] is JsonArray saves)
        {
            text.AppendLine(strings["tool.save.slots"]);
            foreach (var entry in saves)
                text.AppendLine($"{entry!["slot"]}  {entry["flavor"]}  {entry["saveVersion"]}  {entry["readable"]}  {entry["savePath"]}");
            if (saves.Count == 0) text.AppendLine(strings["tool.save.empty"]);
        }
        if (response["save"] is JsonObject save)
        {
            foreach (var key in new[] { "saveSlotIndex", "buildFlavor", "saveVersion", "difficultyStage", "totalPlaySeconds", "questCoinBalance", "totalExperiencePoints", "selectedOutfitId" })
                if (save[key] is { } value) Line(key, value);
            Line("inventoryCapacity", response["inventoryCapacity"]);
            var definitions = (response["itemDefinitions"] as JsonArray ?? []).OfType<JsonObject>().ToDictionary(x => x["itemId"]!.GetValue<int>());
            var instances = (save["itemInstances"] as JsonArray ?? []).OfType<JsonObject>().GroupBy(x => x["uid"]!.GetValue<string>()).ToDictionary(g => g.Key, g => g.First());
            string Name(int id) => definitions.TryGetValue(id, out var d)
                ? (d["names"]?[language] ?? d["names"]?["en"] ?? d["nameStringKey"])?.ToString() ?? id.ToString()
                : id.ToString();
            foreach (var section in new[] { "equipmentSlots", "inventorySlots", "auxiliaryBagSlots", "usableQuickSlots", "storageSlots" })
            {
                text.AppendLine();
                text.AppendLine(strings[$"tool.save.{section}"]);
                text.AppendLine(strings["tool.save.item_header"]);
                int count = 0;
                if (save[section] is JsonArray slots)
                    for (int i = 0; i < slots.Count; i++)
                    {
                        var uid = slots[i]?["itemUid"]?.ToString();
                        if (uid is null || !instances.TryGetValue(uid, out var item)) continue;
                        count++;
                        int id = item["itemId"]!.GetValue<int>();
                        string ammo = item["loadedAmmoItemId"]?.GetValue<int>() is > 0 and var ammoId
                            ? $"{Name(ammoId)} × {item["loadedAmmoCount"]}" : "-";
                        text.AppendLine($"{i,3}  {id,6}  {Name(id)} × {item["quantity"]}  |  {ammo}");
                        if (item["attachmentSlots"] is JsonObject attachments)
                            foreach (var a in attachments)
                                if (a.Value is not null && instances.TryGetValue(a.Value.ToString(), out var child))
                                    text.AppendLine($"     {a.Key}: {Name(child["itemId"]!.GetValue<int>())}");
                    }
                if (count == 0) text.AppendLine(strings["tool.save.empty"]);
            }
            text.AppendLine();
            foreach (var key in new[] { "questProgressStates", "activeResearchStates", "appliedResearchNodeIds", "completedScenarioFlags", "acquiredMemoIds", "worldProgressStates" })
                if (save[key] is JsonArray rows) Line(key, rows.Count);
            text.AppendLine(strings["tool.save.json_hint"]);
        }
        if (response["preset"] is JsonObject preset)
        {
            Line("presetCount", (preset["equipment"] as JsonArray)?.Count ?? 0);
            text.AppendLine(strings["tool.save.preset_hint"]);
        }
        return text.ToString();
    }
}
