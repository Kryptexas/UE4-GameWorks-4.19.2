// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Editor/Kismet/Public/Profiler/TracePath.h"

//////////////////////////////////////////////////////////////////////////
// FBlueprintExecutionTrace

struct FBlueprintExecutionTrace
{
	EScriptInstrumentation::Type TraceType;
	FTracePath TracePath;
	FName InstanceName;
	FName FunctionName;
	FName NodeName;
	int32 Offset;
	double ObservationTime;
};

//////////////////////////////////////////////////////////////////////////
// FBlueprintExecutionContext

class FBlueprintExecutionContext : public TSharedFromThis<FBlueprintExecutionContext>
{
public:

	FBlueprintExecutionContext()
		: bIsBlueprintMapped(false)
		, ExecutionTraceHistory(256)
	{
	}

	/** Initialise the context from the blueprint path name */
	bool InitialiseContext(const FString& BlueprintPath);

	/** Returns if the blueprint context is fully formed */
	BLUEPRINTPROFILER_API bool IsContextValid() const { return Blueprint.IsValid() && BlueprintClass.IsValid(); }

	/** Returns if the blueprint is mapped */
	bool IsBlueprintMapped() const { return bIsBlueprintMapped; }

	/** Removes the blueprint mapping */
	void RemoveMapping();

	/** Returns the blueprint this context represents */
	TWeakObjectPtr<UBlueprint> GetBlueprint() const { return Blueprint; }

	/** Returns the blueprint class this context represents */
	TWeakObjectPtr<UBlueprintGeneratedClass> GetBlueprintClass() const { return BlueprintClass; }

	/** Returns the blueprint exec node for this context */
	TSharedPtr<class FScriptExecutionBlueprint> GetBlueprintExecNode() const { return BlueprintNode; }

	/** Returns the blueprint trace history */
	const TSimpleRingBuffer<FBlueprintExecutionTrace>& GetTraceHistory() const { return ExecutionTraceHistory; }

	/** Returns a new trace history event */
	FBlueprintExecutionTrace& AddNewTraceHistoryEvent() { return ExecutionTraceHistory.WriteNewElementInitialized(); }

	/** Returns if an event is mapped. */
	bool IsEventMapped(const FName EventName) const;

	/** Add event execution node */
	void AddEventNode(TSharedPtr<class FBlueprintFunctionContext> FunctionContext, TSharedPtr<FScriptExecutionNode> EventExecNode);

	/** Register a function context for an event */
	void RegisterEventContext(const FName EventName, TSharedPtr<FBlueprintFunctionContext> FunctionContext);

	/** Returns true if an execution node exists for the instance */
	BLUEPRINTPROFILER_API bool HasProfilerDataForInstance(const FName InstanceName) const;

	/** Maps the instance returning the system name for instance */
	BLUEPRINTPROFILER_API FName MapBlueprintInstance(const FString& InstancePath);

	/** Returns the instance if mapped */
	BLUEPRINTPROFILER_API TSharedPtr<class FScriptExecutionInstance> GetInstanceExecNode(const FName InstanceName);

	/** Remaps PIE actor instance paths to editor actor instances */
	BLUEPRINTPROFILER_API FName RemapInstancePath(const FName InstanceName) const;

	/** Returns the function name containing the event */
	FName GetEventFunctionName(const FName EventName) const;

	/** Returns the function context containing the event */
	TSharedPtr<class FBlueprintFunctionContext> GetFunctionContextForEvent(const FName EventName) const;

	/** Returns the function context matching the supplied name providing it exists */
	TSharedPtr<class FBlueprintFunctionContext> GetFunctionContext(const FName FunctionNameIn) const;

	/** Adds a manually created function context */
	void AddFunctionContext(const FName FunctionName, TSharedPtr<class FBlueprintFunctionContext> FunctionContext)
	{
		FunctionContexts.Add(FunctionName) = FunctionContext; 
	}

	/** Returns if this context has an execution node for the specified graph pin */
	BLUEPRINTPROFILER_API bool HasProfilerDataForPin(const UEdGraphPin* GraphPin) const;

	/** Returns if this context has an execution node for the specified graph pin, otherwise it falls back to a owning node lookup */
	BLUEPRINTPROFILER_API TSharedPtr<FScriptExecutionNode> GetProfilerDataForPin(const UEdGraphPin* GraphPin);

	/** Returns if this context has an execution node for the specified graph node */
	BLUEPRINTPROFILER_API bool HasProfilerDataForNode(const UEdGraphNode* GraphNode) const;

	/** Returns existing or creates a new execution node for the specified graphnode */
	BLUEPRINTPROFILER_API TSharedPtr<FScriptExecutionNode> GetProfilerDataForNode(const UEdGraphNode* GraphNode);

