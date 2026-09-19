using UnrealBuildTool;

public class TunaSweeperAnimGraph : ModuleRules
{
    public TunaSweeperAnimGraph(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "AnimGraph", "BlueprintGraph", "TunaSweeper" });
    }
}
