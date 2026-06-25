// Copyright (c) 2026 Timur Ragimkhanov. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "IModelContextProtocolTool.h"

/**
 * MCP tool "take_screenshot".
 *
 * Captures the currently active editor viewport, downscales it (aspect
 * preserved) to a caller-chosen maximum width, encodes it as JPEG or PNG and
 * writes it under <Project>/Saved/Screenshots/MCP/.
 *
 * Context safety (by design): the default result is a small TEXT block with the
 * absolute file path, pixel dimensions and file size in KB — never a base64
 * blob. A base64 image content block is attached only when the caller passes
 * "inline": true AND the encoded file is below a hard size cap; otherwise the
 * path is returned with a note to read the file. This prevents a full-resolution
 * screenshot from flooding the model's context window.
 *
 * Input schema (all optional):
 *   - "width"  (integer) max long-edge width in pixels, default 1280, clamped 256..3840.
 *   - "format" (string)  "jpg" or "png", default "jpg".
 *   - "inline" (boolean) attach an inline image block if small enough, default false.
 */
class FTakeScreenshotTool : public IModelContextProtocolTool
{
public:
	//~ Begin IModelContextProtocolTool interface
	virtual FString GetName() const override;
	virtual FString GetDescription() const override;
	virtual TSharedPtr<FJsonObject> GetInputJsonSchema() const override;
	virtual FModelContextProtocolToolResult Run(const TSharedPtr<FJsonObject>& Params) override;
	//~ End IModelContextProtocolTool interface
};
