using UnrealBuildTool;

public class StylizedWaterEditor : ModuleRules
{
	public StylizedWaterEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"ImageCore",
			"LevelEditor",
			"ProceduralMeshComponent",
			"RHI",
			"RenderCore",
			"Slate",
			"SlateCore",
			"StylizedWater",
			"ToolMenus",
			"UnrealEd"
		});
	}
}
