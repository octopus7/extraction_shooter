using UnrealBuildTool;

public class TunaWarpTransitionEditor : ModuleRules
{
	public TunaWarpTransitionEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AssetRegistry",
			"Core",
			"CoreUObject",
			"Engine",
			"MaterialEditor",
			"TunaWarpTransition",
			"UnrealEd"
		});
	}
}
