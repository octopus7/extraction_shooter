using UnrealBuildTool;

public class ForestScatter : ModuleRules
{
	public ForestScatter(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "PCG" });
		PrivateDependencyModuleNames.AddRange(new[] { "Landscape", "Projects" });
		RuntimeDependencies.Add("$(PluginDir)/Content/Localization/ForestScatterEditor.csv", StagedFileType.UFS);
		RuntimeDependencies.Add("$(PluginDir)/Content/Localization/ForestScatterEditor.ko.csv", StagedFileType.UFS);
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.Add("ImageCore");
		}
	}
}
