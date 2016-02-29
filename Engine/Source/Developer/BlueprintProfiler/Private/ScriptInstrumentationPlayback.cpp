// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "BlueprintProfilerPCH.h"
#include "EditorStyleSet.h"

#define LOCTEXT_NAMESPACE "ScriptInstrumentationPlayback"

DECLARE_CYCLE_STAT(TEXT("Statistic Update"), STAT_StatUpdateCost, STATGROUP_BlueprintProfiler);
DECLARE_CYCLE_STAT(TEXT("Node Lookup"), STAT_NodeLookupCost, STATGROUP_BlueprintProfiler);

//////////////////////////////////////////////////////////////////////////
// FBlueprintExecutionContext

bool FBlueprintExecutionContext::InitialiseContext(const FString& BlueprintPath)
{
	// Locate the blueprint from the path
	if (UObject* ObjectPtr = FindObject<UObject>(nullptr, *BlueprintPath))
	{
		UBlueprintGeneratedClass* BPClass = Cast<UBlueprintGeneratedClass>(ObjectPtr);
		if (BPClass && BPClass->bHasInstrumentation)
		{
			BlueprintClass = BPClass;
			Blueprint = Cast<UBlueprint>(BPClass->ClassGeneratedBy);
			UbergraphFunctionName = BPClass->UberGraphFunction ? BPClass->UberGraphFunction->GetFName() : NAME_None;
		}
	}
	if (Blueprint.IsValid() && BlueprintClass.IsValid())
	{
		// Create new blueprint exec node
		BlueprintNode = MakeShareable<FScriptExecutionBlueprint>(new FScriptExecutionBlueprint);
		BlueprintNode->SetNameByString(BlueprintPath);
		BlueprintNode->SetObservedObject(Blueprint.Get());
		BlueprintNode->SetGraphName(NAME_None);
		BlueprintNode->SetDisplayName(FText::FromName(Blueprint.Get()->GetFName()));
		BlueprintNode->SetFlags(EScriptExecutionNodeFlags::Class);
		const FSlateBrush* Icon_Normal = FEditorStyle::GetBrush(TEXT("BlueprintProfiler.BPIcon_Normal"));
		BlueprintNode->SetIcon(const_cast<FSlateBrush*>(Icon_Normal));
		BlueprintNode->SetIconColor(FLinearColor(0.46f, 0.54f, 0.81f));
		BlueprintNode->SetToolTipText(LOCTEXT("NavigateToBlueprintHyperlink_ToolTip", "Navigate to the Blueprint"));
		// Map the blueprint execution
		if (MapBlueprintExecution())
		{
			return true;
		}
	}
	return false;
}

bool FBlueprintExecutionContext::HasProfilerDataForInstance(const FName InstanceName) const
{
	bool bHasInstanceData = false;
	if (BlueprintNode.IsValid())
	{
		FName RemappedInstanceName = RemapInstancePath(InstanceName);
		TSharedPtr<FScriptExecutionNode> Result = BlueprintNode->GetInstanceByName(RemappedInstanceName);
		bHasInstanceData = Result.IsValid();
	}
	return bHasInstanceData;
}

FName FBlueprintExecutionContext::RemapInstancePath(const FName InstanceName) const
{
	FName Result = InstanceName;
	if (const FName* RemappedName = PIEInstanceNameMap.Find(InstanceName))
	{
		Result = *RemappedName;
	}
	return Result;
}

TWeakObjectPtr<const UObject> FBlueprintExecutionContext::GetInstance(FName& InstanceNameInOut)
{
	TWeakObjectPtr<const UObject> Result;
	FName CorrectedName = InstanceNameInOut;
	if (const FName* SearchName = PIEInstanceNameMap.Find(InstanceNameInOut)) 
	{
		CorrectedName = *SearchName;
	}
	if (TWeakObjectPtr<const UObject>* SearchResult = ActorInstances.Find(CorrectedName))
	{
		Result = *SearchResult;
	}
	else
	{
		// Attempt to locate the instance and map PIE objects to editor world objects
		if (const UObject* ObjectPtr = FindObject<UObject>(nullptr, *InstanceNameInOut.ToString()))
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
								CorrectedName = FName(*EditorObject->GetPathName());
								ActorInstances.Add(CorrectedName) = EditorObject;
								PIEInstanceNameMap.Add(InstanceNameInOut) = CorrectedName;
								InstanceNameInOut = CorrectedName;
								Result = EditorObject;
								break;
							}
						}
					}
				}
				else
				{
					ActorInstances.Add(InstanceNameInOut) = ObjectPtr;
					Result = ObjectPtr;
				}
			}
		}

	}
	return Result;
}

