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
			"ImageCore",
			"Json",
			"JsonUtilities",
			"UnrealEd",
			"AssetTools",
			"AssetRegistry",
			"AudioEditor",
			"KismetCompiler",
			"Landscape",
			"LevelEditor",
			"BlueprintGraph",
			"UMG",
			"UMGEditor",
			"MediaAssets",
			"MeshDescription",
			"Niagara",
			"RenderCore",
			"Slate",
			"SlateCore",
			"StaticMeshDescription",
			"ToolMenus",
			"TunaSweeper"
		});
	}
}
