// Copyright (c) 2026 Timur Ragimkhanov. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

/**
 * Process-wide, capped (ring) buffer of recent engine log lines.
 *
 * The engine exposes no clean public "give me the last N log lines" API: GLog's
 * backlog access is deprecated since 5.7, FBufferedOutputDevice is unbounded with
 * no string getter, and the editor OutputLog module is Slate-only. So this
 * toolkit installs its own tiny FOutputDevice on GLog at module startup, keeping
 * a fixed number of recent lines under a lock, and removes it at shutdown.
 *
 * The get_output_log tool reads back from here. Installation is owned by the
 * module; the buffer is a singleton so the tool needs no module reference.
 */
class FMCPLogRingBuffer
{
public:
	/** Default ring capacity in lines. */
	static constexpr int32 DefaultCapacity = 4096;

	/** Adds the ring-buffer output device to GLog. Idempotent. Call from StartupModule. */
	static void Install(int32 Capacity = DefaultCapacity);

	/** Removes the output device from GLog. Idempotent. Call from ShutdownModule. */
	static void Uninstall();

	/** True while the device is installed on GLog. */
	static bool IsInstalled();

	/**
	 * Returns the most recent N captured lines, newline-joined (oldest first).
	 * Returns a short notice string if the buffer is not installed.
	 */
	static FString GetRecentLines(int32 N);
};
