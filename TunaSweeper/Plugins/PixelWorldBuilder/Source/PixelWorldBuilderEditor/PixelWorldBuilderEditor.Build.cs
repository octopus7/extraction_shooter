using UnrealBuildTool;

public class PixelWorldBuilderEditor : ModuleRules
{
    public PixelWorldBuilderEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "Landscape" });
        PrivateDependencyModuleNames.AddRange(new[] {
            "UnrealEd", "Slate", "SlateCore", "InputCore", "ToolMenus", "Projects",
            "PropertyEditor", "AssetTools", "AssetRegistry", "ImageCore", "ImageWrapper", "Foliage"
        });
    }
}
