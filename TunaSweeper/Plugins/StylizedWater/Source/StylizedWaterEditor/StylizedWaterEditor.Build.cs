using UnrealBuildTool;

public class StylizedWaterEditor : ModuleRules
{
	public StylizedWaterEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AssetRegistry",
			"AssetTools",
			"BlueprintGraph",
			"Core",
			"CoreUObject",
			"Engine",
			"Kismet",
			"LevelEditor",
			"MaterialEditor",
			"Projects",
			"ProceduralMeshComponent",
			"RHI",
			"Slate",
			"SlateCore",
			"StylizedWater",
			"ToolMenus",
			"UnrealEd"
		});
	}
}
