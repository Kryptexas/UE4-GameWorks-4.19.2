// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "Private/BlueprintProfilerPCH.h"
#include "EditorStyleSet.h"

#define LOCTEXT_NAMESPACE "BlueprintProfiler"

//////////////////////////////////////////////////////////////////////////
// FBlueprintProfiler

IMPLEMENT_MODULE(FBlueprintProfiler, BlueprintProfiler);

DECLARE_STATS_GROUP(TEXT("BlueprintProfiler"), STATGROUP_BlueprintProfiler, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("Tick"), STAT_ProfilerTickCost, STATGROUP_BlueprintProfiler);
DECLARE_CYCLE_STAT(TEXT("Processing Events"), STAT_ProfilerInstrumentationCost, STATGROUP_BlueprintProfiler);
DECLARE_CYCLE_STAT(TEXT("Statistic Update"), STAT_StatUpdateCost, STATGROUP_BlueprintProfiler);
DECLARE_CYCLE_STAT(TEXT("Node Lookup"), STAT_NodeLookupCost, STATGROUP_BlueprintProfiler);

FBlueprintProfiler::FBlueprintProfiler()
	: bProfilerActive(false)
	, bProfilingCaptureActive(false)
	, bPIEActive(false)
{
}

FBlueprintProfiler::~FBlueprintProfiler()
{
	if (bProfilingCaptureActive)
	{
		ToggleProfilingCapture();
	}
}

void FBlueprintProfiler::StartupModule()
{
	FBlueprintCoreDelegates::OnToggleScriptProfiler.AddRaw(this, &FBlueprintProfiler::RegisterDelegates);
}

void FBlueprintProfiler::ShutdownModule()
{
	FBlueprintCoreDelegates::OnToggleScriptProfiler.RemoveAll(this);
}

void FBlueprintProfiler::ToggleProfilingCapture()
{
	// Toggle profiler state
	bProfilerActive = !bProfilerActive;
#if WITH_EDITOR
	bProfilingCaptureActive = bProfilerActive && bPIEActive;
#else
	bProfilingCaptureActive = bProfilerActive;
#endif // WITH_EDITOR
	// Broadcast capture state change so delegate consumers can update their state.
	FBlueprintCoreDelegates::OnToggleScriptProfiler.Broadcast(bProfilerActive);
}

void FBlueprintProfiler::InstrumentEvent(const EScriptInstrumentationEvent& Event)
{
	#if WITH_EDITOR
	SCOPE_CYCLE_COUNTER(STAT_ProfilerInstrumentationCost);
	#endif
	// Handle context switching events
	ActiveContext.UpdateContext(Event.GetContextObject(), InstrumentationEvents);
	// Add instrumented event
	const int32 ScriptCodeOffset = GetScriptCodeOffset(Event);
	const FString CurrentFunctionPath = GetCurrentFunctionPath(Event);
	InstrumentationEvents.Add(FBlueprintInstrumentedEvent(Event.GetType(), CurrentFunctionPath, ScriptCodeOffset));
	// Reset context on event end
	if (Event.GetType() == EScriptInstrumentation::Stop)
	{
		ActiveContext.ResetContext();
	}
}

int32 FBlueprintProfiler::GetScriptCodeOffset(const EScriptInstrumentationEvent& Event) const
{
	int32 ScriptCodeOffset = -1;
	if (Event.IsStackFrameValid())
	{
		const FFrame& StackFrame = Event.GetStackFrame();
		ScriptCodeOffset = StackFrame.Code - StackFrame.Node->Script.GetData() - 1;
	}
	return ScriptCodeOffset;
}

FString FBlueprintProfiler::GetCurrentFunctionPath(const EScriptInstrumentationEvent& Event) const
{
	FString CurrentFunctionPath;
	if (Event.IsStackFrameValid())
	{
		const FFrame& StackFrame = Event.GetStackFrame();
		CurrentFunctionPath = StackFrame.Node->GetPathName();
	}
	else
	{
		const FString ClassName = Event.GetContextObject()->GetClass()->GetPathName();
		CurrentFunctionPath = FString::Printf( TEXT( "%s:%s" ), *ClassName, *Event.GetEventName().ToString());
	}
	return CurrentFunctionPath;
}

#if WITH_EDITOR

TSharedPtr<FScriptExecutionContext> FBlueprintProfiler::GetCachedObjectContext(const FString& ObjectPath)
{
	SCOPE_CYCLE_COUNTER(STAT_NodeLookupCost);
	TSharedPtr<FScriptExecutionContext>& Result = PathToObjectContext.FindOrAdd(ObjectPath);
	if (!Result.IsValid())
	{
		Result = MakeShareable<FScriptExecutionContext>(new FScriptExecutionContext);
	}
	return Result;
}

TSharedPtr<FScriptExecutionNode> FBlueprintProfiler::GetProfilerDataForNode(const FName NodeName)
{
	SCOPE_CYCLE_COUNTER(STAT_NodeLookupCost);
	TSharedPtr<FScriptExecutionNode> Result;
	if (TSharedPtr<FScriptExecutionNode>* Entry = NodeProfilingData.Find(NodeName))
	{
		Result = *Entry;
	}
	return Result;
}

