// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "PerfCounters.h"

class FPerfCountersModule : public IPerfCountersModule
{
private:

	/** All created perf counter instances from this module */
	FPerfCounters* PerfCountersSingleton;

public:

	FPerfCountersModule() : 
		PerfCountersSingleton(NULL)
	{}

	void ShutdownModule()
	{
		if (PerfCountersSingleton)
		{
			delete PerfCountersSingleton;
			PerfCountersSingleton = nullptr;
		}
	}

	virtual bool SupportsDynamicReloading() override
	{
		return false;
	}

	virtual bool SupportsAutomaticShutdown() override
	{
		return false;
	}

	IPerfCounters * GetPerformanceCounters() const
	{
		return PerfCountersSingleton;
	}

	IPerfCounters * CreatePerformanceCounters(const FString& JsonConfigFilename, const FString& UniqueInstanceId) override
	{
		if (PerfCountersSingleton)
		{
			UE_LOG(LogPerfCounters, Warning, TEXT("CreatePerformanceCounters: instance already exists, new instance not created."));
			return PerfCountersSingleton;
		}

		const FString JsonConfigFullPath = FPaths::GameDir() + TEXT("Config/") + JsonConfigFilename;

		FString JSonText;
		if (!FFileHelper::LoadFileToString(JSonText, *JsonConfigFullPath))
		{
			UE_LOG(LogPerfCounters, Warning, TEXT("CreatePerformanceCounters: failed to find configuration file for perfcounters: %s"), *JsonConfigFullPath);
			return nullptr;
		}

		FString InstanceUID = UniqueInstanceId;
		if (InstanceUID.IsEmpty())
		{
			InstanceUID = FString::Printf(TEXT("perfcounters-of-pid-%d"), FPlatformProcess::GetCurrentProcessId());
		}

		FPerfCounters * PerfCounters = new FPerfCounters(InstanceUID);
		if (!PerfCounters->Initialize(JSonText))
		{
			UE_LOG(LogPerfCounters, Warning, TEXT("CreatePerformanceCounters: could not create perfcounters from (invalid?) configuration file: %s"), *JsonConfigFullPath);
			delete PerfCounters;
			return nullptr;
		}

		PerfCountersSingleton = PerfCounters;
		return PerfCounters;
	}
};

IMPLEMENT_MODULE(FPerfCountersModule, PerfCounters)
DEFINE_LOG_CATEGORY(LogPerfCounters);
