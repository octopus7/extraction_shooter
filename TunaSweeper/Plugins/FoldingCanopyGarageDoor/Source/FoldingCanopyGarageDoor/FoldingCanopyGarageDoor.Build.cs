using UnrealBuildTool;

public class FoldingCanopyGarageDoor : ModuleRules
{
	public FoldingCanopyGarageDoor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine"
		});
	}
}