TSharedPtr<FScriptExecutionNode> FBlueprintProfiler::GetOrCreateProfilerDataForNode(const FName NodeName)
{
	TSharedPtr<FScriptExecutionNode>& Result = NodeProfilingData.FindOrAdd(NodeName);
	if (!Result.IsValid())
	{
		Result = MakeShareable(new FScriptExecutionNode);
	}
	return Result;
}

bool FBlueprintProfiler::HasDataForInstance(const UObject* Instance)
{
	return false;
}

void FBlueprintProfiler::Tick(float DeltaSeconds)
{
	SCOPE_CYCLE_COUNTER(STAT_ProfilerTickCost);
	ProcessEventProfilingData();
}

bool FBlueprintProfiler::IsTickable() const
{
	return InstrumentationEvents.Num() > 0;
}

void FBlueprintProfiler::ProcessEventProfilingData()
{
	// Really not keen on this function its all very ugly, it mostly works but I'll try and refactor as I get chance.

	// Structure to track trace path stacks
	struct LocalStruct
	{
		const UObject* Object;
		EScriptInstrumentation::Type Type;
		int32 ScriptCodeOffset;
		double EventTime;
		FTracePath TracePath;

		bool operator == (const LocalStruct& Other) const
		{
			return Object == Other.Object; 
		}

	};
	// Processing context info
	int32 ExecStart = 0;
	int32 ProcessedIndex = 0;
	TArray<LocalStruct> TraceStack;
	LocalStruct NewEntry;
	FName InstanceName = NAME_None;
	TSharedPtr<FScriptExecutionContext> ExecContext;
	FTracePath TracePath;
	// Process incoming event data. This needs pulling out into events and processing.
	for (int32 EventIdx = 0; EventIdx < InstrumentationEvents.Num(); ++EventIdx)
	{
		const FBlueprintInstrumentedEvent& CurrEvent = InstrumentationEvents[EventIdx];
		NewEntry.Type = CurrEvent.GetType();
		NewEntry.EventTime = CurrEvent.GetTime();
		NewEntry.ScriptCodeOffset = CurrEvent.GetScriptCodeOffset();

		switch(CurrEvent.GetType())
		{
			case EScriptInstrumentation::Class:
			{
				ExecContext = MapBlueprintExecutionFlow(CurrEvent.GetObjectPath());
				ExecStart = EventIdx;
				break;
			}
			case EScriptInstrumentation::Instance:
			{
				MapBlueprintInstance(CurrEvent.GetObjectPath());
				ExecStart = EventIdx;
				InstanceName = FName(*GetValidInstancePath(CurrEvent.GetObjectPath()));
				TracePath.Reset();
				break;
			}
			case EScriptInstrumentation::NodeEntry:
			case EScriptInstrumentation::PureNodeEntry:
			{
				UFunction* NodeFunction = ExecContext->BlueprintClass->FindFunctionByName(CurrEvent.GetFunctionName());
				NewEntry.Object = ExecContext.IsValid() ? ExecContext->GetNodeFromCodeLocation(CurrEvent.GetScriptCodeOffset(), NodeFunction) : nullptr;
				NewEntry.TracePath = TracePath;
				// Add new event.
				TraceStack.Add(NewEntry);
				break;
			}
			case EScriptInstrumentation::NodeExit:
			{
				if (TraceStack.Num() > 0)
				{
					const int32 ScriptCodeOffset = CurrEvent.GetScriptCodeOffset();
					UFunction* NodeFunction = ExecContext->BlueprintClass->FindFunctionByName(CurrEvent.GetFunctionName());
					const UEdGraphNode* GraphNode = ExecContext.IsValid() ? ExecContext->GetNodeFromCodeLocation(ScriptCodeOffset, NodeFunction) : nullptr;
					// Jumps in scripts can cause multiple node exits on branches, check here and ignore the first exit if two are present.
					if (EventIdx + 1 < InstrumentationEvents.Num() && InstrumentationEvents[EventIdx+1].GetType() == EScriptInstrumentation::NodeExit)
					{
						UFunction* NextNodeFunction = ExecContext->BlueprintClass->FindFunctionByName(InstrumentationEvents[EventIdx+1].GetFunctionName());
						const UEdGraphNode* NextGraphNode = ExecContext.IsValid() ? ExecContext->GetNodeFromCodeLocation(InstrumentationEvents[EventIdx+1].GetScriptCodeOffset(), NextNodeFunction) : nullptr;
						if (NextGraphNode && NextGraphNode == GraphNode)
						{
							// Skip this branch jump.
							continue;
						}
					}
					// Group up timings and submit.
					if (TraceStack.Last().Object == GraphNode && TraceStack.Last().Type == EScriptInstrumentation::NodeEntry)
					{
						// Find node time and pool pure node timings.
						const double NodeTime = NewEntry.EventTime - TraceStack.Last().EventTime;
						double PureNodeTime = 0.0;
						const int32 NodeIndex = TraceStack.Num() - 1;
						int32 PureNodeIndex = NodeIndex - 1;
						for (; PureNodeIndex >= 0 && TraceStack[PureNodeIndex] == TraceStack[NodeIndex]; --PureNodeIndex)
						{
							PureNodeTime = TraceStack[NodeIndex].EventTime - TraceStack[PureNodeIndex].EventTime;
						}
						// Find exec node from graph node
						TSharedPtr<FScriptExecutionNode> ExecNode = GetProfilerDataForNode(FName(*GraphNode->GetPathName()));

						if (ExecNode.IsValid())
						{
							if (ExecNode->GetNumChildren() > 0)
							{
								// Restore TraceStack when exiting macro/function
								TracePath = TraceStack.Last().TracePath;
							}
							// Update performance data on the tracepath.
							TSharedPtr<FScriptPerfData> PerfData = ExecNode->GetPerfDataByInstanceAndTracePath(InstanceName, TracePath);
							PerfData->AddEventTiming(PureNodeTime, NodeTime);
						}
						// Flush the trace stack of the current events.
						TraceStack.RemoveAt(PureNodeIndex+1, NodeIndex - PureNodeIndex, false);
						// Apply new node timing
						ProcessedIndex = EventIdx;
					}
					// Update exit pin
					TracePath.AddExitPin(ScriptCodeOffset);
				}
				break;
			}
			case EScriptInstrumentation::Event:
			{
				TracePath.Reset();
				// Start event timing
				NewEntry.Object = ExecContext->BlueprintClass->FindFunctionByName(CurrEvent.GetFunctionName());
				NewEntry.TracePath.Reset();
				// Add new event.
				TraceStack.Add(NewEntry);
				break;
			}
			case EScriptInstrumentation::Stop:
			{
				// Add event timing
				if (TraceStack.Num() > 0 && TraceStack.Last().Type == EScriptInstrumentation::Event)
				{
					// Find node time and pool pure node timings.
					const double NodeTime = NewEntry.EventTime - TraceStack.Last().EventTime;
					// Find event exec node
					FName EventPath(*FString::Printf( TEXT( "%s:%s" ), *ExecContext->ContextObject.Get()->GetName(), *TraceStack.Last().Object->GetName()));
					TSharedPtr<FScriptExecutionNode> ExecNode = GetProfilerDataForNode(EventPath);
					TracePath.Reset();

					if (ExecNode.IsValid())
					{
						// Update performance data on the tracepath.
						TSharedPtr<FScriptPerfData> PerfData = ExecNode->GetPerfDataByInstanceAndTracePath(InstanceName, TracePath);
						PerfData->AddEventTiming(0.0, NodeTime);
					}
					// Flush the trace stack of the current events.
					TraceStack.SetNum(0);
				}
				// Update stats
				if (ExecContext.IsValid())
				{
					SCOPE_CYCLE_COUNTER(STAT_StatUpdateCost);
					ExecContext->UpdateConnectedStats();
				}
				// Update processed index.
				ProcessedIndex = EventIdx;
				break;
			}
		}
	}
	// Reset profiler data
	if (ProcessedIndex > 0)
	{
		InstrumentationEvents.RemoveAt(0, ProcessedIndex+1);
	}
}

