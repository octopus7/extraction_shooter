using UnrealBuildTool;
using System.IO;
public class RaidLevelRuntime : ModuleRules
{
    public RaidLevelRuntime(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine" });
        PrivateDependencyModuleNames.Add("Projects");
        RuntimeDependencies.Add(Path.Combine(PluginDirectory, "Content/Localization/RaidLevelEditor.csv"), StagedFileType.UFS);
    }
}
