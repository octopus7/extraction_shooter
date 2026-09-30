using UnrealBuildTool;
public class RaidLevelEditor : ModuleRules
{
    public RaidLevelEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        bUseUnity = false;
        PrivateDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "UnrealEd", "Json", "RaidLevelRuntime" });
    }
}
