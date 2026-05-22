using UnrealBuildTool;

public class SogSplatEditor : ModuleRules
{
	public SogSplatEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AssetRegistry",
			"AssetTools",
			"Core",
			"CoreUObject",
			"EditorFramework",
			"Engine",
			"SogSplat",
			"UnrealEd"
		});
	}
}
