using UnrealBuildTool;

public class TunaSweeperEditor : ModuleRules
{
	public TunaSweeperEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		// Keep editor tools independent of accidental unity-build includes.
		bUseUnity = false;

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"InputCore",
            "AIModule", // Hopper PIE test issues real navigation requests.
            "NavigationSystem", // Wait for navigation readiness before PIE movement checks.
			"Core",
			"CoreUObject",
			"Engine",
			"EngineSettings",
			"ImageCore",
			"Json",
			"JsonUtilities",
			"UnrealEd",
			"AssetTools",
			"AssetRegistry",
			"AudioEditor",
			"DeveloperToolSettings",
			"LevelEditor",
			"UMG",
			"UMGEditor",
			"PropertyEditor",
			"RenderCore",
			"RHI", // Anime-tree render validation checks the active shader platform.
			"MeshDescription",
			"SkeletalMeshDescription",
			"AnimGraph",
			"BlueprintGraph",
			"Slate",
			"SlateCore",
			"ToolMenus",
			"UATHelper",
			"TunaSweeper",
			"CanopyRim",
		});
	}
}
