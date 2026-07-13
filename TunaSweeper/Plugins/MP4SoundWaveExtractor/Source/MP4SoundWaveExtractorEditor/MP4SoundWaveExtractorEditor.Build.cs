using UnrealBuildTool;

public class MP4SoundWaveExtractorEditor : ModuleRules
{
	public MP4SoundWaveExtractorEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AssetTools",
			"AudioEditor",
			"Core",
			"CoreUObject",
			"DesktopPlatform",
			"Engine",
			"InputCore",
			"Slate",
			"SlateCore",
			"ToolMenus",
			"UnrealEd"
		});

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			PublicSystemLibraries.AddRange(new string[]
			{
				"mfplat.lib",
				"mfreadwrite.lib",
				"mfuuid.lib",
				"ole32.lib"
			});
		}
	}
}