void FBlueprintProfiler::RemoveAllBlueprintReferences(UBlueprint* Blueprint)
{
	// Remove all blueprint references
	if (Blueprint)
	{
		const FString BlueprintPath = Blueprint->GeneratedClass->GetPathName();
		// Remove Object Context
		if (TSharedPtr<FScriptExecutionContext>* ContextResult = PathToObjectContext.Find(BlueprintPath))
		{
			TSharedPtr<FScriptExecutionContext> ContextToRemove = *ContextResult;
			// Clean up instances
			TSharedPtr<FScriptExecutionBlueprint> BlueprintRoot = StaticCastSharedPtr<FScriptExecutionBlueprint>(ContextToRemove->ExecutionNode);
			if (BlueprintRoot.IsValid())
			{
				// Remove PIE remaps
				for (int32 InstanceIdx = 0; InstanceIdx < BlueprintRoot->GetInstanceCount(); ++InstanceIdx)
				{
					const FString InstancePath(BlueprintRoot->GetInstanceByIndex(InstanceIdx)->GetName().ToString());
					PIEInstancePathMap.Remove(InstancePath);
					if (const FString* Key = PIEInstancePathMap.FindKey(InstancePath))
					{
						PIEInstancePathMap.Remove(*Key);
					}
				}
			}
			// Clean up nodes
			TMap<FName, TSharedPtr<FScriptExecutionNode>> ExecNodes;
			ContextToRemove->GetAllExecNodes(ExecNodes);
			// Remove all exec nodes.
			for (auto ExecNodeIter : ExecNodes)
			{
				NodeProfilingData.Remove(ExecNodeIter.Key);
				PathToObjectContext.Remove(ExecNodeIter.Key.ToString());
			}
			// Remove compilation hook, this will get added again if the blueprint is profiled
			Blueprint->OnCompiled().RemoveAll(this);
			// Broadcast changes to stats structure
			GraphLayoutChangedDelegate.Broadcast(Blueprint);
		}
	}
}

