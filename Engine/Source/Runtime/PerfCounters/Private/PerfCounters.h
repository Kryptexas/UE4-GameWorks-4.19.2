// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PerfCountersModule.h"
#include "Json.h"

DECLARE_LOG_CATEGORY_EXTERN(LogPerfCounters, Verbose, All);

class FPerfCounters : public IPerfCounters
{
	struct FPerfCounter
	{
		/** Name of this counter*/
		FString		Name;
		
		/** Pointer to the shared memory */
		void *		Pointer;
		
		/** Size of the counter in bytes*/
		SIZE_T		SizeOf;

		/** Initializes from JSON object. */
		bool		Initialize(const FJsonObject& JsonObject);
	};

	/** Unique name of this instance */
	FString UniqueInstanceId;

	/** Map of all known performance counters */
	TMap<FString, FPerfCounter>  PerfCounterMap;

	/** Shared memory */
	FPlatformMemory::FSharedMemoryRegion * SharedMemory;

public:

	FPerfCounters(const FString& InUniqueInstanceId)
		:	UniqueInstanceId(InUniqueInstanceId)
		,	SharedMemory(nullptr)
	{
	}

	virtual ~FPerfCounters();

	/** Initializes this instance from JSON config. */
	bool Initialize(const FString& JsonText);

	// IPerfCounters interface
	const FString& GetInstanceName() const override { return UniqueInstanceId; }
	void SetPOD(const FString& Name, const void* Ptr, SIZE_T Size) override;
	// IPerfCounters interface end
};

