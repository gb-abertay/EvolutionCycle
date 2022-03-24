// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class AntPlusReader : ModuleRules
{

	private string ModulePath
	{
		get { return ModuleDirectory; }
	}

	private string ThirdPartyPath
	{
		get { return Path.GetFullPath(Path.Combine(ModulePath, "../ThirdPartyLibraries/")); }
	}

	public AntPlusReader(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);


		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);


		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				// ... add other public dependencies that you statically link with here ...
			}
			);


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"Projects",
				// ... add private dependencies that you statically link with here ...	
			}
			);


		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);

		PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "ANT_LIB/Libraries/ANT_LIB.lib"));
		PublicIncludePaths.Add(Path.Combine(ThirdPartyPath, "ANT_LIB/Includes"));
		Definitions.Add("WITH_ANT_LIB_BINDING=1");

		PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "POWERRECORDER_LIB/Libraries/PowerRecordingLib.lib"));
		PublicIncludePaths.Add(Path.Combine(ThirdPartyPath, "POWERRECORDER_LIB/Includes"));
		Definitions.Add("WITH_POWERRECORDER_LIB_BINDING=1");
	}
}