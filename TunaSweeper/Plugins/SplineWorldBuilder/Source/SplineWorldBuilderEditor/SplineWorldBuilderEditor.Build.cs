using UnrealBuildTool;

public class SplineWorldBuilderEditor : ModuleRules
{
	public SplineWorldBuilderEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AssetRegistry",
			"Core",
			"CoreUObject",
			"Engine",
			"ImageCore",
			"LevelEditor",
			"MaterialEditor",
			"MeshConversion",
			"MeshDescription",
			"Projects",
			"Slate",
			"SlateCore",
			"SplineWorldBuilder",
			"StaticMeshDescription",
			"ToolMenus",
			"UnrealEd"
		});
	}
}
