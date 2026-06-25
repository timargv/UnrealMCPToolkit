// Copyright (c) 2026 Timur Ragimkhanov. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "IModelContextProtocolTool.h"

/**
 * MCP tool "save_all".
 *
 * Saves every dirty map and content package in the open project without showing
 * a dialog (equivalent to File > Save All). Takes no parameters.
 */
class FSaveAllTool : public IModelContextProtocolTool
{
public:
	//~ Begin IModelContextProtocolTool interface
	virtual FString GetName() const override;
	virtual FString GetDescription() const override;
	virtual TSharedPtr<FJsonObject> GetInputJsonSchema() const override;
	virtual FModelContextProtocolToolResult Run(const TSharedPtr<FJsonObject>& Params) override;
	//~ End IModelContextProtocolTool interface
};
