// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class TunaSweeperDemoTarget : TargetRules
{
	public TunaSweeperDemoTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		bOverrideBuildEnvironment = true;
		CustomConfig = "Demo";
		GlobalDefinitions.Add("TUNASWEEPER_DEMO=1");
		GlobalDefinitions.Add("CUSTOM_CONFIG=\"Demo\"");
		GlobalDefinitions.Add("UE_PROJECT_STEAMSHIPPINGID=5158070");
		ExtraModuleNames.Add("TunaSweeper");
	}
}