TSharedPtr<FBlueprintFunctionContext> FBlueprintExecutionContext::FindFunctionContext(const FName FunctionNameIn) const
{
	TSharedPtr<FBlueprintFunctionContext> Result;
	FName FunctionName = (FunctionNameIn ==  UEdGraphSchema_K2::GN_EventGraph) ? UbergraphFunctionName : FunctionNameIn;

	if (const TSharedPtr<FBlueprintFunctionContext>* SearchResult = FunctionContexts.Find(FunctionName))
	{
		Result = *SearchResult;
	}
	return Result;
}

TSharedPtr<FBlueprintFunctionContext> FBlueprintExecutionContext::GetOrCreateFunctionContext(const FName FunctionNameIn)
{
	FName FunctionName = (FunctionNameIn ==  UEdGraphSchema_K2::GN_EventGraph) ? UbergraphFunctionName : FunctionNameIn;
	TSharedPtr<FBlueprintFunctionContext>& Result = FunctionContexts.FindOrAdd(FunctionName);
	if (!Result.IsValid())
	{
		if (UFunction* Function = BlueprintClass->FindFunctionByName(FunctionName))
		{
			Result = MakeShareable(new FBlueprintFunctionContext(Function, BlueprintClass));
		}
	}
	check(Result.IsValid());
	return Result;
}

bool FBlueprintExecutionContext::HasProfilerDataForNode(const UEdGraphNode* GraphNode) const
{
	SCOPE_CYCLE_COUNTER(STAT_NodeLookupCost);
	const UEdGraph* OuterGraph = GraphNode ? GraphNode->GetTypedOuter<UEdGraph>() : nullptr;
	if (OuterGraph)
	{
		TSharedPtr<FBlueprintFunctionContext> FunctionContext = FindFunctionContext(OuterGraph->GetFName());
		if (FunctionContext.IsValid())
		{
			return FunctionContext->HasProfilerDataForNode(GraphNode->GetFName());
		}
	}
	return false;
}

TSharedPtr<FScriptExecutionNode> FBlueprintExecutionContext::GetProfilerDataForNode(const UEdGraphNode* GraphNode)
{
	SCOPE_CYCLE_COUNTER(STAT_NodeLookupCost);
	TSharedPtr<FScriptExecutionNode> Result;
	const UEdGraph* OuterGraph = GraphNode ? GraphNode->GetTypedOuter<UEdGraph>() : nullptr;
	if (OuterGraph)
	{
		TSharedPtr<FBlueprintFunctionContext> FunctionContext = FindFunctionContext(OuterGraph->GetFName());
		if (FunctionContext.IsValid() && FunctionContext->HasProfilerDataForNode(GraphNode->GetFName()))
		{
			Result = FunctionContext->GetProfilerDataForNode(GraphNode->GetFName());
		}
	}
	return Result;
}

void FBlueprintExecutionContext::UpdateConnectedStats()
{
	SCOPE_CYCLE_COUNTER(STAT_StatUpdateCost);
	if (BlueprintNode.IsValid())
	{
		FTracePath InitialTracePath;
		BlueprintNode->RefreshStats(InitialTracePath);
	}
}

