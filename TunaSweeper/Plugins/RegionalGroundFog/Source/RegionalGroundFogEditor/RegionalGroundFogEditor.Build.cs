using UnrealBuildTool;

public class RegionalGroundFogEditor : ModuleRules
{
	public RegionalGroundFogEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"RegionalGroundFog",
			"UnrealEd"
		});
	}
}
