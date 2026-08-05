using UnrealBuildTool;

public class QuestDatasetSwitcherEditor : ModuleRules
{
    public QuestDatasetSwitcherEditor(ReadOnlyTargetRules Target)
        : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "InputCore",
                "Json",
                "LevelEditor",
                "Projects",
                "QuestDatasetSwitcher",
                "Slate",
                "SlateCore",
                "ToolMenus",
                "UnrealEd"
            });

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PrivateDependencyModuleNames.Add("LiveCoding");
        }
    }
}
