// Copyright (c) 2026 Timur Ragimkhanov. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "IModelContextProtocolTool.h"

/**
 * MCP tool "list_assets".
 *
 * Queries the Asset Registry and returns the object paths of assets under a
 * content path, optionally filtered by class.
 *
 * Input schema (all optional):
 *   - "path"  (string) content path to search recursively, default "/Game".
 *   - "class" (string) class to filter by, either a short name ("StaticMesh")
 *             or a full path ("/Script/Engine.StaticMesh"); subclasses included.
 */
class FListAssetsTool : public IModelContextProtocolTool
{
public:
	//~ Begin IModelContextProtocolTool interface
	virtual FString GetName() const override;
	virtual FString GetDescription() const override;
	virtual TSharedPtr<FJsonObject> GetInputJsonSchema() const override;
	virtual FModelContextProtocolToolResult Run(const TSharedPtr<FJsonObject>& Params) override;
	//~ End IModelContextProtocolTool interface
};
