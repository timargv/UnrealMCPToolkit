// Copyright (c) 2026 Timur Ragimkhanov. Licensed under the MIT License.

#include "Tools/SaveAllTool.h"

#include "ModelContextProtocolToolResults.h"
#include "FileHelpers.h"                     // UEditorLoadingAndSavingUtils
#include "Dom/JsonObject.h"
#include "JsonDomBuilder.h"

FString FSaveAllTool::GetName() const
{
	return TEXT("save_all");
}

FString FSaveAllTool::GetDescription() const
{
	return TEXT(
		"Saves all dirty map and content packages in the open project without "
		"prompting (equivalent to File > Save All). Takes no parameters.");
}

TSharedPtr<FJsonObject> FSaveAllTool::GetInputJsonSchema() const
{
	// No parameters: an empty object schema accepts an empty argument object.
	FJsonDomBuilder::FObject Schema;
	Schema.Set(TEXT("type"), TEXT("object"));
	return Schema.AsJsonObject().ToSharedPtr();
}

FModelContextProtocolToolResult FSaveAllTool::Run(const TSharedPtr<FJsonObject>& Params)
{
	using namespace UE::ModelContextProtocol;

	// SaveDirtyPackages does not prompt (the ...WithDialog variants are the
	// interactive ones). Returns false if any package failed to save.
	const bool bSaved = UEditorLoadingAndSavingUtils::SaveDirtyPackages(
		/*bSaveMapPackages*/    true,
		/*bSaveContentPackages*/ true);

	if (!bSaved)
	{
		return MakeErrorResult(TEXT("SaveDirtyPackages returned false — one or more packages failed to save (check the Output Log)."));
	}

	return MakeTextResult(TEXT("All dirty map and content packages saved."));
}
