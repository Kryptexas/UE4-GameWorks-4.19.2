// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Editor/Kismet/Public/Profiler/TracePath.h"

//////////////////////////////////////////////////////////////////////////
// FBlueprintExecutionContext

class FBlueprintExecutionContext : public TSharedFromThis<FBlueprintExecutionContext>
{
public:

	/** Initialise the context from the blueprint path name */
	bool InitialiseContext(const FString& BlueprintPath);

	/** Returns if the blueprint context is fully formed */
	BLUEPRINTPROFILER_API bool IsContextValid() const { return Blueprint.IsValid() && BlueprintClass.IsValid(); }

	/** Returns the blueprint this context represents */
	TWeakObjectPtr<UBlueprint> GetBlueprint() const { return Blueprint; }

	/** Returns the blueprint class this context represents */
	TWeakObjectPtr<UBlueprintGeneratedClass> GetBlueprintClass() const { return BlueprintClass; }

	/** Returns the blueprint exec node for this context */
	TSharedPtr<class FScriptExecutionBlueprint> GetBlueprintExecNode() const { return BlueprintNode; }

	/** Add event execution node */
	void AddEventNode(TSharedPtr<FScriptExecutionNode> EventExecNode);

	/** Returns true if an execution node exists for the instance */
	BLUEPRINTPROFILER_API bool HasProfilerDataForInstance(const FName InstanceName) const;

	/** Corrects the instance name if it is a PIE instance and returns the editor actor */
	TWeakObjectPtr<const UObject> GetInstance(FName& InstanceNameInOut);

	/** Remaps PIE actor instance paths to editor actor instances */
	BLUEPRINTPROFILER_API FName RemapInstancePath(const FName InstanceName) const ;

	/** Returns the function name containing the event */
	FName GetEventFunctionName(const FName EventName) const;

	/** Returns the function context matching the supplied name providing it exists */
	TSharedPtr<class FBlueprintFunctionContext> GetFunctionContext(const FName FunctionNameIn) const;

	/** Adds a manually created function context */
	void AddFunctionContext(const FName FunctionName, TSharedPtr<class FBlueprintFunctionContext> FunctionContext)
	{
		FunctionContexts.Add(FunctionName) = FunctionContext; 
	}

	/** Returns if this context has an execution node for the specified graph node */
	BLUEPRINTPROFILER_API bool HasProfilerDataForNode(const UEdGraphNode* GraphNode) const;

	/** Returns existing or creates a new execution node for the specified graphnode */
	BLUEPRINTPROFILER_API TSharedPtr<FScriptExecutionNode> GetProfilerDataForNode(const UEdGraphNode* GraphNode);

	/** Updates cascading stats for the blueprint, this merges instance data upwards to the blueprint */
	void UpdateConnectedStats();

private:

	/** Creates and returns a new function context, returns existing if present  */
	TSharedPtr<FBlueprintFunctionContext> CreateFunctionContext(UEdGraph* Graph);

	/** Walks the blueprint and maps events and functions ready for profiling data */
	bool MapBlueprintExecution();

private:

	/** Blueprint the context represents */
	TWeakObjectPtr<UBlueprint> Blueprint;
	/** Blueprint Generated class for the context */
	TWeakObjectPtr<UBlueprintGeneratedClass> BlueprintClass;
	/** The ubergraph function name for the blueprint generated class */
	FName UbergraphFunctionName;
	/** Root level execution node for this blueprint */
	TSharedPtr<FScriptExecutionBlueprint> BlueprintNode;
	/** UFunction contexts for this blueprint */
	TMap<FName, TSharedPtr<FBlueprintFunctionContext>> FunctionContexts;
	/** PIE instance remap container */
	TMap<FName, FName> PIEInstanceNameMap;
	/** Editor instances */
	TMap<FName, TWeakObjectPtr<const UObject>> ActorInstances;

};

//////////////////////////////////////////////////////////////////////////
// FBlueprintFunctionContext