#endif // WITH_EDITOR

void FBlueprintProfiler::ResetProfilingData()
{
	InstrumentationEvents.Reset();
	NodeProfilingData.Reset();
	PathToObjectContext.Reset();
	ActiveContext.ResetContext();
	PIEInstancePathMap.Reset();
}

void FBlueprintProfiler::RegisterDelegates(bool bEnabled)
{
	if (bEnabled)
	{
#if WITH_EDITOR
		// Register for PIE begin and end events in the editor
		FEditorDelegates::BeginPIE.AddRaw(this, &FBlueprintProfiler::BeginPIE);
		FEditorDelegates::EndPIE.AddRaw(this, &FBlueprintProfiler::EndPIE);
#endif // WITH_EDITOR
		ResetProfilingData();
		// Start consuming profiling events for capture
		FBlueprintCoreDelegates::OnScriptProfilingEvent.AddRaw(this, &FBlueprintProfiler::InstrumentEvent);
	}
	else
	{
#if WITH_EDITOR
		// Unregister for PIE begin and end events in the editor
		FEditorDelegates::BeginPIE.RemoveAll(this);
		FEditorDelegates::EndPIE.RemoveAll(this);
#endif // WITH_EDITOR
		ResetProfilingData();
		// Stop consuming profiling events for capture
		FBlueprintCoreDelegates::OnScriptProfilingEvent.RemoveAll(this);
	}
}

void FBlueprintProfiler::BeginPIE(bool bIsSimulating)
{
#if WITH_EDITOR
	bPIEActive = true;
	bProfilingCaptureActive = bProfilerActive && bPIEActive;
#endif // WITH_EDITOR
}

void FBlueprintProfiler::EndPIE(bool bIsSimulating)
{
#if WITH_EDITOR
	bPIEActive = false;
	bProfilingCaptureActive = bProfilerActive && bPIEActive;
#endif // WITH_EDITOR
}

