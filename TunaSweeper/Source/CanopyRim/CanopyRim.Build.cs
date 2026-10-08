using UnrealBuildTool;

public class CanopyRim : ModuleRules
{
    public CanopyRim(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine" });
        PrivateDependencyModuleNames.AddRange(new[] { "Renderer", "RenderCore", "RHI", "Projects" });
    }
}
