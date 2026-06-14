using UnrealBuildTool;

public class BushMeshBuilderEditor : ModuleRules
{
	public BushMeshBuilderEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AssetRegistry",
			"AssetTools",
			"ContentBrowser",
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"MeshConversion",
			"MeshDescription",
			"Slate",
			"SlateCore",
			"StaticMeshDescription",
			"ToolMenus",
			"UnrealEd"
		});
	}
}
