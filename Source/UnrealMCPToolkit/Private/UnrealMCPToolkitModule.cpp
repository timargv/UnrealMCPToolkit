// Copyright (c) 2026 Timur Ragimkhanov. Licensed under the MIT License.

#include "Modules/ModuleManager.h"

#include "IModelContextProtocolModule.h"
#include "IModelContextProtocolTool.h"

#include "Tools/ExecutePythonTool.h"
#include "Tools/ExecuteConsoleCommandTool.h"
#include "Tools/TakeScreenshotTool.h"
#include "Tools/SaveAllTool.h"
#include "Tools/ListAssetsTool.h"
#include "Tools/GetOutputLogTool.h"
#include "MCPLogRingBuffer.h"

DEFINE_LOG_CATEGORY_STATIC(LogUnrealMCPToolkit, Log, All);

/** Module name of Epic's built-in Model Context Protocol plugin. */
static const FName GModelContextProtocolModuleName(TEXT("ModelContextProtocol"));

/**
 * Editor module for the Unreal MCP Toolkit.
 *
 * Registers the toolkit's MCP tools with Epic's built-in Model Context Protocol
 * server. Because the MCP server can clear its tool list at any time (the
 * ModelContextProtocol.RefreshTools console command calls Tools.Reset()), the
 * module re-registers its tools on every OnRefreshTools broadcast. Registration
 * is idempotent so the same code path serves startup and refresh alike.
 *
 * Load ordering: the ModelContextProtocol module is not guaranteed to be loaded
 * before this one. If it isn't, the module subscribes to FModuleManager's
 * OnModulesChanged and completes registration once ModelContextProtocol reports
 * ModuleLoaded, then unbinds itself.
 */
class FUnrealMCPToolkitModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		// Install the log ring buffer first so it captures lines emitted from
		// here on, and is ready before get_output_log can ever be called.
		FMCPLogRingBuffer::Install();

		if (IModelContextProtocolModule::Get() != nullptr)
		{
			// MCP module already loaded: register now and bind OnRefreshTools.
			BindAndRegister();
		}
		else
		{
			// MCP module not loaded yet, and load order is not guaranteed.
			// Wait for the module manager to report it has loaded.
			UE_LOG(LogUnrealMCPToolkit, Log,
				TEXT("ModelContextProtocol module not loaded yet; deferring tool registration until it loads."));

			OnModulesChangedHandle = FModuleManager::Get().OnModulesChanged().AddRaw(
				this, &FUnrealMCPToolkitModule::OnModulesChanged);
		}
	}

	virtual void ShutdownModule() override
	{
		// Unbind the deferred-load watcher if it is still pending.
		if (OnModulesChangedHandle.IsValid())
		{
			FModuleManager::Get().OnModulesChanged().Remove(OnModulesChangedHandle);
			OnModulesChangedHandle.Reset();
		}

		if (IModelContextProtocolModule* Module = IModelContextProtocolModule::Get())
		{
			if (OnRefreshToolsHandle.IsValid())
			{
				Module->OnRefreshTools().Remove(OnRefreshToolsHandle);
			}

			for (const TSharedRef<IModelContextProtocolTool>& Tool : RegisteredTools)
			{
				Module->RemoveTool(Tool);
			}
		}

		OnRefreshToolsHandle.Reset();
		RegisteredTools.Reset();

		FMCPLogRingBuffer::Uninstall();
	}

private:
	/**
	 * Handler for FModuleManager::OnModulesChanged. Fires once the
	 * ModelContextProtocol module finishes loading; registers tools and then
	 * unbinds itself so it never runs twice.
	 */
	void OnModulesChanged(FName ModuleName, EModuleChangeReason Reason)
	{
		if (Reason == EModuleChangeReason::ModuleLoaded && ModuleName == GModelContextProtocolModuleName)
		{
			// Unbind first so re-entrancy / a later load event can't double-fire us.
			FModuleManager::Get().OnModulesChanged().Remove(OnModulesChangedHandle);
			OnModulesChangedHandle.Reset();

			BindAndRegister();
		}
	}

	/**
	 * Binds OnRefreshTools (once) and performs the initial registration.
	 * Safe to call only when IModelContextProtocolModule::Get() is non-null.
	 */
	void BindAndRegister()
	{
		IModelContextProtocolModule* Module = IModelContextProtocolModule::Get();
		if (!Module)
		{
			return;
		}

		// Bind OnRefreshTools exactly once (idempotent guard).
		if (!OnRefreshToolsHandle.IsValid())
		{
			OnRefreshToolsHandle = Module->OnRefreshTools().AddRaw(this, &FUnrealMCPToolkitModule::RegisterTools);
		}

		RegisterTools();
	}

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
		AddTool(MakeShared<FTakeScreenshotTool>());
		AddTool(MakeShared<FSaveAllTool>());
		AddTool(MakeShared<FListAssetsTool>());
		AddTool(MakeShared<FGetOutputLogTool>());

		UE_LOG(LogUnrealMCPToolkit, Log, TEXT("Registered %d MCP tool(s)."), RegisteredTools.Num());
	}

	/** Tool instances kept alive for the lifetime of the module. */
	TArray<TSharedRef<IModelContextProtocolTool>> RegisteredTools;

	/** Handle for the OnRefreshTools delegate binding. */
	FDelegateHandle OnRefreshToolsHandle;

	/** Handle for the deferred FModuleManager::OnModulesChanged binding (pending MCP load). */
	FDelegateHandle OnModulesChangedHandle;
};

IMPLEMENT_MODULE(FUnrealMCPToolkitModule, UnrealMCPToolkit)