	/** Updates cascading stats for the blueprint, this merges instance data upwards to the blueprint */
	void UpdateConnectedStats();

private:

	/** Creates and returns a new function context, returns existing if present  */
	template<typename FunctionType> TSharedPtr<FunctionType> CreateFunctionContext(const FName FunctionName, UEdGraph* Graph);

	/** Walks the blueprint and maps events and functions ready for profiling data */
	bool MapBlueprintExecution();

	/** Corrects the instance name if it is a PIE instance and sets the object weakptr to the editor actor, returns true if this is a new instance */
	bool ResolveInstance(FName& InstanceNameInOut, TWeakObjectPtr<const UObject>& ObjectInOut);

private:

	/** Indicates the blueprint has been mapped */
	bool bIsBlueprintMapped;
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
	/** UFunction context lookup by event name */
	TMap<FName, TSharedPtr<FBlueprintFunctionContext>> EventFunctionContexts;
	/** PIE instance remap container */
	TMap<FName, FName> PIEInstanceNameMap;
	/** Editor instances */
	TMap<FName, TWeakObjectPtr<const UObject>> EditorActorInstances;
	/** PIE instances */
	TMap<FName, TWeakObjectPtr<const UObject>> PIEActorInstances;
	/** Event Trace History */
	TSimpleRingBuffer<FBlueprintExecutionTrace> ExecutionTraceHistory;
};

//////////////////////////////////////////////////////////////////////////
// FBlueprintFunctionContext

class FBlueprintFunctionContext : public TSharedFromThis<FBlueprintFunctionContext>
{
public:

	virtual ~FBlueprintFunctionContext() {}

	/** Initialise the function context from the graph */
	virtual void InitialiseContextFromGraph(TSharedPtr<FBlueprintExecutionContext> BlueprintContext, const FName FunctionNameIn, UEdGraph* Graph);

	/** Discover and add any tunnels */
	void DiscoverTunnels(UEdGraph* Graph, TMap<UK2Node_Tunnel*, TSharedPtr<FBlueprintFunctionContext>>& DiscoveredTunnels);

	/** Map Function */
	virtual void MapFunction();

	/** Returns if the function context is fully formed */
	bool IsContextValid() const { return Function.IsValid() && BlueprintClass.IsValid(); }

	/** Returns the graph node at the specified script offset */
	const UEdGraphNode* GetNodeFromCodeLocation(const int32 ScriptOffset);

	/** Returns the tunnel source node at the specified script offset */
	const UEdGraphNode* FindTunnelSourceNodeFromCodeLocation(const int32 ScriptOffset);

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

	/** Returns the execution node representing the node name, asserting on fail */
	TSharedPtr<FScriptExecutionNode> GetProfilerDataForNodeChecked(const FName NodeName);

	/** Returns the execution node representing the graph node */
	TSharedPtr<FScriptExecutionNode> GetProfilerDataForGraphNode(const UEdGraphNode* Node);

	/** Returns the execution node representing the node name */
	template<typename ExecNodeType> TSharedPtr<ExecNodeType> GetTypedProfilerDataForNode(const FName NodeName);

	/** Returns the execution node representing the object */
	template<typename ExecNodeType> TSharedPtr<ExecNodeType> GetTypedProfilerDataForGraphNode(const UEdGraphNode* Node);

	/** Adds all function entry points to node as children */
	void AddCallSiteEntryPointsToNode(TSharedPtr<FScriptExecutionNode> CallingNode) const;

	/** Returns the function name for this context */
	FName GetFunctionName() const { return FunctionName; }

	/** Returns the UFunction for this context */
	TWeakObjectPtr<UFunction> GetUFunction() { return Function; }

	/** Returns the BlueprintClass for this context */
	TWeakObjectPtr<UBlueprintGeneratedClass> GetBlueprintClass() { return BlueprintClass; }

	/** Returns a valid pin name, creating one based on pin characteristics if not available */
	static FName GetPinName(const UEdGraphPin* Pin);

	/** Returns a fully qualified pin name including owning node id */
	static FName GetUniquePinName(const UEdGraphPin* Pin);

	/** Returns a tunnel instance Boundary name */
	static FName GetTunnelBoundaryName(const UEdGraphPin* Pin);

	/** Looks for matching pin in the supplied node, handles pins with no name. */
	static UEdGraphPin* FindMatchingPin(const UEdGraphNode* NodeToSearch, const UEdGraphPin* PinToFind, const bool bIgnoreDirection = false);

	/** Utility function that returns the tunnel graph from the supplied node */
	static UEdGraph* GetTunnelGraphFromNode(const UEdGraphNode* TunnelNode);

