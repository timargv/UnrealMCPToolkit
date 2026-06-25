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
				"UnrealEd",            // editor + viewport access, save_all
				"ModelContextProtocol",
				"PythonScriptPlugin",
				"AssetRegistry",       // list_assets
				"ImageCore",           // take_screenshot: FImage/FImageView resize
				"ImageWrapper"         // take_screenshot: JPEG/PNG encoding (via FImageUtils)
			});
	}
}
