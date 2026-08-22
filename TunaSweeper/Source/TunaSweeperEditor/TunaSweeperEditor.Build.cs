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
			"AnimGraph",
			"AnimGraphRuntime",
			"AudioEditor",
			"Chaos",
			"DataflowCore",
			"DeveloperToolSettings",
			"FractureEngine",
			"GeometryCollectionEngine",
			"KismetCompiler",
			"Landscape",
			"LevelEditor",
			"BlueprintGraph",
			"UMG",
			"UMGEditor",
			"MediaAssets",
			"MeshDescription",
			"Niagara",
			"PropertyEditor",
			"PlanarCut",
			"PhysicsCore",
			"PhysicsUtilities",
			"RenderCore",
			"Slate",
			"SlateCore",
			"StaticMeshDescription",
			"ToolMenus",
			"FoldingCanopyGarageDoor",
			"TunaSweeper",
			"MiyakovCharacterSystem",
			"MiyakovCharacterSystemEditor"
		});
	}
}
