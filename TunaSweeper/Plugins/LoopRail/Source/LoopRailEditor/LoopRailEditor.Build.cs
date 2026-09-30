using UnrealBuildTool;
public class LoopRailEditor : ModuleRules
{
    public LoopRailEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        bUseUnity = false;
        PrivateDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "LoopRail", "UnrealEd", "RenderCore", "ImageCore",
            "AssetRegistry" });
    }
}
