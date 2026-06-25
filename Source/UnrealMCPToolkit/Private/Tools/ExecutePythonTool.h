// Copyright (c) 2026 Timur Ragimkhanov. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "IModelContextProtocolTool.h"

/**
 * MCP tool "execute_python".
 *
 * Executes an arbitrary Python snippet inside the Unreal Editor via the
 * PythonScriptPlugin and returns the captured result and log output to the
 * MCP client.
 *
 * Input schema: an object with a single required string property "python"
 * containing the Python source to run.
 *
 * WARNING: this runs arbitrary code with full editor privileges. It is a
 * development-only convenience and must not be shipped in a production build.
 */
class FExecutePythonTool : public IModelContextProtocolTool
{
public:
	//~ Begin IModelContextProtocolTool interface
	virtual FString GetName() const override;
	virtual FString GetDescription() const override;
	virtual TSharedPtr<FJsonObject> GetInputJsonSchema() const override;
	virtual FModelContextProtocolToolResult Run(const TSharedPtr<FJsonObject>& Params) override;
	//~ End IModelContextProtocolTool interface
};