bool FBlueprintExecutionContext::MapBlueprintExecution()
{
	bool bMappingSuccessful = false;

	if (UBlueprint* BlueprintToMap = Blueprint.Get())
	{
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
			FName EventName = EventIter->GetFunctionName();
			TSharedPtr<FBlueprintFunctionContext> FunctionContext = GetOrCreateFunctionContext(EventName);
			TSharedPtr<FScriptExecutionNode> EventExecNode = FunctionContext->GetProfilerDataForNode(EventName);
			EventExecNode->SetName(EventName);
			EventExecNode->SetObservedObject(EventIter);
			EventExecNode->SetGraphName(EventIter->GetTypedOuter<UEdGraph>()->GetFName());
			EventExecNode->SetFlags(EScriptExecutionNodeFlags::Event);
			FLinearColor IconColor = FLinearColor(0.91f, 0.16f, 0.16f);
			FText ToolTipText = LOCTEXT("NavigateToEventLocationHyperlink_ToolTip", "Navigate to the Event");
			EventExecNode->SetDisplayName(EventIter->GetNodeTitle(ENodeTitleType::ListView));
			EventExecNode->SetToolTipText(ToolTipText);
			const FSlateBrush* Icon_Node = EventIter->ShowPaletteIconOnNode() ?	FEditorStyle::GetBrush(EventIter->GetPaletteIcon(IconColor)) :
																			FEditorStyle::GetBrush(TEXT("BlueprintProfiler.BPNode"));
			EventExecNode->SetIcon(const_cast<FSlateBrush*>(Icon_Node));
			EventExecNode->SetIconColor(IconColor);
			// Add event to mapped blueprint
			BlueprintNode->AddChildNode(EventExecNode);
			// Map the event execution flow
			TSharedPtr<FScriptExecutionNode> MappedEvent = MapNodeExecution(EventIter);
			if (MappedEvent.IsValid())
			{
				EventExecNode->AddChildNode(MappedEvent);
			}
		}
		// Map execution paths for each function entry node
		for (auto FunctionIter : FunctionEntryNodes)
		{
			FName FunctionName = FunctionIter->SignatureName;
			TSharedPtr<FBlueprintFunctionContext> FunctionContext = GetOrCreateFunctionContext(FunctionName);
			TSharedPtr<FScriptExecutionNode> FunctionExecNode = FunctionContext->GetProfilerDataForNode(FunctionName);
			FunctionExecNode->SetName(FunctionName);
			FunctionExecNode->SetObservedObject(FunctionIter);
			FunctionExecNode->SetGraphName(FunctionIter->GetTypedOuter<UEdGraph>()->GetFName());
			FunctionExecNode->SetFlags(EScriptExecutionNodeFlags::Event);
			FLinearColor IconColor = FLinearColor(0.91f, 0.16f, 0.16f);
			FText ToolTipText = LOCTEXT("NavigateToFunctionLocationHyperlink_ToolTip", "Navigate to the Function");
			FunctionExecNode->SetDisplayName(FunctionIter->GetNodeTitle(ENodeTitleType::ListView));
			FunctionExecNode->SetToolTipText(ToolTipText);
			const FSlateBrush* Icon = FunctionIter->ShowPaletteIconOnNode() ?	FEditorStyle::GetBrush(FunctionIter->GetPaletteIcon(IconColor)) :
																				FEditorStyle::GetBrush(TEXT("BlueprintProfiler.BPNode"));
			FunctionExecNode->SetIcon(const_cast<FSlateBrush*>(Icon));
			FunctionExecNode->SetIconColor(IconColor);
			// Add Function to mapped blueprint
			BlueprintNode->AddChildNode(FunctionExecNode);
			// Map the Function execution flow
			TSharedPtr<FScriptExecutionNode> MappedFunction = MapNodeExecution(FunctionIter);
			if (MappedFunction.IsValid())
			{
				FunctionExecNode->AddChildNode(MappedFunction);
			}
		}
		bMappingSuccessful = true;
	}
	return bMappingSuccessful;
}

