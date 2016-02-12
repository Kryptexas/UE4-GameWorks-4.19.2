// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleManager.h"

class UBlueprint;
struct EScriptInstrumentationEvent;

/** Delegate to broadcast structural stats changes */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnBPStatGraphLayoutChanged, TWeakObjectPtr<UBlueprint>);

//////////////////////////////////////////////////////////////////////////
// IBlueprintProfilerInterface

class IBlueprintProfilerInterface : public IModuleInterface
{
public:

	/** Returns true if the profiler is enabled */
	virtual bool IsProfilerEnabled() const { return false; }

	/** Returns true if the profiler is enabled and actively capturing events */
	virtual bool IsProfilingCaptureActive() const { return false; }

	/** Toggles the profiling event capture state */
	virtual void ToggleProfilingCapture() {}

	/** Instruments and add a profiling event to the data */
	virtual void InstrumentEvent(const EScriptInstrumentationEvent& Event) {}

#if WITH_EDITOR

	/** Returns the multicast delegate to signal blueprint layout changes to stats */
	virtual FOnBPStatGraphLayoutChanged& GetGraphLayoutChangedDelegate() = 0;

	/** Returns the cached object context or creates a new entry */
	virtual TSharedPtr<class FScriptExecutionContext> GetCachedObjectContext(const FString& ObjectPath) = 0;

	/** Returns the current profiling event data for node */
	virtual TSharedPtr<class FScriptExecutionNode> GetProfilerDataForNode(const FName NodeName) = 0;

	/** Returns or creates an entry for the node */
	virtual TSharedPtr<class FScriptExecutionNode> GetOrCreateProfilerDataForNode(const FName NodeName) = 0;

	/** Returns if the profiler currently retains any data for the specified instance */
	virtual bool HasDataForInstance(const UObject* Instance) { return false; }

	/** Get Valid Instance Path */
	virtual const FString GetValidInstancePath(const FString PathIn) = 0;

	/** Process profiling event data */
	virtual void ProcessEventProfilingData() {}

	/** Navigate to blueprint */
	virtual void NavigateToBlueprint(const FName NameIn) const = 0;

	/** Navigate to instance */
	virtual void NavigateToInstance(const FName NameIn) const = 0;

	/** Navigate to profiler node */
	virtual void NavigateToNode(const FName NameIn) const = 0;

#endif // WITH_EDITOR

};