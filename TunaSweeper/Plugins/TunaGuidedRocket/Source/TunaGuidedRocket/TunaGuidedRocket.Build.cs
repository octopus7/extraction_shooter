using UnrealBuildTool;
public class TunaGuidedRocket : ModuleRules
{
    public TunaGuidedRocket(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        bUseUnity = false;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "Niagara" });
    }
}