TSharedPtr<FScriptExecutionContext> FBlueprintProfiler::MapBlueprintExecutionFlow(const FString& BlueprintPath)
{
	TSharedPtr<FScriptExecutionContext> Result = GetCachedObjectContext(BlueprintPath);

	if (!Result->IsContextValid())
	{
		// Create new blueprint root node
		TSharedPtr<FScriptExecutionNode>& BlueprintEntry = NodeProfilingData.FindOrAdd(FName(*BlueprintPath));
		if (!BlueprintEntry.IsValid())
		{
			BlueprintEntry = MakeShareable<FScriptExecutionNode>(new FScriptExecutionBlueprint);
		}
		Result->ExecutionNode = BlueprintEntry;
		BlueprintEntry->SetNameByString(BlueprintPath);
		BlueprintEntry->SetGraphName(NAME_None);
		BlueprintEntry->SetFlags(EScriptExecutionNodeFlags::Class);
		const FSlateBrush* Icon_Normal = FEditorStyle::GetBrush(TEXT("BlueprintProfiler.BPIcon_Normal"));
		BlueprintEntry->SetIcon(const_cast<FSlateBrush*>(Icon_Normal));
		BlueprintEntry->SetIconColor(FLinearColor(0.46f, 0.54f, 0.81f));
		BlueprintEntry->SetToolTipText(LOCTEXT("NavigateToBlueprintHyperlink_ToolTip", "Navigate to the Blueprint"));
		// Attempt to locate the blueprint
		UBlueprint* BlueprintToMap = nullptr;
		UBlueprintGeneratedClass* BPGC = nullptr;

		if (UObject* ObjectPtr = FindObject<UObject>(nullptr, *BlueprintPath))
		{
			BPGC = Cast<UBlueprintGeneratedClass>(ObjectPtr);
			if (BPGC)
			{
				Result->BlueprintClass = BPGC;
				Result->BlueprintFunction = BPGC->UberGraphFunction;
				BlueprintToMap = Cast<UBlueprint>(BPGC->ClassGeneratedBy);
				if (BlueprintToMap)
				{
					Result->ContextObject = BlueprintToMap;
					BlueprintEntry->SetDisplayName(FText::FromName(BlueprintToMap->GetFName()));
				}
			}
		}
		if (BlueprintToMap && BPGC)
		{
			// Register delegate to handle updates
			BlueprintToMap->OnCompiled().AddRaw(this, &FBlueprintProfiler::RemoveAllBlueprintReferences);
			// Find all event nodes for event graph and UCS
			TArray<UEdGraph*> Graphs;
			TArray<UK2Node_Event*> EventNodes;
			TArray<UK2Node_FunctionEntry*> FunctionEntryNodes;
			BlueprintToMap->GetAllGraphs(Graphs);
			for (auto GraphIter : Graphs)
			{
				if (GraphIter->GetFName() == UEdGraphSchema_K2::GN_EventGraph)
				{
					GraphIter->GetNodesOfClass<UK2Node_Event>(EventNodes);
				}
				if (GraphIter->GetFName() == UEdGraphSchema_K2::FN_UserConstructionScript)
				{
					GraphIter->GetNodesOfClass<UK2Node_FunctionEntry>(FunctionEntryNodes);
				}
			}
			// Map execution paths for each event node
			for (auto EventIter : EventNodes)
			{
				
				const FString EventPath = FString::Printf(TEXT("%s:%s"), *BlueprintToMap->GetName(), *EventIter->GetFunctionName().ToString());
				FName EventName(*EventPath);
				TSharedPtr<FScriptExecutionContext> EventContext = MakeShareable<FScriptExecutionContext>(new FScriptExecutionContext);
				*EventContext = *Result;
				EventContext->ContextObject = EventIter;
				EventContext->ExecutionNode = GetOrCreateProfilerDataForNode(EventName);
				EventContext->ExecutionNode->SetName(EventName);
				EventContext->ExecutionNode->SetGraphName(EventIter->GetTypedOuter<UEdGraph>()->GetFName());
				EventContext->ExecutionNode->SetFlags(EScriptExecutionNodeFlags::Event);
				FLinearColor IconColor = FLinearColor(0.91f, 0.16f, 0.16f);
				FText ToolTipText = LOCTEXT("NavigateToEventLocationHyperlink_ToolTip", "Navigate to the Event");
				EventContext->ExecutionNode->SetDisplayName(EventIter->GetNodeTitle(ENodeTitleType::ListView));
				EventContext->ExecutionNode->SetToolTipText(ToolTipText);
				const FSlateBrush* Icon_Node = EventIter->ShowPaletteIconOnNode() ?	FEditorStyle::GetBrush(EventIter->GetPaletteIcon(IconColor)) :
																				FEditorStyle::GetBrush(TEXT("BlueprintProfiler.BPNode"));
				EventContext->ExecutionNode->SetIcon(const_cast<FSlateBrush*>(Icon_Node));
				EventContext->ExecutionNode->SetIconColor(IconColor);
				// Add event to mapped blueprint
				Result->ExecutionNode->AddChildNode(EventContext->ExecutionNode);
				// Map the event execution flow
				TSharedPtr<FScriptExecutionNode> MappedEvent = MapNodeExecutionFlow(EventContext, EventIter);
				if (MappedEvent.IsValid())
				{
					EventContext->ExecutionNode->AddChildNode(MappedEvent);
				}
			}
			// Map execution paths for each function entry node
			for (auto FunctionIter : FunctionEntryNodes)
			{
				const FString FunctionPath = FString::Printf(TEXT("%s:%s"), *BlueprintToMap->GetName(), *FunctionIter->SignatureName.ToString());
				FName FunctionName(*FunctionPath);
				TSharedPtr<FScriptExecutionContext> FunctionContext = MakeShareable<FScriptExecutionContext>(new FScriptExecutionContext);
				*FunctionContext = *Result;
				FunctionContext->ContextObject = FunctionIter;
				FunctionContext->BlueprintFunction = BPGC->FindFunctionByName(FunctionIter->SignatureName, EIncludeSuperFlag::ExcludeSuper);
				FunctionContext->ExecutionNode = GetOrCreateProfilerDataForNode(FunctionName);
				FunctionContext->ExecutionNode->SetName(FunctionName);
				FunctionContext->ExecutionNode->SetGraphName(FunctionIter->GetTypedOuter<UEdGraph>()->GetFName());
				FunctionContext->ExecutionNode->SetFlags(EScriptExecutionNodeFlags::Event);
				FLinearColor IconColor = FLinearColor(0.91f, 0.16f, 0.16f);
				FText ToolTipText = LOCTEXT("NavigateToFunctionLocationHyperlink_ToolTip", "Navigate to the Function");
				FunctionContext->ExecutionNode->SetDisplayName(FunctionIter->GetNodeTitle(ENodeTitleType::ListView));
				FunctionContext->ExecutionNode->SetToolTipText(ToolTipText);
				const FSlateBrush* Icon = FunctionIter->ShowPaletteIconOnNode() ?	FEditorStyle::GetBrush(FunctionIter->GetPaletteIcon(IconColor)) :
																					FEditorStyle::GetBrush(TEXT("BlueprintProfiler.BPNode"));
				FunctionContext->ExecutionNode->SetIcon(const_cast<FSlateBrush*>(Icon));
				FunctionContext->ExecutionNode->SetIconColor(IconColor);
				// Add Function to mapped blueprint
				Result->ExecutionNode->AddChildNode(FunctionContext->ExecutionNode);
				// Map the Function execution flow
				TSharedPtr<FScriptExecutionNode> MappedFunction = MapNodeExecutionFlow(FunctionContext, FunctionIter);
				if (MappedFunction.IsValid())
				{
					FunctionContext->ExecutionNode->AddChildNode(MappedFunction);
				}
			}
		}
	}
	return Result;
}

