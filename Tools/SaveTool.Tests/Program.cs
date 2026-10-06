using TunaSweeper.SaveTool;
using System.Text.Json.Nodes;

int failures = 0;
void Check(bool ok, string name) { Console.WriteLine($"{(ok ? "PASS" : "FAIL")} {name}"); if (!ok) failures++; }
void Reject(string[] args, string name)
{
    try { Cli.BuildRequest(args); Check(false, name); }
    catch (ArgumentException) { Check(true, name); }
}
string[] common = ["--save", "C:/save folder/세이브.sav", "--flavor", "Demo", "--slot", "1"];
var preview = Cli.BuildRequest(["add", ..common, "--item", "2002", "--quantity", "250"]);
Check(preview["operation"]?.GetValue<string>() == "add", "Operation forwarded");
Check(preview["commit"]?.GetValue<bool>() == false, "No implicit writes");
Check(preview["items"]?[0]?["quantity"]?.GetValue<int>() == 250, "Quantity preserved");
Check(preview["savePath"]?.GetValue<string>()?.Contains("세이브.sav") == true, "Unicode path preserved");
var commit = Cli.BuildRequest(["add", ..common, "--item", "1002", "--ammo", "2002", "--rounds", "30", "--commit"]);
Check(commit["commit"]?.GetValue<bool>() == true && commit["items"]?[0]?["loadedAmmoCount"]?.GetValue<int>() == 30, "Explicit loaded weapon commit");
Reject(["add", ..common, "--item", "1002", "--commmit"], "Typo rejected");
Reject(["add", ..common, "--item", "1002", "--quantity", "0"], "Zero rejected");
Reject(["inspect", ..common, "--commit"], "Read cannot commit");
Reject(["inspect", ..common, "--slot", "2"], "Duplicate option rejected");
Reject(["inspect", "--save", "a.sav"], "Identity required");
Reject(["add", ..common, "--item", "1002", "--quantity", "1.5"], "Fraction rejected");
Reject(["add", ..common, "--item", "1002", "--rounds", "30"], "Rounds require ammo");
var snapshot = JsonNode.Parse("""
{"ok":true,"code":"ok","inventoryCapacity":40,"save":{"saveSlotIndex":1,"buildFlavor":"Demo","questCoinBalance":12345,"totalExperiencePoints":"6789","equipmentSlots":[{"itemUid":"ABC"}],"inventorySlots":[],"itemInstances":[{"uid":"ABC","itemId":1002,"quantity":1,"loadedAmmoItemId":2002,"loadedAmmoCount":30}]},"itemDefinitions":[{"itemId":1002,"nameStringKey":"item.rifle","names":{"ko":"소총"}},{"itemId":2002,"names":{"ko":"소총탄"}}]}
""")!.AsObject();
var rendered = HumanOutput.Render(snapshot, new Localization(Cli.FindProject(null), "ko"), "ko");
Check(rendered.Contains("소총") && rendered.Contains("30") && !rendered.Contains("ABC"), "Human output joins slots to localized item rows");
Check(rendered.Contains("12345") && rendered.Contains("6789"), "Human output includes saved progression summary");
return failures == 0 ? 0 : 1;
