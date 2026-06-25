// Copyright (c) 2026 Timur Ragimkhanov. Licensed under the MIT License.

#include "Tools/ExecuteConsoleCommandTool.h"

#include "ModelContextProtocolToolResults.h"
#include "Editor.h"                          // GEditor
#include "Engine/Engine.h"                   // GEngine
#include "Misc/StringOutputDevice.h"         // FStringOutputDevice
#include "Dom/JsonObject.h"
#include "JsonDomBuilder.h"

FString FExecuteConsoleCommandTool::GetName() const
{
	return TEXT("execute_console_command");
}

FString FExecuteConsoleCommandTool::GetDescription() const
{
	return TEXT(
		"Executes a single Unreal Editor console command and returns its captured "
		"text output. Useful for stat commands (e.g. 'stat fps'), object queries "
		"(e.g. 'obj list'), and any other registered console command. Provide the "
		"command line via the required 'command' string parameter.");
}

TSharedPtr<FJsonObject> FExecuteConsoleCommandTool::GetInputJsonSchema() const
{
	FJsonDomBuilder::FObject CommandProp;
	CommandProp.Set(TEXT("type"), TEXT("string"));
	CommandProp.Set(TEXT("description"), TEXT("The console command line to execute, e.g. \"stat fps\"."));

	FJsonDomBuilder::FObject Props;
	Props.Set(TEXT("command"), CommandProp);

	FJsonDomBuilder::FArray Required;
	Required.Add(TEXT("command"));

	FJsonDomBuilder::FObject Schema;
	Schema.Set(TEXT("type"), TEXT("object"));
	Schema.Set(TEXT("properties"), Props);
	Schema.Set(TEXT("required"), Required);

	return Schema.AsJsonObject().ToSharedPtr();
}

FModelContextProtocolToolResult FExecuteConsoleCommandTool::Run(const TSharedPtr<FJsonObject>& Params)
{
	using namespace UE::ModelContextProtocol;

	FString Command;
	if (!Params.IsValid() || !Params->TryGetStringField(TEXT("command"), Command) || Command.IsEmpty())
	{
		return MakeErrorResult(TEXT("Missing required 'command' string parameter."));
	}

	// FStringOutputDevice accumulates everything the command writes into a single
	// FString, which we then return to the MCP client.
	FStringOutputDevice CapturedOutput;
	CapturedOutput.SetAutoEmitLineTerminator(true);

	bool bExecResult = false;
	if (GEditor)
	{
		UWorld* World = GEditor->GetEditorWorldContext().World();
		bExecResult = GEditor->Exec(World, *Command, CapturedOutput);
	}
	else if (GEngine)
	{
		bExecResult = GEngine->Exec(nullptr, *Command, CapturedOutput);
	}
	else
	{
		return MakeErrorResult(TEXT("No editor or engine instance available to execute the command."));
	}

	FString Output = MoveTemp(CapturedOutput); // FStringOutputDevice derives from FString
	Output.TrimEndInline();

	FString Message = FString::Printf(
		TEXT("Console command '%s' %s."),
		*Command,
		bExecResult ? TEXT("executed") : TEXT("was not handled"));

	if (!Output.IsEmpty())
	{
		Message += FString::Printf(TEXT("\n%s"), *Output);
	}

	return MakeTextResult(Message);
}