TSharedPtr<FScriptExecutionNode> FBlueprintProfiler::MapNodeExecutionFlow(TSharedPtr<FScriptExecutionContext> InFunctionContext, UEdGraphNode* NodeToMap)
{
	TSharedPtr<FScriptExecutionNode> MappedNode;

	if (NodeToMap)
	{
		TSharedPtr<FScriptExecutionContext> NodeContext = GetCachedObjectContext(NodeToMap->GetPathName());
		NodeContext->ContextObject = NodeToMap;
		NodeContext->BlueprintClass = InFunctionContext->BlueprintClass;
		NodeContext->BlueprintFunction = InFunctionContext->BlueprintFunction;
		UBlueprintGeneratedClass* BlueprintClass = NodeContext->BlueprintClass.Get();
		UFunction* BlueprintFunction = NodeContext->BlueprintFunction.Get();

		if (NodeContext->ExecutionNode.IsValid())
		{
			MappedNode = NodeContext->ExecutionNode;
		}
		else
		{
			FName NodeName(FName(*NodeToMap->GetPathName()));
			NodeContext->ExecutionNode = GetOrCreateProfilerDataForNode(NodeName);
			NodeContext->ExecutionNode->SetName(NodeName);
			NodeContext->ExecutionNode->SetGraphName(NodeToMap->GetTypedOuter<UEdGraph>()->GetFName());
			NodeContext->ExecutionNode->SetFlags(EScriptExecutionNodeFlags::Node);
			FLinearColor IconColor = FLinearColor(1.f, 1.f, 1.f, 0.8f);
			FText ToolTipText = LOCTEXT("NavigateToNodeLocationHyperlink_ToolTip", "Navigate to the Node");
			MappedNode = NodeContext->ExecutionNode;
			// Evaluate Execution Pins
			TArray<UEdGraphPin*> ExecPins;
			for (auto Pin : NodeToMap->Pins)
			{
				if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
				{
					ExecPins.Add(Pin);
				}
			}
			const bool bBranchNode = ExecPins.Num() > 1;
			if (bBranchNode)
			{
				// Determine branch exec type
				IconColor = FLinearColor(1.f, 1.f, 1.f, 0.8f);
				if (NodeToMap->IsA<UK2Node_ExecutionSequence>())
				{
					MappedNode->AddFlags(EScriptExecutionNodeFlags::SequentialBranch);
				}
				else
				{
					MappedNode->AddFlags(EScriptExecutionNodeFlags::ConditionalBranch);
				}
			}
			for (auto Pin : ExecPins)
			{
				const UEdGraphPin* LinkedPin = Pin->LinkedTo.Num() > 0 ? Pin->LinkedTo[0] : nullptr;

				if (bBranchNode)
				{
					const int32 PinScriptCodeOffset = BlueprintClass->GetDebugData().FindPinToCodeAssociation(Pin, BlueprintFunction);
					check(PinScriptCodeOffset != INDEX_NONE);
					FName PinName(FName(*Pin->GetPathName()));
					TSharedPtr<FScriptExecutionNode> LinkedNode = GetOrCreateProfilerDataForNode(PinName);
					LinkedNode->SetName(PinName);
					LinkedNode->SetGraphName(Pin->GetTypedOuter<UEdGraph>()->GetFName());
					LinkedNode->SetDisplayName(Pin->GetDisplayName());
					LinkedNode->SetFlags(EScriptExecutionNodeFlags::ExecPin);
					LinkedNode->SetToolTipText(LOCTEXT("ExecPin_ToolTip", "Expand execution path"));
					const FSlateBrush* Icon = LinkedPin ?	FEditorStyle::GetBrush(TEXT("BlueprintProfiler.BPPinConnected")) : 
															FEditorStyle::GetBrush(TEXT("BlueprintProfiler.BPPinDisconnected"));
					LinkedNode->SetIcon(const_cast<FSlateBrush*>(Icon));
					LinkedNode->SetIconColor(FLinearColor(1.f, 1.f, 1.f, 0.8f));
					MappedNode->AddLinkedNode(PinScriptCodeOffset, LinkedNode);
					// Map pin exec flow
					if (LinkedPin)
					{
						TSharedPtr<FScriptExecutionNode> ExecPinNode = MapNodeExecutionFlow(InFunctionContext, LinkedPin->GetOwningNode());
						if (ExecPinNode.IsValid())
						{
							LinkedNode->AddChildNode(ExecPinNode);
						}
					}
				}
				else if (LinkedPin)
				{
					TSharedPtr<FScriptExecutionNode> ExecLinkedNode = MapNodeExecutionFlow(InFunctionContext, LinkedPin->GetOwningNode());
					if (ExecLinkedNode.IsValid())
					{
						const int32 PinScriptCodeOffset = BlueprintClass->GetDebugData().FindPinToCodeAssociation(Pin, BlueprintFunction);
						check(PinScriptCodeOffset != INDEX_NONE);
						MappedNode->AddLinkedNode(PinScriptCodeOffset, ExecLinkedNode);
					}
				}
			}
			// Evaluate Children for call sites
			if (UK2Node_CallFunction* FunctionCallSite = Cast<UK2Node_CallFunction>(NodeToMap))
			{
				const UEdGraphNode* EventNode = nullptr; 
				if (UEdGraph* CalledGraph = FunctionCallSite->GetFunctionGraph(EventNode))
				{
					// Update Exec node
					NodeContext->ExecutionNode->AddFlags(EScriptExecutionNodeFlags::FunctionCall);
					IconColor = FLinearColor(0.46f, 0.54f, 0.95f);
					ToolTipText = LOCTEXT("NavigateToFunctionLocationHyperlink_ToolTip", "Navigate to the Function Callsite");
					// Create temporary function context
					TSharedPtr<FScriptExecutionContext> NewFunctionContext = MakeShareable<FScriptExecutionContext>(new FScriptExecutionContext);
					*NewFunctionContext = *InFunctionContext;
					NewFunctionContext->BlueprintFunction = FunctionCallSite->FunctionReference.ResolveMember<UFunction>(BlueprintClass);
					NewFunctionContext->ExecutionNode = NodeContext->ExecutionNode;
					// Grab function entries
					TArray<UK2Node_FunctionEntry*> FunctionEntryNodes;
					CalledGraph->GetNodesOfClass<UK2Node_FunctionEntry>(FunctionEntryNodes);
	
					for (auto EntryNodeIter : FunctionEntryNodes)
					{
						TSharedPtr<FScriptExecutionNode> FunctionEntry = MapNodeExecutionFlow(NewFunctionContext, EntryNodeIter);
						NewFunctionContext->ExecutionNode->AddChildNode(FunctionEntry);
					}
				}
			}
			else if (UK2Node_MacroInstance* MacroCallSite = Cast<UK2Node_MacroInstance>(NodeToMap))
			{
				if (UEdGraph* MacroGraph = MacroCallSite->GetMacroGraph())
				{
					// Update Exec node
					NodeContext->ExecutionNode->AddFlags(EScriptExecutionNodeFlags::MacroCall);
					IconColor = FLinearColor(0.36f, 0.34f, 0.71f);
					ToolTipText = LOCTEXT("NavigateToMacroLocationHyperlink_ToolTip", "Navigate to the Macro Callsite");
					// Create temporary macro context
					TSharedPtr<FScriptExecutionContext> NewMacroContext = MakeShareable<FScriptExecutionContext>(new FScriptExecutionContext);
					*NewMacroContext = *InFunctionContext;
					UBlueprint* MacroBP = MacroGraph->GetTypedOuter<UBlueprint>();
					UBlueprintGeneratedClass* MacroBPGC = Cast<UBlueprintGeneratedClass>(MacroBP->GeneratedClass);
					NewMacroContext->ContextObject = MacroBP;
					NewMacroContext->BlueprintClass = MacroBPGC;
					NewMacroContext->BlueprintFunction = MacroBPGC->UberGraphFunction;
					NewMacroContext->ExecutionNode = NodeContext->ExecutionNode;
					// Grab function entries
					TArray<UK2Node_FunctionEntry*> FunctionEntryNodes;
					MacroGraph->GetNodesOfClass<UK2Node_FunctionEntry>(FunctionEntryNodes);
	
					for (auto EntryNodeIter : FunctionEntryNodes)
					{
						TSharedPtr<FScriptExecutionNode> MacroEntry = MapNodeExecutionFlow(NewMacroContext, EntryNodeIter);
						NewMacroContext->ExecutionNode->AddChildNode(MacroEntry);
					}
				}
			}
			// Finalise exec node data.
			MappedNode->SetDisplayName(NodeToMap->GetNodeTitle(ENodeTitleType::ListView));
			MappedNode->SetToolTipText(ToolTipText);
			const FSlateBrush* Icon = NodeToMap->ShowPaletteIconOnNode() ?	FEditorStyle::GetBrush(NodeToMap->GetPaletteIcon(IconColor)) :
																			FEditorStyle::GetBrush(TEXT("BlueprintProfiler.BPNode"));
			MappedNode->SetIcon(const_cast<FSlateBrush*>(Icon));
			MappedNode->SetIconColor(IconColor);
		}
	}
	return MappedNode;
}

