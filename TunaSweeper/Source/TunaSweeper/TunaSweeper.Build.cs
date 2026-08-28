// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TunaSweeper : ModuleRules
{
	public TunaSweeper(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "AIModule", "UMG", "Slate", "SlateCore", "MediaAssets", "Niagara", "GameplayTags", "PhysicsCore", "ProceduralMeshComponent", "Chaos", "GeometryCollectionEngine", "FieldSystemEngine", "TunaWarpTransition", "MiyakovCharacterSystem" });

		PrivateDependencyModuleNames.AddRange(new string[] { "DLSSBlueprint", "DeveloperSettings", "ImageWrapper", "Json", "NavigationSystem", "OnlineSubsystem", "RenderCore", "RHI" });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
	}
}
