// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleManager.h"

/**
 * A programming interface for setting/updating performance counters
 */
class IPerfCounters
{
public:

	virtual ~IPerfCounters() {};

	/** Get the unique identifier for this perf counter instance */
	virtual const FString& GetInstanceName() const = 0;

	/** Copies the value as is */
	virtual void SetPOD(const FString& Name, const void* Ptr, SIZE_T Size) = 0;

	/** Sets (POD) value */
	template< typename T >
	void Set(const FString& Name, const T & Val)
	{
		SetPOD(Name, &Val, sizeof(T));
	}
};


/**
 * The public interface to this module
 */
class IPerfCountersModule : public IModuleInterface
{
public:

	/**
	 * Singleton-like access to this module's interface.  This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static inline IPerfCountersModule& Get()
	{
		return FModuleManager::LoadModuleChecked< IPerfCountersModule >("PerfCounters");
	}

	/**
	 * Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("PerfCounters");
	}

	/**
	 * @return the currently initialized / in use perf counters 
	 */
	virtual IPerfCounters* GetPerformanceCounters() const = 0;

	/** 
	 * Creates and initializes the performance counters from a JSON config file.
	 *
	 * @param JsonConfigFilename	file that should reside in <GameName>/Config
	 * @param UniqueInstanceId		optional parameter that allows to assign a known name for this set of counters (a default one that will include process id will be provided if not given)
	 *
	 * @return IPerfCounters object (should be explicitly deleted later), or nullptr if failed
	 */
	virtual IPerfCounters * CreatePerformanceCounters(const FString& JsonConfigFilename, const FString& UniqueInstanceId = TEXT("")) = 0;
};