void FBlueprintProfiler::MapBlueprintInstance(const FString& InstancePath)
{
	FString RemappedInstancePath = GetValidInstancePath(InstancePath);
	TSharedPtr<FScriptExecutionContext> Result = GetCachedObjectContext(RemappedInstancePath);

	if (!Result->ExecutionNode.IsValid())
	{
		// Create new instance node
		Result->ExecutionNode = GetOrCreateProfilerDataForNode(FName(*RemappedInstancePath));
		Result->ExecutionNode->SetNameByString(RemappedInstancePath);
		Result->ExecutionNode->SetGraphName(NAME_None);
		Result->ExecutionNode->SetFlags(EScriptExecutionNodeFlags::Instance);
		const FSlateBrush* Icon = FEditorStyle::GetBrush(TEXT("BlueprintProfiler.Actor"));
		Result->ExecutionNode->SetIcon(const_cast<FSlateBrush*>(Icon));
		Result->ExecutionNode->SetIconColor(FLinearColor(1.f, 1.f, 1.f, 0.8f));
		Result->ExecutionNode->SetToolTipText(LOCTEXT("NavigateToInstanceHyperlink_ToolTip", "Navigate to the Instance"));
		// Update widget data
		if (const AActor* Actor = Cast<AActor>(Result->ContextObject.Get()))
		{
			Result->ExecutionNode->SetDisplayName(FText::FromString(Actor->GetActorLabel()));
		}
		else
		{
			Result->ExecutionNode->SetDisplayName(FText::FromString(Result->ContextObject.Get()->GetName()));
		}
		// Link to parent blueprint entry
		if (UBlueprint* Blueprint = Cast<UBlueprint>(Result->ContextObject->GetClass()->ClassGeneratedBy))
		{
			Result->BlueprintClass = Cast<UBlueprintGeneratedClass>(Blueprint->GeneratedClass);
			const FString ClassPath = Result->BlueprintClass->GetPathName();
			TSharedPtr<FScriptExecutionNode> BlueprintNode = GetProfilerDataForNode(FName(*ClassPath));

			if (BlueprintNode.IsValid() && BlueprintNode->IsRootNode())
			{
				TSharedPtr<FScriptExecutionBlueprint> BlueprintRootNode = StaticCastSharedPtr<FScriptExecutionBlueprint>(BlueprintNode);
				BlueprintRootNode->AddInstance(Result->ExecutionNode);
				Result->ExecutionNode->SetParentNode(BlueprintNode);
				// Fill out events from the blueprint root node
				for (int32 NodeIdx = 0; NodeIdx < BlueprintRootNode->GetNumChildren(); ++NodeIdx)
				{
					Result->ExecutionNode->AddChildNode(BlueprintRootNode->GetChildByIndex(NodeIdx));
				}
			}
		}
	}
}

