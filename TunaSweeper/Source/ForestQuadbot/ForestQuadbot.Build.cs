using UnrealBuildTool;

public class ForestQuadbot : ModuleRules
{
	public ForestQuadbot(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "TunaSweeper" });
		PrivateDependencyModuleNames.Add("MiyakovCharacterSystem");
	}
}