	/** Utility function that returns if the tunnel node is internal to a tunnel graph */
	static bool IsTunnelInternal(const UEdGraphNode* TunnelNode);

protected:

	friend FBlueprintExecutionContext;
	friend class FBlueprintTunnelInstanceContext;

	/** Utility to create execution node */
	virtual TSharedPtr<FScriptExecutionNode> CreateExecutionNode(FScriptExecNodeParams& InitParams);

	/** Utility to create typed execution node */
	template<typename ExecNodeType> TSharedPtr<ExecNodeType> CreateTypedExecutionNode(FScriptExecNodeParams& InitParams);

	/** Adds and links in a new function entry point */
	void AddEntryPoint(TSharedPtr<FScriptExecutionNode> EntryPoint);

	/** Adds a new function exit point */
	void AddExitPoint(TSharedPtr<FScriptExecutionNode> ExitPoint);

	/** Creates an event for delegate pin entry points */
	void CreateDelegatePinEvents(TSharedPtr<FBlueprintExecutionContext> BlueprintContextIn, const TMap<FName, FEdGraphPinReference>& PinEvents);

	/** Processes and detects any cyclic links making the linkage safe for traversal */
	bool DetectCyclicLinks(TSharedPtr<FScriptExecutionNode> ExecNode, TSet<TSharedPtr<FScriptExecutionNode>>& Filter);

	// --Execution mapping functionality

	/** Maps each blueprint node following execution wires */
	virtual TSharedPtr<FScriptExecutionNode> MapNodeExecution(UEdGraphNode* NodeToMap);

	/** Maps input pin execution */
	void MapInputPins(TSharedPtr<FScriptExecutionNode> ExecNode, const TArray<UEdGraphPin*>& Pins);

	/** Maps pin execution */
	void MapExecPins(TSharedPtr<FScriptExecutionNode> ExecNode, const TArray<UEdGraphPin*>& Pins);

	// --Tunnel mapping functionality

	/** Maps tunnel boundries */
	virtual TSharedPtr<FScriptExecutionNode> MapTunnelBoundary(const UEdGraphPin* TunnelPin);

	/** Maps the tunnel point into the instanced graph, creating the instanced graph if not already existing. */
	void MapTunnelInstance(UK2Node_Tunnel* TunnelInstance);

	/** Maps execution pin tunnel entry */
	void MapTunnelExits(TSharedPtr<FScriptExecutionTunnelEntry> TunnelEntryPoint);

protected:

	/** The graph name */
	FName GraphName;
	/** The function name */
	FName FunctionName;
	/** The Owning Blueprint context */
	TWeakPtr<FBlueprintExecutionContext> BlueprintContext;
	/** The UFunction this context represents */
	TWeakObjectPtr<UFunction> Function;
	/** The blueprint that owns this function graph */
	TWeakObjectPtr<UBlueprint> OwningBlueprint;
	/** The blueprint generated class for the owning blueprint */
	TWeakObjectPtr<UBlueprintGeneratedClass> BlueprintClass;
	/** ScriptCode offset to node cache */
	TMap<int32, TWeakObjectPtr<const UEdGraphNode>> ScriptOffsetToNodes;
	/** ScriptCode offset to pin cache */
	TMap<int32, FEdGraphPinReference> ScriptOffsetToPins;
	/** Execution nodes containing profiling data */
	TMap<FName, TSharedPtr<FScriptExecutionNode>> ExecutionNodes;
	/** Other function contexts in use by this context */
	TMap<FName, TWeakPtr<FBlueprintFunctionContext>> ChildFunctionContexts;
	/** Graph entry points */
	TArray<TSharedPtr<FScriptExecutionNode>> EntryPoints;
	/** Graph exit points */
	TArray<TSharedPtr<FScriptExecutionNode>> ExitPoints;
};

//////////////////////////////////////////////////////////////////////////
// FBlueprintTunnelInstanceContext

class FBlueprintTunnelInstanceContext : public FBlueprintFunctionContext
{
public:

	// ~FBlueprintFunctionContext Begin
	virtual void InitialiseContextFromGraph(TSharedPtr<FBlueprintExecutionContext> BlueprintContext, const FName TunnelInstanceName, UEdGraph* TunnelGraph);
	virtual void MapFunction() override {}
	virtual TSharedPtr<FScriptExecutionNode> CreateExecutionNode(FScriptExecNodeParams& InitParams) override;
	// ~FBlueprintFunctionContext End

	/** Maps a tunnel function context. */
	void MapTunnelContext(TSharedPtr<FBlueprintFunctionContext> CallingFunctionContext, UK2Node_Tunnel* TunnelInstance);

private:

	friend FBlueprintExecutionContext;