class FBlueprintFunctionContext : public TSharedFromThis<FBlueprintFunctionContext>
{
public:

	/** Initialise the function context from the graph */
	void InitialiseContextFromGraph(TSharedPtr<FBlueprintExecutionContext> BlueprintContext, UEdGraph* Graph);

	/** Map Function */
	void MapFunction();

	/** Returns if the function context is fully formed */
	bool IsContextValid() const { return Function.IsValid() && BlueprintClass.IsValid(); }

	/** Returns the graph node at the specified script offset */
	const UEdGraphNode* GetNodeFromCodeLocation(const int32 ScriptOffset);

	/** Returns the pin at the specified script offset */
	const UEdGraphPin* GetPinFromCodeLocation(const int32 ScriptOffset);

	/** Returns the script offset for the specified pin */
	const int32 GetCodeLocationFromPin(const UEdGraphPin* Pin) const;

	/** Returns all registered script offsets for the specified pin */
	void GetAllCodeLocationsFromPin(const UEdGraphPin* Pin, TArray<int32>& OutCodeLocations) const;

	/** Returns the pure node script code range for the specified node */
	FInt32Range GetPureNodeScriptCodeRange(const UEdGraphNode* Node) const;

	/** Returns true if this function context contains an execution node matching the node name */
	bool HasProfilerDataForNode(const FName NodeName) const;

	/** Returns the execution node representing the node name */
	TSharedPtr<FScriptExecutionNode> GetProfilerDataForNode(const FName NodeName);

	/** Adds all function entry points to node as children */
	void AddCallSiteEntryPointsToNode(TSharedPtr<FScriptExecutionNode> CallingNode) const;

	/** Returns the function name for this context */
	FName GetFunctionName() const { return FunctionName; }

private:

	friend FBlueprintExecutionContext;

	/** Utility to create execution node */
	TSharedPtr<FScriptExecutionNode> CreateExecutionNode(FScriptExecNodeParams& InitParams);

	/** Adds and links in a new function entry point */
	void AddEntryPoint(TSharedPtr<FScriptExecutionNode> EntryPoint);

	/** Adds a new function exit point */
	void AddExitPoint(TSharedPtr<FScriptExecutionNode> ExitPoint);

	/** Locates and generates event contexts for any input events passed in */
	void CreateInputEvents(TSharedPtr<FBlueprintExecutionContext> BlueprintContextIn, const TArray<UK2Node*>& InputEventNodes);

	/** Processes and detects any cyclic links making the linkage safe for traversal */
	bool DetectCyclicLinks(TSharedPtr<FScriptExecutionNode> ExecNode, TSet<TSharedPtr<FScriptExecutionNode>>& Filter);

	// --Execution mapping functionality

	/** Maps each blueprint node following execution wires */
	TSharedPtr<FScriptExecutionNode> MapNodeExecution(UEdGraphNode* NodeToMap);

	/** Maps input pin execution */
	void MapInputPins(TSharedPtr<FScriptExecutionNode> ExecNode, const TArray<UEdGraphPin*>& Pins);

	/** Maps pin execution */
	void MapExecPins(TSharedPtr<FScriptExecutionNode> ExecNode, const TArray<UEdGraphPin*>& Pins);

	/** Maps execution pin tunnel entry */
	TSharedPtr<FScriptExecutionNode> MapTunnelEntry(const FName TunnelName, const UEdGraphPin* ExecPin, const UEdGraphPin* TunnelPin, const int32 PinScriptOffset);

	// --Tunnel mapping functionality

	/** Maps the tunnel point into the instanced graph, creating the instanced graph if not already existing. */
	void MapTunnelInstance(UK2Node_Tunnel* TunnelInstance);

	/** Returns the tunnel function context that the tunnel node invokes */
	TSharedPtr<FBlueprintFunctionContext> GetTunnelContextFromNode(const UEdGraphNode* TunnelNode) const;

