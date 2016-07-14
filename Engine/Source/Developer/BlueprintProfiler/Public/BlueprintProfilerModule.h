// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleManager.h"

#if WITH_EDITOR

/** Profiler heat map display mode */
namespace EBlueprintProfilerHeatMapDisplayMode
{
	enum Type
	{
		None = 0,
		Inclusive,
		Exclusive,
		MaxTiming,
		Total,
		HottestPath,
		HottestEndpoint,
		PinToPin
	};
}

#endif

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

	/** Returns the cached blueprint context or creates and maps a new entry */
	virtual TSharedPtr<class FBlueprintExecutionContext> GetBlueprintContext(const FString& BlueprintClassPath) = 0;

	/** Returns the current profiling event data for node */
	virtual TSharedPtr<class FScriptExecutionNode> GetProfilerDataForNode(const UEdGraphNode* GraphNode) = 0;

	/** Returns if the profiler currently retains any data for the specified instance */
	virtual bool HasDataForInstance(const UObject* Instance) const { return false; }

	/** Process profiling event data */
	virtual void ProcessEventProfilingData() {}

	/** Returns the current graph node heat map display mode setting */
	virtual EBlueprintProfilerHeatMapDisplayMode::Type GetGraphNodeHeatMapDisplayMode() const = 0;

	/** Sets the current graph node heat map display mode setting */
	virtual void SetGraphNodeHeatMapDisplayMode(EBlueprintProfilerHeatMapDisplayMode::Type InHeatMapDisplayMode) {}

	/** Returns the current wire heat map display mode setting */
	virtual EBlueprintProfilerHeatMapDisplayMode::Type GetWireHeatMapDisplayMode() const = 0;

	/** Sets the current wire heat map display mode setting */
	virtual void SetWireHeatMapDisplayMode(EBlueprintProfilerHeatMapDisplayMode::Type InHeatMapDisplayMode) {}

#endif // WITH_EDITOR

};