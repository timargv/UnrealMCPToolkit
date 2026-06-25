// Copyright (c) 2026 Timur Ragimkhanov. Licensed under the MIT License.

#include "MCPLogRingBuffer.h"

#include "Misc/OutputDevice.h"
#include "Misc/OutputDeviceRedirector.h"   // GLog
#include "Misc/OutputDeviceHelper.h"       // FOutputDeviceHelper::FormatLogLine
#include "Misc/ScopeLock.h"
#include "HAL/CriticalSection.h"

namespace
{
	/** Thread-safe ring buffer of the last N formatted log lines. */
	class FRingLogDevice final : public FOutputDevice
	{
	public:
		explicit FRingLogDevice(int32 InCapacity)
			: Capacity(FMath::Max(1, InCapacity))
		{
		}

		virtual void Serialize(const TCHAR* Message, ELogVerbosity::Type Verbosity, const FName& Category) override
		{
			// Re-entrancy guard: never record a line emitted from inside this sink.
			if (bWriting)
			{
				return;
			}
			TGuardValue<bool> Guard(bWriting, true);

			FString Line = FOutputDeviceHelper::FormatLogLine(Verbosity, Category, Message, GPrintLogTimes);

			FScopeLock Lock(&Crit);
			if (Lines.Num() >= Capacity)
			{
				Lines.RemoveAt(0, 1, EAllowShrinking::No);
			}
			Lines.Add(MoveTemp(Line));
		}

		FString GetRecent(int32 N) const
		{
			FScopeLock Lock(&Crit);
			const int32 Count = FMath::Clamp(N, 1, Lines.Num());
			const int32 Start = Lines.Num() - Count;

			FString Out;
			Out.Reserve(Count * 96);
			for (int32 Index = Start; Index < Lines.Num(); ++Index)
			{
				Out += Lines[Index];
				Out += TEXT("\n");
			}
			return Out;
		}

	private:
		mutable FCriticalSection Crit;
		TArray<FString> Lines;
		int32 Capacity;
		bool bWriting = false;
	};

	/** The single installed device, owned for the lifetime of the module. */
	TUniquePtr<FRingLogDevice> GRingLogDevice;
}

void FMCPLogRingBuffer::Install(int32 Capacity)
{
	if (GRingLogDevice.IsValid())
	{
		return;
	}

	GRingLogDevice = MakeUnique<FRingLogDevice>(Capacity);
	if (GLog)
	{
		// In 5.7+ AddOutputDevice auto-serializes the existing backlog into the
		// new device, so prior lines are seeded if backlog was enabled.
		GLog->AddOutputDevice(GRingLogDevice.Get());
	}
}

void FMCPLogRingBuffer::Uninstall()
{
	if (!GRingLogDevice.IsValid())
	{
		return;
	}

	if (GLog)
	{
		GLog->RemoveOutputDevice(GRingLogDevice.Get());
	}
	GRingLogDevice.Reset();
}

bool FMCPLogRingBuffer::IsInstalled()
{
	return GRingLogDevice.IsValid();
}

FString FMCPLogRingBuffer::GetRecentLines(int32 N)
{
	if (!GRingLogDevice.IsValid())
	{
		return TEXT("(log ring buffer is not installed)");
	}
	return GRingLogDevice->GetRecent(N);
}