	/** Returns the registered tunnel execution entry point given the tunnel name */
	TSharedPtr<FScriptExecutionNode> GetTunnelEntryPoint(const FName TunnelName) const;

	/** Returns the registered tunnel exit points given the tunnel name */
	void GetTunnelExitPoints(const FName TunnelName, TArray<TSharedPtr<FScriptExecutionNode>>& ExitPointsOut) const;

	/** Maps input and output pins in a tunnel graph */
	void MapTunnelIO(UEdGraph* TunnelGraph);

	/** Returns the tunnel Id associated with the tunnel name */
	int32 GetTunnelIdFromName(const FName TunnelName) const;

private:

	/** The function name */
	FName FunctionName;
	/** The Owning Blueprint context */
	TWeakPtr<FBlueprintExecutionContext> BlueprintContext;
	/** The UFunction this context represents */
	TWeakObjectPtr<UFunction> Function;
	/** The blueprint generated class for the owning blueprint */
	TWeakObjectPtr<UBlueprintGeneratedClass> BlueprintClass;
	/** ScriptCode offset to node cache */
	TMap<int32, TWeakObjectPtr<const UEdGraphNode>> ScriptOffsetToNodes;
	/** ScriptCode offset to pin cache */
	TMap<int32, TWeakObjectPtr<const UEdGraphPin>> ScriptOffsetToPins;
	/** Execution nodes containing profiling data */
	TMap<FName, TSharedPtr<FScriptExecutionNode>> ExecutionNodes;
	/** Graph entry points */
	TArray<TSharedPtr<FScriptExecutionNode>> EntryPoints;
	/** Graph exit points */
	TArray<TSharedPtr<FScriptExecutionNode>> ExitPoints;
	/** Map associating function entry points to execution points */
	TMap<FName, TSharedPtr<FScriptExecutionNode>> TunnelEntryPointMap;
	/** Map associating function entry points to exit points */
	TMap<FName, TArray<TSharedPtr<FScriptExecutionNode>>> TunnelExitPointMap;
	/** Staging tunnel name during mapping */
	FName StagingTunnelName;
	/** Map of tunnel Ids */
	TMap<FName, int32> TunnelIds;
};

//////////////////////////////////////////////////////////////////////////
// FScriptEventPlayback

namespace EEventProcessingResult
{
	enum Type
	{
		None = 0,
		Failed,
		Success,
		Suspended
	};
};

class FScriptEventPlayback : public TSharedFromThis<FScriptEventPlayback>
{
public:

	FScriptEventPlayback(TSharedPtr<FBlueprintExecutionContext> BlueprintContextIn, const FName InstanceNameIn)
		: InstanceName(InstanceNameIn)
		, ProcessingState(EEventProcessingResult::None)
		, BlueprintContext(BlueprintContextIn)
	{
	}

	/** Returns the current processing state */
	EEventProcessingResult::Type GetProcessingState() const { return ProcessingState; }

	/** Returns if current processing state is suspended */
	bool IsSuspended() const { return ProcessingState == EEventProcessingResult::Suspended; }

	/** Processes the event and cleans up any errant signals */
	bool Process(const TArray<class FScriptInstrumentedEvent>& Events, const int32 Start, const int32 Stop);

	/** Sets the blueprint context for this batch */
	void SetBlueprintContext(TSharedPtr<FBlueprintExecutionContext> BlueprintContextIn) { BlueprintContext = BlueprintContextIn; }

private:

	/** Instance name */
	FName InstanceName;
	/** Event name */
	FName EventName;
	/** Processing state */
	EEventProcessingResult::Type ProcessingState;
	/** Current tracepath state */
	FTracePath TracePath;
	/** Current tracepath stack */
	TArray<FTracePath> TraceStack;
	/** Event Timings */
	TMultiMap<FName, double> EventTimings;
	/** Blueprint exec context */
	TSharedPtr<FBlueprintExecutionContext> BlueprintContext;

};


