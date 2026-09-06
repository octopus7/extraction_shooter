using UnrealBuildTool;

public class TunaWarpTransitionEditor : ModuleRules
{
	public TunaWarpTransitionEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core",
		});
	}
}
