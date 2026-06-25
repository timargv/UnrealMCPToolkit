// Copyright (c) 2026 Timur Ragimkhanov. Licensed under the MIT License.

#include "Modules/ModuleManager.h"

#include "IModelContextProtocolModule.h"
#include "IModelContextProtocolTool.h"

#include "Tools/ExecutePythonTool.h"
#include "Tools/ExecuteConsoleCommandTool.h"

DEFINE_LOG_CATEGORY_STATIC(LogUnrealMCPToolkit, Log, All);

/**
 * Editor module for the Unreal MCP Toolkit.
 *
 * Registers the "execute_python" and "execute_console_command" tools with
 * Epic's built-in Model Context Protocol server. Because the MCP server can
 * clear its tool list at any time (the ModelContextProtocol.RefreshTools
 * console command calls Tools.Reset()), the module re-registers its tools on
 * every OnRefreshTools broadcast. Registration is idempotent so the same code
 * path serves both startup and refresh.
 */
class FUnrealMCPToolkitModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		// Register now (if MCP is present)...
		RegisterTools();

		// ...and again every time the server clears its tool list.
		if (IModelContextProtocolModule* Module = IModelContextProtocolModule::Get())
		{
			OnRefreshToolsHandle = Module->OnRefreshTools().AddRaw(this, &FUnrealMCPToolkitModule::RegisterTools);
		}
		else
		{
			UE_LOG(LogUnrealMCPToolkit, Log,
				TEXT("ModelContextProtocol module not loaded; tools will not be registered. Enable the 'Model Context Protocol' plugin."));
		}
	}

	virtual void ShutdownModule() override
	{
		if (IModelContextProtocolModule* Module = IModelContextProtocolModule::Get())
		{
			Module->OnRefreshTools().Remove(OnRefreshToolsHandle);

			for (const TSharedRef<IModelContextProtocolTool>& Tool : RegisteredTools)
			{
				Module->RemoveTool(Tool);
			}
		}

		OnRefreshToolsHandle.Reset();
		RegisteredTools.Reset();
	}

private:
	/**
	 * Idempotent registration: removes any previously added tools, builds fresh
	 * instances and re-adds them. Safe to call on startup and on OnRefreshTools.
	 */
	void RegisterTools()
	{
		IModelContextProtocolModule* Module = IModelContextProtocolModule::Get();
		if (!Module)
		{
			// MCP plugin disabled / not loaded — degrade gracefully.
			return;
		}

		for (const TSharedRef<IModelContextProtocolTool>& Tool : RegisteredTools)
		{
			Module->RemoveTool(Tool);
		}
		RegisteredTools.Reset();

		auto AddTool = [Module, this](const TSharedRef<IModelContextProtocolTool>& Tool)
		{
			if (Module->AddTool(Tool)) // returns false (and logs) on name collision
			{
				RegisteredTools.Add(Tool);
			}
			else
			{
				UE_LOG(LogUnrealMCPToolkit, Warning,
					TEXT("Failed to register MCP tool '%s' (name may already be in use)."), *Tool->GetName());
			}
		};

		AddTool(MakeShared<FExecutePythonTool>());
		AddTool(MakeShared<FExecuteConsoleCommandTool>());

		UE_LOG(LogUnrealMCPToolkit, Log, TEXT("Registered %d MCP tool(s)."), RegisteredTools.Num());
	}

	/** Tool instances kept alive for the lifetime of the module. */
	TArray<TSharedRef<IModelContextProtocolTool>> RegisteredTools;

	/** Handle for the OnRefreshTools delegate binding. */
	FDelegateHandle OnRefreshToolsHandle;
};

IMPLEMENT_MODULE(FUnrealMCPToolkitModule, UnrealMCPToolkit)