	// ~FBlueprintFunctionContext Begin
	virtual TSharedPtr<FScriptExecutionNode> MapNodeExecution(UEdGraphNode* NodeToMap) override;
	virtual TSharedPtr<FScriptExecutionNode> MapTunnelBoundary(const UEdGraphPin* TunnelPin) override;
	// ~FBlueprintFunctionContext End
	
	/** Maps input and output pins in a tunnel graph */
	void MapTunnelIO();

	/** Returns true if the pin is part of this tunnel */
	bool IsPinFromThisTunnel(const UEdGraphPin* TunnelPin) const;

	/** Discovers already mapped exit sites */
	void DiscoverExitSites(TSharedPtr<FScriptExecutionNode> MappedNode);

private:

	/** The tunnel instance this context represents */
	TWeakObjectPtr<UK2Node_Tunnel> TunnelInstanceNode;
	/** The non-instance context of the tunnel graph */
	TSharedPtr<FBlueprintFunctionContext> TunnelFunctionContext;
	/** External tunnel nodes */
	TMap<FName, TSharedPtr<FScriptExecutionNode>> ExternalNodes;
	/** Staging entry point */
	TSharedPtr<FScriptExecutionTunnelEntry> StagingEntryPoint;

};

//////////////////////////////////////////////////////////////////////////
// FScriptEventPlayback

class FScriptEventPlayback : public TSharedFromThis<FScriptEventPlayback>
{
public:

	struct NodeSignalHelper
	{
		TSharedPtr<FScriptExecutionNode> ImpureNode;
		TSharedPtr<FBlueprintExecutionContext> BlueprintContext;
		TSharedPtr<FBlueprintFunctionContext> FunctionContext;
		TArray<class FScriptInstrumentedEvent> ExclusiveEvents;
		TArray<class FScriptInstrumentedEvent> InclusiveEvents;
		TArray<FTracePath> ExclusiveTracePaths;
		TArray<FTracePath> InputTracePaths;
		TArray<FTracePath> InclusiveTracePaths;
	};

	struct TunnelEventHelper
	{
		double TunnelEntryTime;
		FTracePath EntryTracePath;
		TSharedPtr<FScriptExecutionTunnelEntry> TunnelEntryPoint;
	};

	struct PushState
	{
		PushState(const UEdGraphNode* NodeIn, const FTracePath& TracePathIn)
			: InstigatingNode(NodeIn)
			, TracePath(TracePathIn)
		{
		}

		const UEdGraphNode* InstigatingNode;
		FTracePath TracePath;
	};

	FScriptEventPlayback(TSharedPtr<FBlueprintExecutionContext> BlueprintContextIn, const FName InstanceNameIn)
		: InstanceName(InstanceNameIn)
		, CurrentFunctionName(NAME_None)
		, LatentLinkId(INDEX_NONE)
		, BlueprintContext(BlueprintContextIn)
	{
	}

	/** Returns if current processing state is suspended */
	bool IsSuspended() const { return LatentLinkId != INDEX_NONE; }

	/** Returns the current latent action UUID */
	const int32 GetLatentLinkId() const { return LatentLinkId; }

	/** Processes the event and cleans up any errant signals */
	bool Process(const TArray<class FScriptInstrumentedEvent>& Events, const int32 Start, const int32 Stop);

	/** Sets the blueprint context for this batch */
	void SetBlueprintContext(TSharedPtr<FBlueprintExecutionContext> BlueprintContextIn) { BlueprintContext = BlueprintContextIn; }

private:

	/** Process tunel boundries */
	void ProcessTunnelBoundary(NodeSignalHelper& CurrentNodeData, const FScriptInstrumentedEvent& CurrSignal);

	/** Process execution sequence */
	void ProcessExecutionSequence(NodeSignalHelper& CurrentNodeData, const FScriptInstrumentedEvent& CurrSignal);

	/** Add to trace history */
	void AddToTraceHistory(const FName NodeName, const FScriptInstrumentedEvent& TraceSignal);

private:

	/** Instance name */
	FName InstanceName;
	/** Function name */
	FName CurrentFunctionName;
	/** Event name */
	FName EventName;
	/** Latent Link Id for suspended Event */
	int32 LatentLinkId;
	/** Current tracepath state */
	FTracePath TracePath;
	/** Current tracepath stack */
	TArray<FTracePath> TraceStack;
	/** Current tunnel tracepath stack */
	TArray<FTracePath> TunnelTraceStack;
	/** Active tunnels */
	TMap<FName, TunnelEventHelper> ActiveTunnels;
	/** Event Timings */
	TMultiMap<FName, double> EventTimings;
	/** Blueprint exec context */
	TSharedPtr<FBlueprintExecutionContext> BlueprintContext;

};


