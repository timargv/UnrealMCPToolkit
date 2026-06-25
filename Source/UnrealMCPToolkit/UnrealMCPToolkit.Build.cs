// Copyright (c) 2026 Timur Ragimkhanov. Licensed under the MIT License.

using UnrealBuildTool;

public class UnrealMCPToolkit : ModuleRules
{
	public UnrealMCPToolkit(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"Json"
			});

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"JsonUtilities",
				"UnrealEd",
				"ModelContextProtocol",
				"PythonScriptPlugin"
			});
	}
}
