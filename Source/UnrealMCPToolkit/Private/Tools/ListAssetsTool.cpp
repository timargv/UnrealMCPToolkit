// Copyright (c) 2026 Timur Ragimkhanov. Licensed under the MIT License.

#include "Tools/ListAssetsTool.h"

#include "ModelContextProtocolToolResults.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/AssetData.h"
#include "UObject/TopLevelAssetPath.h"
#include "UObject/Class.h"                   // UClass::TryConvertShortTypeNameToPathName
#include "Dom/JsonObject.h"
#include "JsonDomBuilder.h"

namespace
{
	/** Upper bound on listed paths so a huge /Game query can't flood the result. */
	constexpr int32 MaxListedAssets = 1000;
}

FString FListAssetsTool::GetName() const
{
	return TEXT("list_assets");
}

FString FListAssetsTool::GetDescription() const
{
	return TEXT(
		"Lists asset object paths from the Asset Registry. Optional 'path' (content "
		"path searched recursively, default \"/Game\") and 'class' (filter by class, "
		"either a short name like \"StaticMesh\" or a full path like "
		"\"/Script/Engine.StaticMesh\"; subclasses are included). Output is capped to "
		"keep the result small.");
}

TSharedPtr<FJsonObject> FListAssetsTool::GetInputJsonSchema() const
{
	FJsonDomBuilder::FObject PathProp;
	PathProp.Set(TEXT("type"), TEXT("string"));
	PathProp.Set(TEXT("description"), TEXT("Content path to search recursively, e.g. \"/Game/Meshes\". Default \"/Game\"."));

	FJsonDomBuilder::FObject ClassProp;
	ClassProp.Set(TEXT("type"), TEXT("string"));
	ClassProp.Set(TEXT("description"),
		TEXT("Optional class filter: short name (\"StaticMesh\") or full path (\"/Script/Engine.StaticMesh\"). Subclasses included."));

	FJsonDomBuilder::FObject Props;
	Props.Set(TEXT("path"), PathProp);
	Props.Set(TEXT("class"), ClassProp);

	FJsonDomBuilder::FObject Schema;
	Schema.Set(TEXT("type"), TEXT("object"));
	Schema.Set(TEXT("properties"), Props);

	return Schema.AsJsonObject().ToSharedPtr();
}

FModelContextProtocolToolResult FListAssetsTool::Run(const TSharedPtr<FJsonObject>& Params)
{
	using namespace UE::ModelContextProtocol;

	FString SearchPath = TEXT("/Game");
	FString ClassName;
	if (Params.IsValid())
	{
		FString PathValue;
		if (Params->TryGetStringField(TEXT("path"), PathValue) && !PathValue.IsEmpty())
		{
			SearchPath = PathValue;
		}
		Params->TryGetStringField(TEXT("class"), ClassName);
	}

	IAssetRegistry& AssetRegistry = FAssetRegistryModule::GetRegistry();
	// Ensure the async scan has finished before querying (headless-safe).
	AssetRegistry.WaitForCompletion();

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Add(FName(*SearchPath));

	if (!ClassName.IsEmpty())
	{
		FTopLevelAssetPath ClassPath;
		if (ClassName.Contains(TEXT(".")))
		{
			// Already a full path, e.g. "/Script/Engine.StaticMesh".
			ClassPath = FTopLevelAssetPath(ClassName);
		}
		else
		{
			// Resolve a short name like "StaticMesh" against loaded classes.
			ClassPath = UClass::TryConvertShortTypeNameToPathName(UClass::StaticClass(), ClassName);
		}

		if (!ClassPath.IsValid())
		{
			return MakeErrorResult(FString::Printf(
				TEXT("Could not resolve class '%s'. Use a loaded short name or a full path like \"/Script/Engine.StaticMesh\"."),
				*ClassName));
		}

		Filter.ClassPaths.Add(ClassPath);
		Filter.bRecursiveClasses = true; // include subclasses
	}

	TArray<FAssetData> Found;
	AssetRegistry.GetAssets(Filter, Found);

	const int32 TotalCount = Found.Num();
	const int32 ListedCount = FMath::Min(TotalCount, MaxListedAssets);

	FString Out;
	Out.Reserve(ListedCount * 64);
	for (int32 Index = 0; Index < ListedCount; ++Index)
	{
		Out += Found[Index].GetObjectPathString();
		Out += TEXT("\n");
	}

	FString Header = FString::Printf(TEXT("%d asset(s) found under '%s'"), TotalCount, *SearchPath);
	if (!ClassName.IsEmpty())
	{
		Header += FString::Printf(TEXT(" of class '%s'"), *ClassName);
	}
	if (ListedCount < TotalCount)
	{
		Header += FString::Printf(TEXT(" (showing first %d)"), ListedCount);
	}

	return MakeTextResult(FString::Printf(TEXT("%s\n%s"), *Header, *Out));
}
