using UnrealBuildTool;

public class ChainPhysicsEditor : ModuleRules
{
	public ChainPhysicsEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"UnrealEd",
			"AssetRegistry",
			"AssetTools",
			"AnimGraph",
			"AnimGraphRuntime",
			"BlueprintGraph",
			"KismetCompiler",
			"PhysicsCore",
			"PhysicsUtilities",
			"PropertyEditor",
			"Slate",
			"SlateCore",
			"ToolMenus"
		});
	}
}
