using UnrealBuildTool;

public class TunaSweeperEditor : ModuleRules
{
	public TunaSweeperEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"EngineSettings",
			"InputCore",
			"EnhancedInput",
			"UnrealEd",
			"AssetTools",
			"AssetRegistry",
			"KismetCompiler",
			"BlueprintGraph",
			"UMG",
			"UMGEditor",
			"MediaAssets",
			"Slate",
			"SlateCore",
			"TunaSweeper"
		});
	}
}
