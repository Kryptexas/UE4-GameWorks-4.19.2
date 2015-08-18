// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

#if USE_SERVER_PERF_COUNTERS

#include "PerfCountersModule.h"

void ENGINE_API PerfCountersSet(const FString& Name, float Val, uint32 Flags)
{
	IPerfCounters* PerfCounters = IPerfCountersModule::Get().GetPerformanceCounters();
	if (PerfCounters)
	{
		PerfCounters->Set(Name, Val, Flags);
	}
}

void ENGINE_API PerfCountersSet(const FString& Name, int32 Val, uint32 Flags)
{
	IPerfCounters* PerfCounters = IPerfCountersModule::Get().GetPerformanceCounters();
	if (PerfCounters)
	{
		PerfCounters->Set(Name, Val, Flags);
	}
}

int32 ENGINE_API PerfCountersIncrement(const FString & Name, int32 Add, int32 DefaultValue, uint32 Flags)
{
	IPerfCounters* PerfCounters = IPerfCountersModule::Get().GetPerformanceCounters();
	if (PerfCounters)
	{
		return PerfCounters->Increment(Name, Add, DefaultValue, Flags);
	}

	return DefaultValue + Add;
}

#endif // USE_SERVER_PERF_COUNTERS
