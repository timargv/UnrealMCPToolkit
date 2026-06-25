// Copyright (c) 2026 Timur Ragimkhanov. Licensed under the MIT License.

#include "Tools/GetOutputLogTool.h"

#include "ModelContextProtocolToolResults.h"
#include "MCPLogRingBuffer.h"
#include "Dom/JsonObject.h"
#include "JsonDomBuilder.h"

namespace
{
	constexpr int32 DefaultLines = 100;
	constexpr int32 MinLines     = 1;
	constexpr int32 MaxLines     = 2000;
}

FString FGetOutputLogTool::GetName() const
{
	return TEXT("get_output_log");
}

FString FGetOutputLogTool::GetDescription() const
{
	return TEXT(
		"Returns the most recent Unreal Editor log lines captured since the editor "
		"started. Optional 'lines' parameter sets how many recent lines to return "
		"(default 100, clamped 1..2000).");
}

TSharedPtr<FJsonObject> FGetOutputLogTool::GetInputJsonSchema() const
{
	FJsonDomBuilder::FObject LinesProp;
	LinesProp.Set(TEXT("type"), TEXT("integer"));
	LinesProp.Set(TEXT("description"), TEXT("Number of recent log lines to return. Default 100, clamped 1..2000."));

	FJsonDomBuilder::FObject Props;
	Props.Set(TEXT("lines"), LinesProp);

	FJsonDomBuilder::FObject Schema;
	Schema.Set(TEXT("type"), TEXT("object"));
	Schema.Set(TEXT("properties"), Props);

	return Schema.AsJsonObject().ToSharedPtr();
}

FModelContextProtocolToolResult FGetOutputLogTool::Run(const TSharedPtr<FJsonObject>& Params)
{
	using namespace UE::ModelContextProtocol;

	int32 Lines = DefaultLines;
	if (Params.IsValid())
	{
		double LinesValue = 0.0;
		if (Params->TryGetNumberField(TEXT("lines"), LinesValue))
		{
			Lines = FMath::Clamp(FMath::RoundToInt(LinesValue), MinLines, MaxLines);
		}
	}

	FString Recent = FMCPLogRingBuffer::GetRecentLines(Lines);
	Recent.TrimEndInline();

	if (Recent.IsEmpty())
	{
		return MakeTextResult(TEXT("(no log lines captured yet)"));
	}

	return MakeTextResult(Recent);
}
