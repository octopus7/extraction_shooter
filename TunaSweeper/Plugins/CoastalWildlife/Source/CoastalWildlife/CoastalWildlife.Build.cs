using UnrealBuildTool;

public class CoastalWildlife : ModuleRules
{
	public CoastalWildlife(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine" });
	}
}
