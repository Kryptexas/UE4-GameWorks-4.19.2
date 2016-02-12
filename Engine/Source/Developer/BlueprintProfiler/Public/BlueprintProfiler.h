// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BlueprintProfilerModule.h"
#include "BlueprintInstrumentationSupport.h"
#include "TickableEditorObject.h"

class UEdGraphNode;

//////////////////////////////////////////////////////////////////////////
// FBlueprintProfiler

class FBlueprintProfiler : public IBlueprintProfilerInterface, public FTickableEditorObject
{
public:

	FBlueprintProfiler();
	virtual ~FBlueprintProfiler();

	// Begin IModuleInterface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	// End IModuleInterface

	// Begin IBlueprintProfilerModule
	virtual bool IsProfilerEnabled() const override { return bProfilerActive; }
	virtual bool IsProfilingCaptureActive() const { return bProfilingCaptureActive; }
	virtual void ToggleProfilingCapture() override;
	virtual void InstrumentEvent(const EScriptInstrumentationEvent& Event) override;

#if WITH_EDITOR
	virtual FOnBPStatGraphLayoutChanged& GetGraphLayoutChangedDelegate() { return GraphLayoutChangedDelegate; }
	virtual TSharedPtr<FScriptExecutionContext> GetCachedObjectContext(const FString& ObjectPath) override;
	virtual TSharedPtr<FScriptExecutionNode> GetProfilerDataForNode(const FName NodeName) override;
	virtual TSharedPtr<FScriptExecutionNode> GetOrCreateProfilerDataForNode(const FName NodeName) override;
	virtual bool HasDataForInstance(const UObject* Instance) override;
	virtual void ProcessEventProfilingData() override;
	virtual const FString GetValidInstancePath(const FString PathIn);
	virtual void NavigateToBlueprint(const FName NameIn) const override;
	virtual void NavigateToInstance(const FName NameIn) const override;
	virtual void NavigateToNode(const FName NameIn) const override;
#endif
	// End IBlueprintProfilerModule

	virtual void Tick(float DeltaSeconds) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT( FBlueprintProfiler, STATGROUP_Tickables ); }

private:

	/** Reset Profiling Data */
	virtual void ResetProfilingData();

	/** Registers delegates to recieve profiling events */
	void RegisterDelegates(bool bEnabled);

	/** Handles pie events to enable/disable profiling captures in the editor */
	void BeginPIE(bool bIsSimulating);
	void EndPIE(bool bIsSimulating);

	/** Utility functon to extract script code offset safely */
	int32 GetScriptCodeOffset(const EScriptInstrumentationEvent& Event) const;

	/** Utility functon to extract object path safely */
	FString GetCurrentFunctionPath(const EScriptInstrumentationEvent& Event) const;

#if WITH_EDITOR
	/** Removes all blueprint references from data */
	void RemoveAllBlueprintReferences(UBlueprint* Blueprint);

	/** Map Blueprint Execution flow */
	TSharedPtr<class FScriptExecutionContext> MapBlueprintExecutionFlow(const FString& BlueprintPath);

	/** Map Blueprint instance */
	void MapBlueprintInstance(const FString& InstancePath);

	/** Map Node execution flow */
	TSharedPtr<FScriptExecutionNode> MapNodeExecutionFlow(TSharedPtr<FScriptExecutionContext> BlueprintContext, UEdGraphNode* EventToMap);
#endif

protected:

	/** Multcast delegate to notify of statistic structural changes for events like compilation */
	FOnBPStatGraphLayoutChanged GraphLayoutChangedDelegate;

	/** Current instrumentation context */
	FKismetInstrumentContext ActiveContext;
	/** Raw instrumentation data */
	TArray<FBlueprintInstrumentedEvent> InstrumentationEvents;

	/** Profiler active state */
	bool bProfilerActive;
	/** Profiling capture state */
	bool bProfilingCaptureActive;

#if WITH_EDITOR
	/** PIE Active */
	bool bPIEActive;
	/** Instance path mappings between PIE and editor */
	TMap<FString, FString> PIEInstancePathMap;
	/** Processed profiling data */
	TMap<FName, TSharedPtr<FScriptExecutionNode>> NodeProfilingData;
	/** Cached object path and code offset lookup to UObjects */
	TMap<FString, TSharedPtr<FScriptExecutionContext>> PathToObjectContext;
#endif
	
};
