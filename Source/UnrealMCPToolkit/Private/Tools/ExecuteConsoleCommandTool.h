// Copyright (c) 2026 Timur Ragimkhanov. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "IModelContextProtocolTool.h"

/**
 * MCP tool "execute_console_command".
 *
 * Executes a single Unreal Editor console command (for example "stat fps" or
 * "obj list") and returns whatever the command wrote to its output device.
 *
 * Input schema: an object with a single required string property "command"
 * holding the console command line.
 *
 * WARNING: console commands can mutate editor and engine state. This is a
 * development-only convenience and must not be shipped in a production build.
 */
class FExecuteConsoleCommandTool : public IModelContextProtocolTool
{
public:
	//~ Begin IModelContextProtocolTool interface
	virtual FString GetName() const override;
	virtual FString GetDescription() const override;
	virtual TSharedPtr<FJsonObject> GetInputJsonSchema() const override;
	virtual FModelContextProtocolToolResult Run(const TSharedPtr<FJsonObject>& Params) override;
	//~ End IModelContextProtocolTool interface
};
