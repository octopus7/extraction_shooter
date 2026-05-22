using UnrealBuildTool;

public class SogSplat : ModuleRules
{
	public SogSplat(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"FreeImage",
			"Json",
			"libzip",
			"RenderCore",
			"RHI"
		});
	}
}
