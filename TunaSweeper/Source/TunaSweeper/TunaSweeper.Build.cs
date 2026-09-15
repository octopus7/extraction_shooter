// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System;
using System.IO;
using System.Collections.Generic;
using System.Text;

public class TunaSweeper : ModuleRules
{
	public TunaSweeper(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "AIModule", "UMG", "Slate", "SlateCore", "MediaAssets", "Niagara", "GameplayTags", "PhysicsCore", "ProceduralMeshComponent", "Chaos", "GeometryCollectionEngine", "FieldSystemEngine", "TunaWarpTransition", "MiyakovCharacterSystem" });

		PrivateDependencyModuleNames.AddRange(new string[] { "DLSSBlueprint", "DeveloperSettings", "ImageWrapper", "Json", "NavigationSystem", "OnlineSubsystem", "RenderCore", "RHI", "HTTP" });

		bool bStove = Target.Platform == UnrealTargetPlatform.Win64 && Target.Type == TargetType.Game
			&& (Target.Name == "TunaSweeperStove" || Target.Name == "TunaSweeperStoveDemo");
		PrivateDefinitions.Add("WITH_TUNASWEEPER_STOVE=" + (bStove ? "1" : "0"));
		if (bStove)
		{
			ConfigureStove(Target);
		}

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
	}

	private void ConfigureStove(ReadOnlyTargetRules Target)
	{
		string ProjectRoot = Path.GetFullPath(Path.Combine(ModuleDirectory, "../.."));
		string StoreRoot = Path.GetFullPath(Path.Combine(ProjectRoot, "../store/stove"));
		string SdkRoot = Path.Combine(StoreRoot, "StovePCSDK_Studio_Cpp_3.4.2");
		string CredentialsPath = Path.Combine(StoreRoot, "credentials.env");
		if (!File.Exists(CredentialsPath))
		{
			throw new BuildException("STOVE build requires store/stove/credentials.env.");
		}
		ExternalDependencies.Add(CredentialsPath);
		var Values = new Dictionary<string, string>(StringComparer.Ordinal);
		foreach (string Line in File.ReadAllLines(CredentialsPath))
		{
			string Trimmed = Line.Trim();
			if (Trimmed.Length == 0 || Trimmed.StartsWith("#")) continue;
			int Separator = Trimmed.IndexOf('=');
			if (Separator <= 0) throw new BuildException("Invalid STOVE credentials file format.");
			string Key = Trimmed.Substring(0, Separator).Trim();
			string Value = Trimmed.Substring(Separator + 1).Trim();
			if (Value.Length >= 2 && ((Value[0] == '"' && Value[Value.Length - 1] == '"')
				|| (Value[0] == '\'' && Value[Value.Length - 1] == '\'')))
			{
				Value = Value.Substring(1, Value.Length - 2);
			}
			if (!Values.TryAdd(Key, Value)) throw new BuildException("Duplicate STOVE credential field.");
		}
		foreach (string Key in new[] { "STOVE_GAME_ID", "STOVE_APPLICATION_KEY" })
		{
			if (!Values.TryGetValue(Key, out string Value) || string.IsNullOrWhiteSpace(Value))
				throw new BuildException("Missing STOVE credential field: " + Key);
		}
		// PC SDK 3.4.2 does not require the Application Secret or Product No.
		// Never place credentials in compiler command lines, tracked source or staged config.
		string GeneratedDirectory = Path.Combine(ProjectRoot, "Intermediate", "Stove", Target.Name);
		Directory.CreateDirectory(GeneratedDirectory);
		string HeaderPath = Path.Combine(GeneratedDirectory, "TunaSweeperStoveCredentials.h");
		string Header = "#pragma once\nnamespace TunaSweeperStoveCredentials {\n"
			+ "inline constexpr wchar_t GameId[] = " + WideLiteral(Values["STOVE_GAME_ID"]) + ";\n"
			+ "inline constexpr wchar_t ApplicationKey[] = " + WideLiteral(Values["STOVE_APPLICATION_KEY"]) + ";\n}\n";
		if (!File.Exists(HeaderPath) || File.ReadAllText(HeaderPath) != Header)
			File.WriteAllText(HeaderPath, Header, new UTF8Encoding(false));
		PrivateIncludePaths.Add(GeneratedDirectory);
		PublicSystemIncludePaths.Add(Path.Combine(SdkRoot, "Include"));
		foreach (string Module in new[] { "BaseSDK", "OwnershipSDK" })
		{
			string Library = Path.Combine(SdkRoot, "lib", "x64", Module + ".lib");
			if (!File.Exists(Library)) throw new BuildException("Extract STOVE PC SDK 3.4.2 into store/stove before building.");
			PublicAdditionalLibraries.Add(Library);
		}
		foreach (string Module in new[] { "BaseSDK", "OwnershipSDK", "LogSDK" })
		{
			RuntimeDependencies.Add("$(TargetOutputDir)/" + Module + ".dll",
				Path.Combine(SdkRoot, "dll", "x64", Module + ".dll"), StagedFileType.NonUFS);
		}
	}

	private static string WideLiteral(string Value)
	{
		var Literal = new StringBuilder("L\"");
		foreach (char Character in Value)
		{
			// Fixed-width octal avoids escape injection and greedy hexadecimal escapes.
			if (Character < 32 || Character > 126)
				throw new BuildException("STOVE game ID and application key must contain printable ASCII characters.");
			Literal.Append('\\').Append(Convert.ToString(Character, 8).PadLeft(3, '0'));
		}
		return Literal.Append('"').ToString();
	}
}
