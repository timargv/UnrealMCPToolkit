// Copyright (c) 2026 Timur Ragimkhanov. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "IModelContextProtocolTool.h"

/**
 * MCP tool "get_output_log".
 *
 * Returns the most recent engine log lines captured by the toolkit's log ring
 * buffer (installed on GLog at module startup).
 *
 * Input schema (all optional):
 *   - "lines" (integer) number of recent lines to return, default 100, clamped 1..2000.
 */
class FGetOutputLogTool : public IModelContextProtocolTool
{
public:
	//~ Begin IModelContextProtocolTool interface
	virtual FString GetName() const override;
	virtual FString GetDescription() const override;
	virtual TSharedPtr<FJsonObject> GetInputJsonSchema() const override;
	virtual FModelContextProtocolToolResult Run(const TSharedPtr<FJsonObject>& Params) override;
	//~ End IModelContextProtocolTool interface
};
