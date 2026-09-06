using UnrealBuildTool;

public class SplineWorldBuilderEditor : ModuleRules
{
	public SplineWorldBuilderEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"MeshDescription",
			"StaticMeshDescription",
			"Core",
			"CoreUObject",
			"Engine",
			"LevelEditor",
			"Slate",
			"SlateCore",
			"SplineWorldBuilder",
			"ToolMenus",
			"UnrealEd"
		});
	}
}