TSharedPtr<FScriptExecutionNode> FBlueprintExecutionContext::MapNodeExecution(UEdGraphNode* NodeToMap)
{
	TSharedPtr<FScriptExecutionNode> MappedNode;
	if (NodeToMap)
	{
		const UEdGraph* OwningGraph = NodeToMap->GetTypedOuter<UEdGraph>();
		TSharedPtr<FBlueprintFunctionContext> FunctionContext = GetOrCreateFunctionContext(OwningGraph->GetFName());
		const bool bNodeMapped = FunctionContext->HasProfilerDataForNode(NodeToMap->GetFName());
		MappedNode = FunctionContext->GetProfilerDataForNode(NodeToMap->GetFName());

		if (!bNodeMapped)
		{
			// Construct mapped node values
			MappedNode->SetName(NodeToMap->GetFName());
			MappedNode->SetObservedObject(NodeToMap);
			MappedNode->SetGraphName(NodeToMap->GetTypedOuter<UEdGraph>()->GetFName());
			MappedNode->SetFlags(EScriptExecutionNodeFlags::Node);
			FLinearColor IconColor = FLinearColor(1.f, 1.f, 1.f, 0.8f);
			FText ToolTipText = LOCTEXT("NavigateToNodeLocationHyperlink_ToolTip", "Navigate to the Node");
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
					const int32 PinScriptCodeOffset = FunctionContext->GetCodeLocationFromPin(Pin);
					// Check we have a valid script offset or that the pin isn't linked.
					check((PinScriptCodeOffset != INDEX_NONE)||(LinkedPin == nullptr));
					TSharedPtr<FScriptExecutionNode> LinkedNode = FunctionContext->GetProfilerDataForNode(Pin->GetFName());
					LinkedNode->SetName(Pin->GetFName());
					LinkedNode->SetObservedObject(Pin);
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
						TSharedPtr<FScriptExecutionNode> ExecPinNode = MapNodeExecution(LinkedPin->GetOwningNode());
						if (ExecPinNode.IsValid())
						{
							LinkedNode->AddChildNode(ExecPinNode);
						}
					}
				}
				else if (LinkedPin)
				{
					TSharedPtr<FScriptExecutionNode> ExecLinkedNode = MapNodeExecution(LinkedPin->GetOwningNode());
					if (ExecLinkedNode.IsValid())
					{
						const int32 PinScriptCodeOffset = FunctionContext->GetCodeLocationFromPin(Pin);
						check(PinScriptCodeOffset != INDEX_NONE);
						MappedNode->AddLinkedNode(PinScriptCodeOffset, ExecLinkedNode);
					}
				}
			}
			// Evaluate Children for call sites
			if (UK2Node_CallFunction* FunctionCallSite = Cast<UK2Node_CallFunction>(NodeToMap))
			{
				const UEdGraphNode* FunctionNode = nullptr; 
				if (UEdGraph* CalledGraph = FunctionCallSite->GetFunctionGraph(FunctionNode))
				{
					// Update Exec node
					MappedNode->AddFlags(EScriptExecutionNodeFlags::FunctionCall);
					IconColor = FLinearColor(0.46f, 0.54f, 0.95f);
					ToolTipText = LOCTEXT("NavigateToFunctionLocationHyperlink_ToolTip", "Navigate to the Function Callsite");
					// Grab function entries
					TArray<UK2Node_FunctionEntry*> FunctionEntryNodes;
					CalledGraph->GetNodesOfClass<UK2Node_FunctionEntry>(FunctionEntryNodes);
	
					for (auto EntryNodeIter : FunctionEntryNodes)
					{
						TSharedPtr<FScriptExecutionNode> FunctionEntry = MapNodeExecution(EntryNodeIter);
						MappedNode->AddChildNode(FunctionEntry);
					}
				}
			}
			//else if (UK2Node_MacroInstance* MacroCallSite = Cast<UK2Node_MacroInstance>(NodeToMap))
			//{
			//	if (UEdGraph* MacroGraph = MacroCallSite->GetMacroGraph())
			//	{
			//		// Update Exec node
			//		MappedNode->AddFlags(EScriptExecutionNodeFlags::MacroCall);
			//		IconColor = FLinearColor(0.36f, 0.34f, 0.71f);
			//		ToolTipText = LOCTEXT("NavigateToMacroLocationHyperlink_ToolTip", "Navigate to the Macro Callsite");
			//		// Grab the macro blueprint context
			//		UBlueprint* MacroBP = MacroGraph->GetTypedOuter<UBlueprint>();
			//		UBlueprintGeneratedClass* MacroBPGC = Cast<UBlueprintGeneratedClass>(MacroBP->GeneratedClass);
			//		TSharedPtr<FBlueprintExecutionContext> MacroBlueprintContext = GetBlueprintContext(MacroBPGC->GetPathName());
			//		// Grab function entries
			//		TArray<UK2Node_FunctionEntry*> FunctionEntryNodes;
			//		MacroGraph->GetNodesOfClass<UK2Node_FunctionEntry>(FunctionEntryNodes);
	
			//		for (auto EntryNodeIter : FunctionEntryNodes)
			//		{
			//			TSharedPtr<FScriptExecutionNode> MacroEntry = MapNodeExecution(MacroBlueprintContext, EntryNodeIter);
			//			MappedNode->AddChildNode(MacroEntry);
			//		}
			//	}
			//}
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
//////////////////////////////////////////////////////////////////////////
// FBlueprintFunctionContext

bool FBlueprintFunctionContext::HasProfilerDataForNode(const FName NodeName) const
{
	return ExecutionNodes.Contains(NodeName);
}

TSharedPtr<FScriptExecutionNode> FBlueprintFunctionContext::GetProfilerDataForNode(const FName NodeName)
{
	TSharedPtr<FScriptExecutionNode>& Result = ExecutionNodes.FindOrAdd(NodeName);
	if (!Result.IsValid())
	{
		Result = MakeShareable(new FScriptExecutionNode);
	}
	return Result;
}

