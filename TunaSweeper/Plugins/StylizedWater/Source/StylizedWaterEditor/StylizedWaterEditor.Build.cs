using UnrealBuildTool;

public class StylizedWaterEditor : ModuleRules
{
	public StylizedWaterEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"ProceduralMeshComponent",
			"RHI",
			"Core",
			"CoreUObject",
			"Engine",
			"LevelEditor",
			"Slate",
			"SlateCore",
			"StylizedWater",
			"ToolMenus",
			"UnrealEd"
		});
	}
}
