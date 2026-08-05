using UnrealBuildTool;

public class QuestDatasetSwitcher : ModuleRules
{
    public QuestDatasetSwitcher(ReadOnlyTargetRules Target)
        : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core"
            });

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Json"
            });
    }
}