const FString FBlueprintProfiler::GetValidInstancePath(const FString PathIn)
{
	FString Result(PathIn);
	if (const FString* SearchResult = PIEInstancePathMap.Find(PathIn))
	{
		Result = *SearchResult;
	}
	else
	{
		// Attempt to locate the instance and map PIE objects to editor world objects
		if (const UObject* ObjectPtr = FindObject<UObject>(nullptr, *PathIn))
		{
			// Get Outer world
			if (UWorld* ObjectWorld = ObjectPtr->GetTypedOuter<UWorld>())
			{
				if (ObjectWorld->WorldType == EWorldType::PIE)
				{
					FWorldContext& EditorWorldContext = GEditor->GetEditorWorldContext();
					if (UWorld* EditorWorld = EditorWorldContext.World())
					{
						for (auto LevelIter : EditorWorld->GetLevels())
						{
							if (UObject* EditorObject = FindObject<UObject>(LevelIter, *ObjectPtr->GetName()))
							{
								Result = EditorObject->GetPathName();
								PIEInstancePathMap.Add(PathIn) = Result;
								TSharedPtr<FScriptExecutionContext> ObjContext = GetCachedObjectContext(Result);
								ObjContext->ContextObject = EditorObject;
								break;
							}
						}
					}
				}
				else
				{
					PIEInstancePathMap.Add(Result) = Result;
					TSharedPtr<FScriptExecutionContext> ObjContext = GetCachedObjectContext(PathIn);
					ObjContext->ContextObject = ObjectPtr;
				}
			}
		}

	}
	return Result;
}

void FBlueprintProfiler::NavigateToBlueprint(const FName NameIn) const
{
	const TSharedPtr<FScriptExecutionContext>* Result = PathToObjectContext.Find(NameIn.ToString());
	if (Result && Result->IsValid())
	{
		if ((*Result)->ContextObject.IsValid())
		{
			// Stubbed - complete...
		}
	}
}

void FBlueprintProfiler::NavigateToInstance(const FName NameIn) const
{
	const TSharedPtr<FScriptExecutionContext>* Result = PathToObjectContext.Find(NameIn.ToString());
	if (Result && Result->IsValid())
	{
		if ((*Result)->ContextObject.IsValid())
		{
			// Stubbed - complete...
		}
	}
}

void FBlueprintProfiler::NavigateToNode(const FName NameIn) const
{
	const TSharedPtr<FScriptExecutionContext>* Result = PathToObjectContext.Find(NameIn.ToString());
	if (Result && Result->IsValid())
	{
		if ((*Result)->ContextObject.IsValid())
		{
			FKismetEditorUtilities::BringKismetToFocusAttentionOnObject((*Result)->ContextObject.Get());
		}
	}
}

#undef LOCTEXT_NAMESPACE