const UEdGraphNode* FBlueprintFunctionContext::GetNodeFromCodeLocation(const int32 ScriptOffset)
{
	TWeakObjectPtr<const UEdGraphNode>& Result = ScriptOffsetToNodes.FindOrAdd(ScriptOffset);
	if (!Result.IsValid() && BlueprintClass.IsValid())
	{
		if (const UEdGraphNode* GraphNode = BlueprintClass->GetDebugData().FindSourceNodeFromCodeLocation(Function.Get(), ScriptOffset, true))
		{
			Result = GraphNode;
		}
	}
	return Result.Get();
}

const int32 FBlueprintFunctionContext::GetCodeLocationFromPin(const UEdGraphPin* Pin) const
{
	int32 Result = INDEX_NONE;
	if (BlueprintClass.IsValid())
	{
		Result = BlueprintClass.Get()->GetDebugData().FindPinToCodeAssociation(Pin, Function.Get());
	}
	return Result;
}

//////////////////////////////////////////////////////////////////////////
// FScriptEventPlayback

bool FScriptEventPlayback::Process(const TArray<FScriptInstrumentedEvent>& Events, const int32 Start, const int32 Stop)
{
	struct PerNodeEventHelper
	{
		TArray<FTracePath> EntryTracePaths;
		TSharedPtr<FScriptExecutionNode> ExecNode;
		TSharedPtr<FBlueprintFunctionContext> FunctionContext;
		TArray<FScriptInstrumentedEvent> UnproccessedEvents;
	};

	const int32 NumEvents = (Stop + 1) - Start;
	if (BlueprintContext.IsValid() && NumEvents > 2)
	{
		check(Events[Start].GetType() == EScriptInstrumentation::Class);
		check(Events[Start+1].GetType() == EScriptInstrumentation::Instance);
		check(Events[Start+2].GetType() == EScriptInstrumentation::Event);
		// Update event and instance names
		EventName = Events[Start+2].GetFunctionName();
		InstanceName = BlueprintContext->RemapInstancePath(FName(*Events[Start+1].GetObjectPath()));
		// Process event
		FName CurrentFunctionName = EventName;
		TSharedPtr<FBlueprintFunctionContext> FunctionContext = BlueprintContext->GetOrCreateFunctionContext(CurrentFunctionName);
		TSharedPtr<FScriptExecutionNode> EventNode = FunctionContext->GetProfilerDataForNode(EventName);
		FTracePath EventTrace;
		TSharedPtr<FScriptPerfData> PerfData = EventNode->GetPerfDataByInstanceAndTracePath(InstanceName, EventTrace);
		PerfData->AddEventTiming(0.0, Events[Stop].GetTime() - Events[Start+2].GetTime());
		// Find Associated graph nodes and submit into a node map for later processing.
		FTracePath TracePath;
		TArray<FTracePath> TraceStack;
		TMap<const UEdGraphNode*, PerNodeEventHelper> NodeMap;

		for (int32 EventIdx = Start + 3; EventIdx < Stop; ++EventIdx)
		{
			const FScriptInstrumentedEvent& CurrEvent = Events[EventIdx];
			bool bFunctionChanged = false;
			// Update script function.
			if (CurrentFunctionName != CurrEvent.GetFunctionName())
			{
				bFunctionChanged = true;
				CurrentFunctionName = CurrEvent.GetFunctionName();
				FunctionContext = BlueprintContext->FindFunctionContext(CurrentFunctionName);
				check(FunctionContext.IsValid());
			}
			if (const UEdGraphNode* GraphNode = FunctionContext->GetNodeFromCodeLocation(CurrEvent.GetScriptCodeOffset()))
			{
				PerNodeEventHelper& NodeEntry = NodeMap.FindOrAdd(GraphNode);
				if (!NodeEntry.FunctionContext.IsValid())
				{
					NodeEntry.FunctionContext = FunctionContext;
					NodeEntry.ExecNode = FunctionContext->GetProfilerDataForNode(GraphNode->GetFName());
				}
				switch (CurrEvent.GetType())
				{
					case EScriptInstrumentation::PureNodeEntry:
					{
						NodeEntry.UnproccessedEvents.Add(CurrEvent);
						break;
					}
					case EScriptInstrumentation::NodeEntry:
					{
						NodeEntry.EntryTracePaths.Push(TracePath);
						if (bFunctionChanged && GraphNode->IsA<UK2Node_FunctionEntry>())
						{
							TraceStack.Push(TracePath);
						}
						NodeEntry.UnproccessedEvents.Add(CurrEvent);
						break;
					}
					case EScriptInstrumentation::PushState:
					{
						TraceStack.Push(TracePath);
						NodeEntry.UnproccessedEvents.Add(CurrEvent);
						break;
					}
					case EScriptInstrumentation::RestoreState:
					{
						check(TraceStack.Num());
						TracePath = TraceStack.Last();
						break;
					}
					case EScriptInstrumentation::PopState:
					{
						if (TraceStack.Num())
						{
							TracePath = TraceStack.Pop();
						}
						FScriptInstrumentedEvent OverrideEvent(CurrEvent);
						OverrideEvent.OverrideType(EScriptInstrumentation::NodeExit);
						NodeEntry.UnproccessedEvents.Add(OverrideEvent);
						// Consolidate enclosed timings
						int32 NodeEntryIdx = INDEX_NONE;
						for (int32 NodeEventIdx = NodeEntry.UnproccessedEvents.Num()-2; NodeEventIdx >= 0; --NodeEventIdx)
						{
							const FScriptInstrumentedEvent& NodeEvent = NodeEntry.UnproccessedEvents[NodeEventIdx];

							if (NodeEvent.GetType() == EScriptInstrumentation::NodeEntry)
							{
								if (NodeEntryIdx != INDEX_NONE)
								{
									// Keep only the first node entry.
									NodeEntry.UnproccessedEvents.RemoveAt(NodeEventIdx, 1);
								}
								NodeEntryIdx = NodeEventIdx;
							}
							else if (NodeEvent.GetType() == EScriptInstrumentation::NodeExit)
							{
								// Strip out all node exits apart from the PopExecution overridden event.
								NodeEntry.UnproccessedEvents.RemoveAt(NodeEventIdx, 1);
							}
						}
						break;
					}
					case EScriptInstrumentation::NodeExit:
					{
						if (TraceStack.Num() && bFunctionChanged)
						{
							TracePath = TraceStack.Pop();
						}
						// Cleanup branching multiple exits and correct the tracepath
						if (NodeEntry.UnproccessedEvents.Num() && NodeEntry.UnproccessedEvents.Last().GetType() == EScriptInstrumentation::NodeExit)
						{
							NodeEntry.UnproccessedEvents.Pop();
							if (NodeEntry.EntryTracePaths.Num())
							{
								TracePath = NodeEntry.EntryTracePaths.Last();
							}
						}
						TracePath.AddExitPin(CurrEvent.GetScriptCodeOffset());
						NodeEntry.UnproccessedEvents.Add(CurrEvent);
						break;
					}
					default:
					{
						NodeEntry.UnproccessedEvents.Add(CurrEvent);
						break;
					}
				}
			}
		}
		// Process Node map timings
		for (auto NodeEntry : NodeMap)
		{
			TSharedPtr<FScriptExecutionNode> ExecNode = NodeEntry.Value.ExecNode;
			double PureNodeEntryTime = 0.0;
			double NodeEntryTime = 0.0;
			for (auto EventIter = NodeEntry.Value.UnproccessedEvents.CreateIterator(); EventIter; ++EventIter)
			{
				switch(EventIter->GetType())
				{
					case EScriptInstrumentation::PureNodeEntry:
					{
						if (PureNodeEntryTime == 0.0)
						{
							PureNodeEntryTime = EventIter->GetTime();
						}
						break;
					}
					case EScriptInstrumentation::NodeEntry:
					{
						if (NodeEntryTime == 0.0)
						{
							NodeEntryTime = EventIter->GetTime();
						}
						break;
					}
					case EScriptInstrumentation::NodeExit:
					{
						check(NodeEntryTime != 0.0 );
						PerfData = ExecNode->GetPerfDataByInstanceAndTracePath(InstanceName, NodeEntry.Value.EntryTracePaths.Pop());
						double PureNodeDuration = PureNodeEntryTime != 0.0 ? (NodeEntryTime - PureNodeEntryTime) : 0.0;
						double NodeDuration = EventIter->GetTime() - NodeEntryTime;
						PerfData->AddEventTiming(PureNodeDuration, NodeDuration);
						PureNodeEntryTime = NodeEntryTime = 0.0;
						break;
					}
				}
			}
		}
		return true;
	}
	return false;
}

#undef LOCTEXT_NAMESPACE
