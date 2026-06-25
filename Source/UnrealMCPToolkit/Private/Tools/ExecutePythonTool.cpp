// Copyright (c) 2026 Timur Ragimkhanov. Licensed under the MIT License.

#include "Tools/ExecutePythonTool.h"

#include "ModelContextProtocolToolResults.h"
#include "IPythonScriptPlugin.h"   // pulls in PythonScriptTypes.h
#include "Dom/JsonObject.h"
#include "JsonDomBuilder.h"

FString FExecutePythonTool::GetName() const
{
	return TEXT("execute_python");
}

FString FExecutePythonTool::GetDescription() const
{
	return TEXT(
		"Executes an arbitrary Python script inside the Unreal Editor using the "
		"Python Editor Script Plugin and returns the captured result and log "
		"output. The script runs with full editor access, so it can use the "
		"'unreal' module to inspect and modify the project, assets and the open "
		"level. Provide the source via the required 'python' string parameter. "
		"Multi-statement scripts are supported.");
}

TSharedPtr<FJsonObject> FExecutePythonTool::GetInputJsonSchema() const
{
	FJsonDomBuilder::FObject PythonProp;
	PythonProp.Set(TEXT("type"), TEXT("string"));
	PythonProp.Set(TEXT("description"), TEXT("Python source code to execute in the editor."));

	FJsonDomBuilder::FObject Props;
	Props.Set(TEXT("python"), PythonProp);

	FJsonDomBuilder::FArray Required;
	Required.Add(TEXT("python"));

	FJsonDomBuilder::FObject Schema;
	Schema.Set(TEXT("type"), TEXT("object"));
	Schema.Set(TEXT("properties"), Props);
	Schema.Set(TEXT("required"), Required);

	return Schema.AsJsonObject().ToSharedPtr();
}

FModelContextProtocolToolResult FExecutePythonTool::Run(const TSharedPtr<FJsonObject>& Params)
{
	using namespace UE::ModelContextProtocol;

	FString PythonScript;
	if (!Params.IsValid() || !Params->TryGetStringField(TEXT("python"), PythonScript) || PythonScript.IsEmpty())
	{
		return MakeErrorResult(TEXT("Missing required 'python' string parameter."));
	}

	IPythonScriptPlugin* PythonPlugin = IPythonScriptPlugin::Get();
	if (!PythonPlugin)
	{
		return MakeErrorResult(TEXT("PythonScriptPlugin module is not loaded. Enable the 'Python Editor Script Plugin'."));
	}

	if (!PythonPlugin->IsPythonAvailable())
	{
		// Try to bring Python online on demand before giving up.
		if (!PythonPlugin->ForceEnablePythonAtRuntime() || !PythonPlugin->IsPythonInitialized())
		{
			return MakeErrorResult(TEXT("Python is not available or not initialized. Enable Python support in the editor settings."));
		}
	}

	FPythonCommandEx Command;
	Command.Command            = PythonScript;
	Command.ExecutionMode      = EPythonCommandExecutionMode::ExecuteFile;   // allow multi-statement scripts
	Command.FileExecutionScope = EPythonFileExecutionScope::Private;         // isolated globals/locals
	Command.Flags              = EPythonCommandFlags::Unattended;            // suppress interactive UI

	const bool bSuccess = PythonPlugin->ExecPythonCommandEx(Command);

	// Assemble a single human-readable text payload from the command result and
	// every captured log line. On failure CommandResult holds the exception trace.
	FString Output;

	if (!Command.CommandResult.IsEmpty() && Command.CommandResult != TEXT("None"))
	{
		Output += FString::Printf(TEXT("Result: %s\n"), *Command.CommandResult);
	}

	for (const FPythonLogOutputEntry& Entry : Command.LogOutput)
	{
		switch (Entry.Type)
		{
		case EPythonLogOutputType::Error:
			Output += FString::Printf(TEXT("[Error] %s\n"), *Entry.Output);
			break;
		case EPythonLogOutputType::Warning:
			Output += FString::Printf(TEXT("[Warning] %s\n"), *Entry.Output);
			break;
		default:
			Output += FString::Printf(TEXT("%s\n"), *Entry.Output);
			break;
		}
	}

	if (!bSuccess)
	{
		if (Output.IsEmpty())
		{
			Output = TEXT("Python execution failed with no captured output.");
		}
		return MakeErrorResult(Output);
	}

	if (Output.IsEmpty())
	{
		Output = TEXT("Python executed successfully (no output).");
	}
	return MakeTextResult(Output);
}
