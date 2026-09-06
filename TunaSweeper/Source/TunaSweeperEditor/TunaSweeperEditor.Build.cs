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
			"PropertyEditor",
			"RenderCore",
			"Slate",
			"SlateCore",
			"ToolMenus",
			"UATHelper",
			"TunaSweeper",
		});
	}
}
