// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "BlueprintProfilerPCH.h"
#include "EditorStyleSet.h"
#include "GraphEditorSettings.h"

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
	if (Blueprint.IsValid() && BlueprintClass.IsValid() && UbergraphFunctionName != NAME_None)
	{
		// Create new blueprint exec node
		FScriptExecNodeParams BlueprintParams;
		BlueprintParams.NodeName = FName(*BlueprintPath);
		BlueprintParams.ObservedObject = Blueprint.Get();
		BlueprintParams.OwningGraphName = NAME_None;
		BlueprintParams.DisplayName = FText::FromName(Blueprint.Get()->GetFName());
		BlueprintParams.Tooltip = LOCTEXT("NavigateToBlueprintHyperlink_ToolTip", "Navigate to the Blueprint");
		BlueprintParams.NodeFlags = EScriptExecutionNodeFlags::Class;
		BlueprintParams.Icon = const_cast<FSlateBrush*>(FEditorStyle::GetBrush(TEXT("BlueprintProfiler.BPIcon_Normal")));
		BlueprintParams.IconColor = FLinearColor(0.46f, 0.54f, 0.81f);
		BlueprintNode = MakeShareable<FScriptExecutionBlueprint>(new FScriptExecutionBlueprint(BlueprintParams));
		// Map the blueprint execution
		if (MapBlueprintExecution())
		{
			return true;
		}
	}
	return false;
}

void FBlueprintExecutionContext::AddEventNode(TSharedPtr<FScriptExecutionNode> EventExecNode)
{
	check(BlueprintNode.IsValid());
	BlueprintNode->AddChildNode(EventExecNode);
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
		InstanceNameInOut = CorrectedName;
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
				else if (!ObjectPtr->HasAnyFlags(RF_Transient))
				{
					ActorInstances.Add(InstanceNameInOut) = ObjectPtr;
					PIEInstanceNameMap.Add(InstanceNameInOut) = InstanceNameInOut;
					Result = ObjectPtr;
				}
			}
		}
	}
	return Result;
}

FName FBlueprintExecutionContext::GetEventFunctionName(const FName EventName) const
{
	return (EventName == UEdGraphSchema_K2::FN_UserConstructionScript) ? UEdGraphSchema_K2::FN_UserConstructionScript : UbergraphFunctionName;
}

TSharedPtr<FBlueprintFunctionContext> FBlueprintExecutionContext::GetFunctionContext(const FName FunctionNameIn) const
{
	TSharedPtr<FBlueprintFunctionContext> Result;
	FName FunctionName = (FunctionNameIn == UEdGraphSchema_K2::GN_EventGraph) ? UbergraphFunctionName : FunctionNameIn;

	if (const TSharedPtr<FBlueprintFunctionContext>* SearchResult = FunctionContexts.Find(FunctionName))
	{
		Result = *SearchResult;
	}
	return Result;
}

TSharedPtr<FBlueprintFunctionContext> FBlueprintExecutionContext::CreateFunctionContext(UEdGraph* Graph)
{
	FName FunctionName = Graph->GetFName();
	FunctionName = (FunctionName == UEdGraphSchema_K2::GN_EventGraph) ? UbergraphFunctionName : FunctionName;
	TSharedPtr<FBlueprintFunctionContext>& Result = FunctionContexts.FindOrAdd(FunctionName);
	if (!Result.IsValid())
	{
		Result = MakeShareable(new FBlueprintFunctionContext);
		Result->InitialiseContextFromGraph(AsShared(), Graph);
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
		TSharedPtr<FBlueprintFunctionContext> FunctionContext = GetFunctionContext(OuterGraph->GetFName());
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
		TSharedPtr<FBlueprintFunctionContext> FunctionContext = GetFunctionContext(OuterGraph->GetFName());
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
	UBlueprint* BlueprintToMap = Blueprint.Get();
	UBlueprintGeneratedClass* BPGC = BlueprintClass.Get();

	if (BlueprintToMap && BPGC)
	{
		// Map all blueprint graphs and event entry points
		TArray<UEdGraph*> Graphs;
		TArray<UEdGraph*> FunctionGraphs;
		TSet<UEdGraph*> Macros;
		BlueprintToMap->GetAllGraphs(Graphs);
		for (auto Graph : Graphs)
		{
			FName FunctionName = Graph->GetFName();
			FunctionName = FunctionName == UEdGraphSchema_K2::GN_EventGraph ? UbergraphFunctionName : FunctionName;
			if (UFunction* ScriptFunction = BPGC->FindFunctionByName(FunctionName))
			{
				FunctionGraphs.Add(Graph);
			}
		}
		// Create function stubs
		for (auto Graph : FunctionGraphs)
		{
			CreateFunctionContext(Graph);
		}
		// Map the function context nodes
		for (auto Context : FunctionContexts)
		{
			Context.Value->MapFunction();
		}
		bMappingSuccessful = true;
	}
	return bMappingSuccessful;
}

//////////////////////////////////////////////////////////////////////
// FBlueprintFunctionContext

void FBlueprintFunctionContext::InitialiseContextFromGraph(TSharedPtr<FBlueprintExecutionContext> BlueprintContextIn, UEdGraph* Graph)
{
	if (Graph)
	{
		// Initialise Blueprint references
		BlueprintContext = BlueprintContextIn;
		UBlueprint* Blueprint = BlueprintContextIn->GetBlueprint().Get();
		UBlueprintGeneratedClass* BPClass = BlueprintContextIn->GetBlueprintClass().Get();
		if (Blueprint && BPClass)
		{
			// Instantiate context
			BlueprintClass = BPClass;
			const bool bEventGraph = Graph->GetFName() == UEdGraphSchema_K2::GN_EventGraph;
			FunctionName = bEventGraph && BPClass->UberGraphFunction ? BPClass->UberGraphFunction->GetFName() : Graph->GetFName();
			Function = BPClass->FindFunctionByName(FunctionName);
			Function = Function.IsValid() ? Function : BPClass->UberGraphFunction;
			if (bEventGraph)
			{
				// Map events
				TArray<UK2Node_Event*> GraphEventNodes;
				Graph->GetNodesOfClass<UK2Node_Event>(GraphEventNodes);

				for (auto EventNode : GraphEventNodes)
				{
					FName EventName = EventNode->GetFunctionName();
					FScriptExecNodeParams EventParams;
					EventParams.NodeName = EventName;
					EventParams.ObservedObject = EventNode;
					EventParams.DisplayName = EventNode->GetNodeTitle(ENodeTitleType::ListView);
					EventParams.Tooltip = LOCTEXT("NavigateToEventLocationHyperlink_ToolTip", "Navigate to the Event");
					EventParams.NodeFlags = EScriptExecutionNodeFlags::Event;
					EventParams.IconColor = FLinearColor(0.91f, 0.16f, 0.16f);
					const FSlateBrush* EventIcon = EventNode->ShowPaletteIconOnNode() ?	FEditorStyle::GetBrush(EventNode->GetPaletteIcon(EventParams.IconColor)) :
																						FEditorStyle::GetBrush(TEXT("BlueprintProfiler.BPNode"));
					EventParams.Icon = const_cast<FSlateBrush*>(EventIcon);
					TSharedPtr<FScriptExecutionNode> EventExecNode = CreateExecutionNode(EventParams);
					AddEntryPoint(EventExecNode);
					BlueprintContextIn->AddEventNode(EventExecNode);
				}
				// Create Input events
				TArray<UK2Node*> InputEventNodes;
				Graph->GetNodesOfClassEx<UK2Node_InputAction, UK2Node>(InputEventNodes);
				Graph->GetNodesOfClassEx<UK2Node_InputKey, UK2Node>(InputEventNodes);
				Graph->GetNodesOfClassEx<UK2Node_InputTouch, UK2Node>(InputEventNodes);
				if (InputEventNodes.Num())
				{
					CreateInputEvents(BlueprintContextIn, InputEventNodes);
				}
			}
			else
			{
				// Map execution paths for each function entry nodes
				TArray<UK2Node_FunctionEntry*> FunctionEntryNodes;
				Graph->GetNodesOfClass<UK2Node_FunctionEntry>(FunctionEntryNodes);
				const bool bConstructionScript = FunctionName == UEdGraphSchema_K2::FN_UserConstructionScript;
				// Create function node
				for (auto FunctionEntry : FunctionEntryNodes)
				{
					FScriptExecNodeParams FunctionNodeParams;
					FunctionNodeParams.NodeName = FunctionName;
					FunctionNodeParams.ObservedObject = FunctionEntry;
					FunctionNodeParams.DisplayName = FText::FromName(FunctionName);
					FunctionNodeParams.Tooltip = LOCTEXT("NavigateToFunctionLocationHyperlink_ToolTip", "Navigate to the Function");
					FunctionNodeParams.NodeFlags = EScriptExecutionNodeFlags::Event;
					FunctionNodeParams.IconColor = FLinearColor(0.91f, 0.16f, 0.16f);
					const FSlateBrush* Icon = FunctionEntry->ShowPaletteIconOnNode() ?	FEditorStyle::GetBrush(FunctionEntry->GetPaletteIcon(FunctionNodeParams.IconColor)) :
																						FEditorStyle::GetBrush(TEXT("BlueprintProfiler.BPNode"));
					FunctionNodeParams.Icon = const_cast<FSlateBrush*>(Icon);
					TSharedPtr<FScriptExecutionNode> FunctionEntryNode = CreateExecutionNode(FunctionNodeParams);
					AddEntryPoint(FunctionEntryNode);
					if (bConstructionScript)
					{
						BlueprintContextIn->AddEventNode(FunctionEntryNode);
					}
				}
			}
			// Map all instanced sub graphs
			TArray<UK2Node_Tunnel*> GraphTunnels;
			Graph->GetNodesOfClass<UK2Node_Tunnel>(GraphTunnels);
			// Map sub graphs / composites and macros
			for (auto Tunnel : GraphTunnels)
			{
				MapTunnelInstance(Tunnel);
			}
		}
	}
}

void FBlueprintFunctionContext::MapFunction()
{
	// Map all entry point execution paths
	for (auto EntryPoint : EntryPoints)
	{
		if (const UEdGraphNode* EntryPointNode = Cast<UEdGraphNode>(EntryPoint->GetObservedObject()))
		{
			TSharedPtr<FScriptExecutionNode> ExecutionNode = MapNodeExecution(const_cast<UEdGraphNode*>(EntryPointNode));
			if (ExecutionNode.IsValid())
			{
				EntryPoint->AddChildNode(ExecutionNode);
			}
		}
		// Check for Cyclic Linkage
		TSet<TSharedPtr<FScriptExecutionNode>> Filter;
		DetectCyclicLinks(EntryPoint, Filter);
	}
}

bool FBlueprintFunctionContext::DetectCyclicLinks(TSharedPtr<FScriptExecutionNode> ExecNode, TSet<TSharedPtr<FScriptExecutionNode>>& Filter)
{
	if (Filter.Contains(ExecNode))
	{
		return true;
	}
	else
	{
		Filter.Add(ExecNode);
		for (auto Child : ExecNode->GetChildNodes())
		{
			DetectCyclicLinks(Child, Filter);
		}
		for (auto LinkedNode : ExecNode->GetLinkedNodes())
		{
			if (!LinkedNode.Value->HasFlags(EScriptExecutionNodeFlags::PureStats))
			{
				TSharedPtr<FScriptExecutionNode> LinkedExecNode = LinkedNode.Value;
				if (LinkedExecNode->HasFlags(EScriptExecutionNodeFlags::FunctionTunnel))
				{
					TSharedPtr<FScriptExecutionNode> TunnelBoundry = LinkedExecNode->GetLinkedNodeByScriptOffset(LinkedNode.Key);
					if (TunnelBoundry.IsValid())
					{
						TSharedPtr<FScriptExecutionNode> TunnelExec = TunnelBoundry->GetLinkedNodeByScriptOffset(LinkedNode.Key);
						if (TunnelExec.IsValid())
						{
							ExecNode = TunnelExec;
						}
					}
				}
				if (DetectCyclicLinks(LinkedExecNode, Filter))
				{
					// Break links and flag cycle linkage.
					FScriptExecNodeParams CycleLinkParams;
					const FString NodeName = FString::Printf(TEXT("CyclicLinkTo_%i_%s"), LinkedNode.Key, *LinkedExecNode->GetName().ToString());
					CycleLinkParams.NodeName = FName(*NodeName);
					CycleLinkParams.ObservedObject = LinkedExecNode->GetObservedObject();
					CycleLinkParams.DisplayName = LinkedExecNode->GetDisplayName();
					CycleLinkParams.Tooltip = LOCTEXT("CyclicLink_ToolTip", "Cyclic Link");
					CycleLinkParams.NodeFlags = EScriptExecutionNodeFlags::CyclicLinkage|EScriptExecutionNodeFlags::ExecPin|EScriptExecutionNodeFlags::InvalidTrace;
					CycleLinkParams.IconColor = FLinearColor(0.91f, 0.16f, 0.16f);
					const FSlateBrush* LinkIcon = LinkedExecNode->GetIcon();
					CycleLinkParams.Icon = const_cast<FSlateBrush*>(LinkIcon);
					TSharedPtr<FScriptExecutionNode> NewLink = CreateExecutionNode(CycleLinkParams);
					ExecNode->AddLinkedNode(LinkedNode.Key, NewLink);
				}
			}
		}
	}
	return false;
}

void FBlueprintFunctionContext::CreateInputEvents(TSharedPtr<FBlueprintExecutionContext> BlueprintContextIn, const TArray<UK2Node*>& InputEventNodes)
{
	struct FInputEventDesc
	{
		UEdGraphNode* GraphNode;
		UFunction* EventFunction;
	};
	TArray<FInputEventDesc> InputEventDescs;
	// Extract basic event info
	if (UBlueprintGeneratedClass* BPClass = BlueprintClass.Get())
	{
		FInputEventDesc NewInputDesc;
		for (auto EventNode : InputEventNodes)
		{
			if (EventNode)
			{
				if (UK2Node_InputAction* InputActionNode = Cast<UK2Node_InputAction>(EventNode))
				{
					const FString CustomEventName = FString::Printf(TEXT("InpActEvt_%s_%s"), *InputActionNode->InputActionName.ToString());
					for (auto FunctionIter = TFieldIterator<UFunction>(BPClass); FunctionIter; ++FunctionIter)
					{
						if (FunctionIter->GetName().Contains(CustomEventName))
						{
							NewInputDesc.GraphNode = EventNode;
							NewInputDesc.EventFunction = *FunctionIter;
							InputEventDescs.Add(NewInputDesc);
						}
					}
				}
				else if (UK2Node_InputKey* InputKeyNode = Cast<UK2Node_InputKey>(EventNode))
				{
					const FName ModifierName = InputKeyNode->GetModifierName();
					const FString CustomEventName = ModifierName != NAME_None ? FString::Printf(TEXT("InpActEvt_%s_%s"), *ModifierName.ToString(), *InputKeyNode->InputKey.ToString()) :
																				FString::Printf(TEXT("InpActEvt_%s"), *InputKeyNode->InputKey.ToString());
					for (auto FunctionIter = TFieldIterator<UFunction>(BPClass); FunctionIter; ++FunctionIter)
					{
						if (FunctionIter->GetName().Contains(CustomEventName))
						{
							NewInputDesc.GraphNode = EventNode;
							NewInputDesc.EventFunction = *FunctionIter;
							InputEventDescs.Add(NewInputDesc);
						}
					}
				}
				else if (UK2Node_InputTouch* InputTouchNode = Cast<UK2Node_InputTouch>(EventNode))
				{
					struct EventPinData
					{
						EventPinData(UEdGraphPin* InPin,TEnumAsByte<EInputEvent> InEvent ){	Pin=InPin;EventType=InEvent;};
						UEdGraphPin* Pin;
						TEnumAsByte<EInputEvent> EventType;
					};
					TArray<UEdGraphPin*> ActivePins;
					if (UEdGraphPin* InputTouchPressedPin = InputTouchNode->GetPressedPin())
					{
						if (InputTouchPressedPin->LinkedTo.Num() > 0)
						{
							ActivePins.Add(InputTouchPressedPin);
						}
					}
					if (UEdGraphPin* InputTouchReleasedPin = InputTouchNode->GetReleasedPin())
					{
						if (InputTouchReleasedPin->LinkedTo.Num() > 0)
						{
							ActivePins.Add(InputTouchReleasedPin);
						}
					}
					if (UEdGraphPin* InputTouchMovedPin = InputTouchNode->GetMovedPin())
					{
						if (InputTouchMovedPin->LinkedTo.Num() > 0)
						{
							ActivePins.Add(InputTouchMovedPin);
						}
					}
					for (auto Pin : ActivePins)
					{
						const FString CustomEventName = FString::Printf(TEXT("InpTchEvt_%s"), *Pin->GetName());
						for (auto FunctionIter = TFieldIterator<UFunction>(BPClass); FunctionIter; ++FunctionIter)
						{
							if (FunctionIter->GetName().Contains(CustomEventName))
							{
								NewInputDesc.GraphNode = EventNode;
								NewInputDesc.EventFunction = *FunctionIter;
								InputEventDescs.Add(NewInputDesc);
							}
						}
					}
				}
			}
		}
	}
	// Build event contexts
	for (auto InputEventDesc : InputEventDescs)
	{
		UEdGraphNode* GraphNode = InputEventDesc.GraphNode;
		FName EventName = InputEventDesc.EventFunction->GetFName();
		FScriptExecNodeParams EventParams;
		EventParams.NodeName = EventName;
		EventParams.ObservedObject = GraphNode;
		EventParams.OwningGraphName = NAME_None;
		EventParams.DisplayName = GraphNode->GetNodeTitle(ENodeTitleType::ListView);
		EventParams.Tooltip = LOCTEXT("NavigateToEventLocationHyperlink_ToolTip", "Navigate to the Event");
		EventParams.NodeFlags = EScriptExecutionNodeFlags::Event;
		EventParams.IconColor = FLinearColor(0.91f, 0.16f, 0.16f);
		const FSlateBrush* EventIcon = GraphNode->ShowPaletteIconOnNode() ?	FEditorStyle::GetBrush(GraphNode->GetPaletteIcon(EventParams.IconColor)) :
																			FEditorStyle::GetBrush(TEXT("BlueprintProfiler.BPNode"));
		EventParams.Icon = const_cast<FSlateBrush*>(EventIcon);
		TSharedPtr<FScriptExecutionNode> EventExecNode = CreateExecutionNode(EventParams);
		AddEntryPoint(EventExecNode);
		BlueprintContextIn->AddEventNode(EventExecNode);
	}
}

TSharedPtr<FScriptExecutionNode> FBlueprintFunctionContext::MapNodeExecution(UEdGraphNode* NodeToMap)
{
	TSharedPtr<FScriptExecutionNode> MappedNode;
	if (NodeToMap)
	{
		MappedNode = GetProfilerDataForNode(NodeToMap->GetFName());
		if (!MappedNode.IsValid())
		{
			FScriptExecNodeParams NodeParams;
			NodeParams.NodeName = NodeToMap->GetFName();
			NodeParams.ObservedObject = NodeToMap;
			NodeParams.DisplayName = NodeToMap->GetNodeTitle(ENodeTitleType::ListView);
			NodeParams.Tooltip = LOCTEXT("NavigateToNodeLocationHyperlink_ToolTip", "Navigate to the Node");
			NodeParams.NodeFlags = EScriptExecutionNodeFlags::Node;
			NodeParams.IconColor = FLinearColor(1.f, 1.f, 1.f, 0.8f);
			const FSlateBrush* NodeIcon = NodeToMap->ShowPaletteIconOnNode() ?	FEditorStyle::GetBrush(NodeToMap->GetPaletteIcon(NodeParams.IconColor)) :
																				FEditorStyle::GetBrush(TEXT("BlueprintProfiler.BPNode"));
			NodeParams.Icon = const_cast<FSlateBrush*>(NodeIcon);
			MappedNode = CreateExecutionNode(NodeParams);
			// Evaluate Execution and Input Pins
			TArray<UEdGraphPin*> ExecPins;
			TArray<UEdGraphPin*> InputPins;
			for (auto Pin : NodeToMap->Pins)
			{
				if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
				{
					ExecPins.Add(Pin);
				}
				else if (Pin->Direction == EGPD_Input && Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
				{
					InputPins.Add(Pin);
				}
			}
			// Flag as pure node or build pure node script range.
			if (ExecPins.Num() == 0)
			{
				MappedNode->AddFlags(EScriptExecutionNodeFlags::PureNode);
			}
			else
			{
				MappedNode->SetPureNodeScriptCodeRange(GetPureNodeScriptCodeRange(NodeToMap));
			}
			// Determine a few node characteristics to guide processing.
			if (ExecPins.Num() > 1)
			{
				const bool bExecSequence = NodeToMap->IsA<UK2Node_ExecutionSequence>();
				MappedNode->AddFlags(bExecSequence ? EScriptExecutionNodeFlags::SequentialBranch : EScriptExecutionNodeFlags::ConditionalBranch);
			}
			// Evaluate non-exec input pins (pure node execution chains)
			if (InputPins.Num())
			{
				MapInputPins(MappedNode, InputPins);
			}
			// Evaluate exec output pins (execution chains)
			if (ExecPins.Num())
			{
				MapExecPins(MappedNode, ExecPins);
			}
			// Evaluate Children for call sites
			if (UK2Node_CallFunction* FunctionCallSite = Cast<UK2Node_CallFunction>(NodeToMap))
			{
				const UEdGraphNode* FunctionNode = nullptr; 
				if (UEdGraph* CalledGraph = FunctionCallSite->GetFunctionGraph(FunctionNode))
				{
					// Update Exec node
					const bool bEventCall = FunctionNode ? FunctionNode->IsA<UK2Node_Event>() : false;
					MappedNode->AddFlags(EScriptExecutionNodeFlags::FunctionCall);
					MappedNode->SetIconColor(FLinearColor(0.46f, 0.54f, 0.95f));
					MappedNode->SetToolTipText(LOCTEXT("NavigateToFunctionCallsiteHyperlink_ToolTip", "Navigate to the Function Callsite"));
					// Don't add entry points for events.
					if (!bEventCall)
					{
						// Update the function context
						TSharedPtr<FBlueprintFunctionContext> NewFunctionContext = BlueprintContext.Pin()->GetFunctionContext(CalledGraph->GetFName());
						if (NewFunctionContext.IsValid())
						{
							NewFunctionContext->AddCallSiteEntryPointsToNode(MappedNode);
						}
					}
				}
			}
			else if (UK2Node_CustomEvent* CustomEvent = Cast<UK2Node_CustomEvent>(NodeToMap))
			{
				MappedNode->AddFlags(EScriptExecutionNodeFlags::CustomEvent);
			}
		}
	}
	return MappedNode;
}

void FBlueprintFunctionContext::MapInputPins(TSharedPtr<FScriptExecutionNode> ExecNode, const TArray<UEdGraphPin*>& Pins)
{
	TArray<int32> PinScriptCodeOffsets;
	TSharedPtr<FScriptExecutionNode> PureChainRootNode = ExecNode;

	for (auto InputPin : Pins)
	{
		// If this input pin is linked to a pure node in the source graph, create and map all known execution paths for it.
		for (auto LinkedPin : InputPin->LinkedTo)
		{
			// Note: Intermediate pure nodes can have output pins that masquerade as impure node output pins when links are "moved" from the source graph (thus
			// resulting in a false association here with one or more script code offsets), so we must first ensure that the link is really to a pure node output.
			UK2Node* OwningNode = Cast<UK2Node>(LinkedPin->GetOwningNode());
			if (OwningNode && OwningNode->IsNodePure())
			{
				GetAllCodeLocationsFromPin(LinkedPin, PinScriptCodeOffsets);
				if (PinScriptCodeOffsets.Num() > 0)
				{
					// Add a pure chain container node as the root, if it's not already in place.
					if (PureChainRootNode == ExecNode && !ExecNode->IsPureNode())
					{
						FScriptExecNodeParams PureChainParams;
						static const FString PureChainNodeNameSuffix = TEXT("__PROFILER_PureTime");
						FString PureChainNodeNameString = ExecNode->GetName().ToString() + PureChainNodeNameSuffix;
						PureChainParams.NodeName = FName(*PureChainNodeNameString);
						PureChainParams.DisplayName = LOCTEXT("PureChain_DisplayName", "Pure Time");
						PureChainParams.Tooltip = LOCTEXT("PureChain_ToolTip", "Expand pure node timing");
						PureChainParams.NodeFlags = EScriptExecutionNodeFlags::PureChain;
						PureChainParams.IconColor = FLinearColor(1.f, 1.f, 1.f, 0.8f);
						const FSlateBrush* Icon = FEditorStyle::GetBrush(TEXT("BlueprintProfiler.BPNode"));
						PureChainParams.Icon = const_cast<FSlateBrush*>(Icon);
						PureChainRootNode = CreateExecutionNode(PureChainParams);
						PureChainRootNode->SetPureNodeScriptCodeRange(ExecNode->GetPureNodeScriptCodeRange());
						ExecNode->AddChildNode(PureChainRootNode);
					}

					TSharedPtr<FScriptExecutionNode> PureNode = MapNodeExecution(OwningNode);
					check(PureNode.IsValid() && PureNode->IsPureNode());
					for (int32 i = 0; i < PinScriptCodeOffsets.Num(); ++i)
					{
						PureChainRootNode->AddLinkedNode(PinScriptCodeOffsets[i], PureNode);
					}
				}
			}
		}
	}
}

void FBlueprintFunctionContext::MapExecPins(TSharedPtr<FScriptExecutionNode> ExecNode, const TArray<UEdGraphPin*>& Pins)
{
	const bool bBranchedExecution = ExecNode->IsBranch();
	int32 NumUnwiredPins = 0;
	for (auto Pin : Pins)
	{
		int32 PinScriptCodeOffset = GetCodeLocationFromPin(Pin);
		const bool bInvalidTrace = PinScriptCodeOffset == INDEX_NONE;
		PinScriptCodeOffset = bInvalidTrace ? NumUnwiredPins++ : PinScriptCodeOffset;
		TSharedPtr<FScriptExecutionNode> PinExecNode;
		const UEdGraphPin* LinkedPin = Pin->LinkedTo.Num() > 0 ? Pin->LinkedTo[0] : nullptr;
		UEdGraphNode* LinkedPinNode = LinkedPin ? LinkedPin->GetOwningNode() : nullptr;
		const bool bTunnelBoundry = LinkedPinNode ? LinkedPinNode->IsA<UK2Node_Tunnel>() : false;
		// Create any neccesary dummy pins for branches
		if (bBranchedExecution)
		{
			FScriptExecNodeParams LinkNodeParams;
			LinkNodeParams.NodeName = Pin->GetFName();
			LinkNodeParams.ObservedObject = Pin;
			LinkNodeParams.DisplayName = Pin->GetDisplayName();
			LinkNodeParams.Tooltip = LOCTEXT("ExecPin_ExpandExecutionPath_ToolTip", "Expand execution path");
			LinkNodeParams.NodeFlags = !bInvalidTrace ? EScriptExecutionNodeFlags::ExecPin : (EScriptExecutionNodeFlags::ExecPin|EScriptExecutionNodeFlags::InvalidTrace);
			LinkNodeParams.IconColor = FLinearColor(1.f, 1.f, 1.f, 0.8f);
			const FSlateBrush* Icon = LinkedPin ?	FEditorStyle::GetBrush(TEXT("BlueprintProfiler.BPPinConnected")) : 
													FEditorStyle::GetBrush(TEXT("BlueprintProfiler.BPPinDisconnected"));
			LinkNodeParams.Icon = const_cast<FSlateBrush*>(Icon);
			PinExecNode = CreateExecutionNode(LinkNodeParams);
			ExecNode->AddLinkedNode(PinScriptCodeOffset, PinExecNode);
			if (!bTunnelBoundry)
			{
				TSharedPtr<FScriptExecutionNode> LinkedPinExecNode = MapNodeExecution(LinkedPinNode);
				if (LinkedPinExecNode.IsValid())
				{
					PinExecNode->AddChildNode(LinkedPinExecNode);
				}
			}
		}
		else
		{
			PinExecNode = ExecNode;
			if (!bTunnelBoundry)
			{
				TSharedPtr<FScriptExecutionNode> LinkedPinExecNode = MapNodeExecution(LinkedPinNode);
				if (LinkedPinExecNode.IsValid())
				{
					PinExecNode->AddLinkedNode(PinScriptCodeOffset, LinkedPinExecNode);
				}
			}
		}
		// Handle tunnel boundries
		if (bTunnelBoundry)
		{
			if (StagingTunnelName != NAME_None)
			{
				const FName TunnelExitName(*LinkedPin->PinName);
				TSharedPtr<FScriptExecutionNode> TunnelExitNode = GetProfilerDataForNode(TunnelExitName);
				// Check for re-entrant exit site
				if (ExecNode->HasFlags(EScriptExecutionNodeFlags::SequentialBranch))
				{
					// Last pin in sequence is not re-entrant.
					if (Pin != Pins.Last())
					{
						TunnelExitNode->AddFlags(EScriptExecutionNodeFlags::ReEntrantTunnelPin);
					}
				}
				if (bBranchedExecution)
				{
					PinExecNode->AddChildNode(TunnelExitNode);
				}
				else
				{
					const int32 TunnelId = GetTunnelIdFromName(TunnelExitName);
					check (TunnelId != INDEX_NONE);
					PinExecNode->AddLinkedNode(TunnelId, TunnelExitNode);
				}
				TArray<TSharedPtr<FScriptExecutionNode>>& TunnelExitPoints = TunnelExitPointMap.FindOrAdd(StagingTunnelName);
				TunnelExitPoints.Add(TunnelExitNode);
			}
			else
			{
				const FName TunnelName(*LinkedPin->PinName);
				TSharedPtr<FScriptExecutionNode> TunnelEntryNode = MapTunnelEntry(TunnelName, Pin, LinkedPin, PinScriptCodeOffset);
				if (TunnelEntryNode.IsValid())
				{
					if (bBranchedExecution)
					{
						PinExecNode->AddChildNode(TunnelEntryNode);
					}
					else
					{
						PinExecNode->AddLinkedNode(PinScriptCodeOffset, TunnelEntryNode);
					}
				}
			}
		}
	}
}

TSharedPtr<FScriptExecutionNode> FBlueprintFunctionContext::MapTunnelEntry(const FName TunnelName, const UEdGraphPin* ExecPin, const UEdGraphPin* TunnelPin, const int32 PinScriptOffset)
{
	// Retrieve tunnel function context
	const UEdGraphNode* TunnelNode = TunnelPin->GetOwningNode();
	TSharedPtr<FBlueprintFunctionContext> TunnelContext = GetTunnelContextFromNode(TunnelNode);
	const int32 TunnelId = TunnelContext->GetTunnelIdFromName(TunnelName);
	check(TunnelId != INDEX_NONE);
	// Create tunnel graph instance node
	FScriptExecNodeParams TunnelParams;
	TunnelParams.NodeName = FName(*FString::Printf(TEXT("%s_EntryPointId%i"), *TunnelNode->GetFName().ToString(), TunnelId));
	TunnelParams.ObservedObject = TunnelNode;
	TunnelParams.DisplayName = TunnelNode->GetNodeTitle(ENodeTitleType::ListView);
	TunnelParams.Tooltip = LOCTEXT("NavigateToNodeLocationHyperlink_ToolTip", "Navigate to the Node");
	TunnelParams.NodeFlags = EScriptExecutionNodeFlags::FunctionTunnel;
	TunnelParams.IconColor = FLinearColor(1.f, 1.f, 1.f, 0.8f);
	const FSlateBrush* TunnelIcon = TunnelNode->ShowPaletteIconOnNode() ?	FEditorStyle::GetBrush(TunnelNode->GetPaletteIcon(TunnelParams.IconColor)) :
																			FEditorStyle::GetBrush(TEXT("BlueprintProfiler.BPNode"));
	TunnelParams.Icon = const_cast<FSlateBrush*>(TunnelIcon);
	TSharedPtr<FScriptExecutionNode> TunnelCallNode = CreateExecutionNode(TunnelParams);
	// Add entry point children
	TSharedPtr<FScriptExecutionNode> EntryPinNode = TunnelContext->GetProfilerDataForNode(TunnelName);
	check (EntryPinNode.IsValid());
	TunnelCallNode->AddChildNode(EntryPinNode);
	if (!EntryPinNode->GetNumChildren())
	{
		TSharedPtr<FScriptExecutionNode> FirstTunnelNode = TunnelContext->GetTunnelEntryPoint(TunnelName);
		check(FirstTunnelNode.IsValid());
		EntryPinNode->AddChildNode(FirstTunnelNode);
	}
	// Map exit points
	TArray<TSharedPtr<FScriptExecutionNode>> TunnelExitPoints;
	TunnelContext->GetTunnelExitPoints(TunnelName, TunnelExitPoints);
	if (const UK2Node_Tunnel* TunnelGraphNode = Cast<UK2Node_Tunnel>(TunnelNode))
	{
		TMap<FName, const UEdGraphPin*> NameToPinMap;
		for (auto ExitPin : TunnelGraphNode->Pins)
		{
			if (ExitPin->Direction == EGPD_Output && ExitPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec && ExitPin->LinkedTo.Num())
			{
				NameToPinMap.Add(FName(*ExitPin->PinName)) = ExitPin;
			}
		}
		for (auto CurrExitPoint : TunnelExitPoints)
		{
			if (const UEdGraphPin** ExitPin = NameToPinMap.Find(CurrExitPoint->GetName()))
			{
				for (auto LinkedExitPin : (*ExitPin)->LinkedTo)
				{
					TSharedPtr<FScriptExecutionNode> LinkedExecNode = MapNodeExecution(LinkedExitPin->GetOwningNode());
					if (LinkedExecNode.IsValid())
					{
						const int32 ExitScriptCodeOffset = GetCodeLocationFromPin(*ExitPin);
						if (CurrExitPoint->HasFlags(EScriptExecutionNodeFlags::ReEntrantTunnelPin))
						{
							CurrExitPoint->AddLinkedNode(ExitScriptCodeOffset, LinkedExecNode);
						}
						else
						{
							LinkedExecNode->AddFlags(EScriptExecutionNodeFlags::TunnelThenPin);
							TunnelCallNode->AddLinkedNode(ExitScriptCodeOffset, LinkedExecNode);
						}
					}
				}
			}
		}
	}
	// Return the tunnel entry.
	return TunnelCallNode;
}

bool FBlueprintFunctionContext::HasProfilerDataForNode(const FName NodeName) const
{
	return ExecutionNodes.Contains(NodeName);
}

TSharedPtr<FScriptExecutionNode> FBlueprintFunctionContext::GetProfilerDataForNode(const FName NodeName)
{
	TSharedPtr<FScriptExecutionNode> Result;
	if (TSharedPtr<FScriptExecutionNode>* SearchResult = ExecutionNodes.Find(NodeName))
	{
		Result = *SearchResult;
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
			if (GraphNode->IsA<UK2Node_MacroInstance>())
			{
				GraphNode = BlueprintClass->GetDebugData().FindMacroSourceNodeFromCodeLocation(Function.Get(), ScriptOffset);
			}
			Result = GraphNode;
		}
	}
	return Result.Get();
}

const UEdGraphPin* FBlueprintFunctionContext::GetPinFromCodeLocation(const int32 ScriptOffset)
{
	TWeakObjectPtr<const UEdGraphPin>& Result = ScriptOffsetToPins.FindOrAdd(ScriptOffset);
	if (!Result.IsValid() && BlueprintClass.IsValid())
	{
		if (const UEdGraphPin* GraphPin = BlueprintClass->GetDebugData().FindSourcePinFromCodeLocation(Function.Get(), ScriptOffset))
		{
			Result = GraphPin;
		}
	}
	return Result.Get();
}

const int32 FBlueprintFunctionContext::GetCodeLocationFromPin(const UEdGraphPin* Pin) const
{
	if (BlueprintClass.IsValid())
	{
		return BlueprintClass.Get()->GetDebugData().FindCodeLocationFromSourcePin(Pin, Function.Get());
	}

	return INDEX_NONE;
}

void FBlueprintFunctionContext::GetAllCodeLocationsFromPin(const UEdGraphPin* Pin, TArray<int32>& OutCodeLocations) const
{
	if (BlueprintClass.IsValid())
	{
		BlueprintClass.Get()->GetDebugData().FindAllCodeLocationsFromSourcePin(Pin, Function.Get(), OutCodeLocations);
	}
}

FInt32Range FBlueprintFunctionContext::GetPureNodeScriptCodeRange(const UEdGraphNode* Node) const
{
	if (BlueprintClass.IsValid())
	{
		return BlueprintClass->GetDebugData().FindPureNodeScriptCodeRangeFromSourceNode(Node, Function.Get());
	}

	return FInt32Range(INDEX_NONE);
}

TSharedPtr<FScriptExecutionNode> FBlueprintFunctionContext::CreateExecutionNode(FScriptExecNodeParams& InitParams)
{
	TSharedPtr<FScriptExecutionNode>& ScriptExecNode = ExecutionNodes.FindOrAdd(InitParams.NodeName);
	check(!ScriptExecNode.IsValid());
	UEdGraph* OuterGraph = InitParams.ObservedObject.IsValid() ? InitParams.ObservedObject.Get()->GetTypedOuter<UEdGraph>() : nullptr;
	InitParams.OwningGraphName = OuterGraph ? OuterGraph->GetFName() : NAME_None;
	ScriptExecNode = MakeShareable(new FScriptExecutionNode(InitParams));
	return ScriptExecNode;
}

void FBlueprintFunctionContext::AddCallSiteEntryPointsToNode(TSharedPtr<FScriptExecutionNode> CallingNode) const
{
	if (CallingNode.IsValid())
	{
		for (auto EntryPoint : EntryPoints)
		{
			for (auto EntryPointChild : EntryPoint->GetChildNodes())
			{
				CallingNode->AddChildNode(EntryPointChild);
			}
		}
	}
}

void FBlueprintFunctionContext::MapTunnelInstance(UK2Node_Tunnel* TunnelInstance)
{
	check(TunnelInstance);
	UEdGraph* TunnelGraph = nullptr;
	if (UK2Node_MacroInstance* MacroInstance = Cast<UK2Node_MacroInstance>(TunnelInstance))
	{
		TunnelGraph = MacroInstance->GetMacroGraph();
	}
	else if (UK2Node_Composite* CompositeInstance = Cast<UK2Node_Composite>(TunnelInstance))
	{
		TunnelGraph = CompositeInstance->BoundGraph;
	}
	TSharedPtr<FBlueprintFunctionContext> TunnelGraphContext;
	if (TunnelGraph)
	{
		TunnelGraphContext = BlueprintContext.Pin()->GetFunctionContext(TunnelGraph->GetFName());
		if (!TunnelGraphContext.IsValid())
		{
			// Create the sub graph context
			TunnelGraphContext = MakeShareable(new FBlueprintFunctionContext);
			TunnelGraphContext->FunctionName = TunnelGraph->GetFName();
			TunnelGraphContext->BlueprintContext = BlueprintContext;
			TunnelGraphContext->Function = Function;
			TunnelGraphContext->BlueprintClass = BlueprintClass;
			BlueprintContext.Pin()->AddFunctionContext(TunnelGraph->GetFName(), TunnelGraphContext);
			// Map tunnel Inputs
			TunnelGraphContext->MapTunnelIO(TunnelGraph);
			// Map tunneling pins into the tunnel graph
			for (auto EntryPoint : TunnelGraphContext->EntryPoints)
			{
				TArray<TSharedPtr<FScriptExecutionNode>>& MappedExits = TunnelGraphContext->TunnelExitPointMap.FindOrAdd(EntryPoint->GetName());
				if (const UEdGraphPin* EntryPointPin = Cast<UEdGraphPin>(EntryPoint->GetObservedObject()))
				{
					TunnelGraphContext->StagingTunnelName = EntryPoint->GetName();
					for (auto LinkedPin : EntryPointPin->LinkedTo)
					{
						const UEdGraphNode* LinkedNode = LinkedPin->GetOwningNode();
						if (LinkedNode->IsA<UK2Node_Tunnel>())
						{
							// This handles input linked directly to output
							TSharedPtr<FScriptExecutionNode> OutputPinNode = TunnelGraphContext->GetProfilerDataForNode(FName(*LinkedPin->PinName));
							check(OutputPinNode.IsValid());
							MappedExits.Add(OutputPinNode);
						}
						else
						{
							// This adds any encountered exit points to the active tunnel name.
							TSharedPtr<FScriptExecutionNode> ExecNode = TunnelGraphContext->MapNodeExecution(LinkedPin->GetOwningNode());
							if (ExecNode.IsValid())
							{
								TunnelGraphContext->TunnelEntryPointMap.Add(EntryPoint->GetName()) = ExecNode;
							}
						}
					}
				}
			}
			TunnelGraphContext->StagingTunnelName = NAME_None;
		}
		// Patch info into the calling function context
		for (auto ExecutionNode : TunnelGraphContext->ExecutionNodes)
		{
			const int32 TunnelId = TunnelGraphContext->GetTunnelIdFromName(ExecutionNode.Value->GetName());
			if (TunnelId == INDEX_NONE)
			{
				TSharedPtr<FScriptExecutionNode>* ExistingNode = ExecutionNodes.Find(ExecutionNode.Key);
				if (ExistingNode == nullptr)
				{
					ExecutionNodes.Add(ExecutionNode.Key) = ExecutionNode.Value;
				}
			}
		}
	}
}

TSharedPtr<FBlueprintFunctionContext> FBlueprintFunctionContext::GetTunnelContextFromNode(const UEdGraphNode* TunnelNode) const
{
	TSharedPtr<FBlueprintFunctionContext> TunnelContext;
	if (const UK2Node_Composite* CompositeNode = Cast<UK2Node_Composite>(TunnelNode))
	{
		TunnelContext = BlueprintContext.Pin()->GetFunctionContext(CompositeNode->BoundGraph->GetFName());
	}
	else if (const UK2Node_MacroInstance* MacroNode = Cast<UK2Node_MacroInstance>(TunnelNode))
	{
		TunnelContext = BlueprintContext.Pin()->GetFunctionContext(MacroNode->GetMacroGraph()->GetFName());
	}
	return TunnelContext;
}

TSharedPtr<FScriptExecutionNode> FBlueprintFunctionContext::GetTunnelEntryPoint(const FName TunnelName) const
{
	TSharedPtr<FScriptExecutionNode> EntryPoint;
	if (const TSharedPtr<FScriptExecutionNode>* FoundEntryPoint = TunnelEntryPointMap.Find(TunnelName))
	{
		EntryPoint = *FoundEntryPoint;
	}
	return EntryPoint;
}

void FBlueprintFunctionContext::GetTunnelExitPoints(const FName TunnelName, TArray<TSharedPtr<FScriptExecutionNode>>& ExitPointsOut) const
{
	ExitPointsOut.Reset();
	if (const TArray<TSharedPtr<FScriptExecutionNode>>* FoundExitPoints = TunnelExitPointMap.Find(TunnelName))
	{
		ExitPointsOut = *FoundExitPoints;
	}
}

void FBlueprintFunctionContext::MapTunnelIO(UEdGraph* TunnelGraph)
{
	// Map graph entry points
	check (TunnelGraph);
	TArray<UK2Node_Tunnel*> GraphTunnels;
	TunnelGraph->GetNodesOfClass<UK2Node_Tunnel>(GraphTunnels);
	for (auto Tunnel : GraphTunnels)
	{
		for (auto Pin : Tunnel->Pins)
		{
			if (Pin->LinkedTo.Num() && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
			{
				if (Pin->Direction == EGPD_Output)
				{
					// Create entry point
					FScriptExecNodeParams LinkParams;
					LinkParams.NodeName = FName(*Pin->PinName);
					LinkParams.ObservedObject = Pin;
					LinkParams.DisplayName = Pin->GetDisplayName().IsEmpty() ? FText::FromString(*Pin->PinName) : Pin->GetDisplayName();
					LinkParams.Tooltip = LOCTEXT("ExecPin_ExpandTunnelEntryPoint_ToolTip", "Expand tunnel entry point");
					LinkParams.NodeFlags = EScriptExecutionNodeFlags::TunnelPin;
					LinkParams.IconColor = FLinearColor(1.f, 1.f, 1.f, 0.8f);
					const FSlateBrush* Icon = FEditorStyle::GetBrush(TEXT("BlueprintProfiler.BPPinConnected"));
					LinkParams.Icon = const_cast<FSlateBrush*>(Icon);
					TSharedPtr<FScriptExecutionNode> EntryPoint = CreateExecutionNode(LinkParams);
					AddEntryPoint(EntryPoint);
				}
				else if (Pin->Direction == EGPD_Input)
				{
					// Create exit point
					FScriptExecNodeParams LinkParams;
					LinkParams.NodeName = FName(*Pin->PinName);
					LinkParams.ObservedObject = Pin;
					LinkParams.DisplayName = Pin->GetDisplayName().IsEmpty() ? FText::FromString(*Pin->PinName) : Pin->GetDisplayName();
					LinkParams.Tooltip = LOCTEXT("ExecPin_ExpandTunnelExitPoint_ToolTip", "Expand tunnel exit point");
					LinkParams.NodeFlags = EScriptExecutionNodeFlags::TunnelExitPin;
					LinkParams.IconColor = FLinearColor(1.f, 1.f, 1.f, 0.8f);
					const FSlateBrush* Icon = FEditorStyle::GetBrush(TEXT("BlueprintProfiler.BPPinConnected"));
					LinkParams.Icon = const_cast<FSlateBrush*>(Icon);
					TSharedPtr<FScriptExecutionNode> ExitPoint = CreateExecutionNode(LinkParams);
					AddExitPoint(ExitPoint);
				}
			}
		}
	}
}

int32 FBlueprintFunctionContext::GetTunnelIdFromName(const FName TunnelName) const
{
	int32 TunnelId = INDEX_NONE;
	if (const int32* FoundTunnelId = TunnelIds.Find(TunnelName))
	{
		TunnelId = *FoundTunnelId;
	}
	return TunnelId;
}

void FBlueprintFunctionContext::AddEntryPoint(TSharedPtr<FScriptExecutionNode> EntryPoint)
{
	EntryPoints.Add(EntryPoint);
	const int32 TunnelId = TunnelIds.Num();
	TunnelIds.Add(EntryPoint->GetName()) = TunnelId;
}

void FBlueprintFunctionContext::AddExitPoint(TSharedPtr<FScriptExecutionNode> ExitPoint)
{
	ExitPoints.Add(ExitPoint);
	const int32 TunnelId = TunnelIds.Num();
	TunnelIds.Add(ExitPoint->GetName()) = TunnelId;
}

//////////////////////////////////////////////////////////////////////////
// FScriptEventPlayback

bool FScriptEventPlayback::Process(const TArray<FScriptInstrumentedEvent>& SignalData, const int32 StartIdx, const int32 StopIdx)
{
	struct PerNodeEventHelper
	{
		TArray<FTracePath> EntryTracePaths;
		TArray<FTracePath> InputTracePaths;
		TSharedPtr<FScriptExecutionNode> ExecNode;
		TSharedPtr<FBlueprintFunctionContext> FunctionContext;
		TArray<FScriptInstrumentedEvent> UnproccessedEvents;
	};
	struct TunnelEventHelper
	{
		double TunnelEntryTime;
		FTracePath EntryTracePath;
		TSharedPtr<FScriptExecutionNode> TunnelEntryPoint;
	};

	const int32 NumEvents = (StopIdx+1) - StartIdx;
	ProcessingState = EEventProcessingResult::Failed;
	const bool bEventIsResuming = SignalData[StartIdx].IsResumeEvent() ;

	if (BlueprintContext.IsValid() && InstanceName != NAME_None)
	{
		check(SignalData[StartIdx].IsEvent());
		EventName = bEventIsResuming ? EventName : SignalData[StartIdx].GetFunctionName();
		FName CurrentFunctionName = BlueprintContext->GetEventFunctionName(SignalData[StartIdx].GetFunctionName());
		TSharedPtr<FBlueprintFunctionContext> FunctionContext = BlueprintContext->GetFunctionContext(CurrentFunctionName);
		TSharedPtr<FScriptExecutionNode> EventNode = FunctionContext->GetProfilerDataForNode(EventName);
		// Find Associated graph nodes and submit into a node map for later processing.
		TMap<const UEdGraphNode*, PerNodeEventHelper> NodeMap;
		TArray<TunnelEventHelper> Tunnels;
		ProcessingState = EEventProcessingResult::Success;
		int32 LastEventIdx = StartIdx;
		const int32 EventStartOffset = SignalData[StartIdx].IsResumeEvent() ? 3 : 1;

		for (int32 SignalIdx = StartIdx + EventStartOffset; SignalIdx < StopIdx; ++SignalIdx)
		{
			const FScriptInstrumentedEvent& CurrSignal = SignalData[SignalIdx];
			// Update script function.
			if (CurrentFunctionName != CurrSignal.GetFunctionName())
			{
				CurrentFunctionName = CurrSignal.GetFunctionName();
				FunctionContext = BlueprintContext->GetFunctionContext(CurrentFunctionName);
				check(FunctionContext.IsValid());
			}
			if (const UEdGraphNode* GraphNode = FunctionContext->GetNodeFromCodeLocation(CurrSignal.GetScriptCodeOffset()))
			{
				PerNodeEventHelper& NodeEntry = NodeMap.FindOrAdd(GraphNode);
				if (!NodeEntry.FunctionContext.IsValid())
				{
					NodeEntry.FunctionContext = FunctionContext;
					NodeEntry.ExecNode = FunctionContext->GetProfilerDataForNode(GraphNode->GetFName());
					check(NodeEntry.ExecNode.IsValid());
				}
				switch (CurrSignal.GetType())
				{
					case EScriptInstrumentation::PureNodeEntry:
					{
						if (const UEdGraphPin* Pin = FunctionContext->GetPinFromCodeLocation(CurrSignal.GetScriptCodeOffset()))
						{
							if (FunctionContext->HasProfilerDataForNode(Pin->GetOwningNode()->GetFName()))
							{
								TracePath.AddExitPin(CurrSignal.GetScriptCodeOffset());
								NodeEntry.InputTracePaths.Insert(TracePath, 0);
							}
						}
						NodeEntry.UnproccessedEvents.Add(CurrSignal);
						break;
					}
					case EScriptInstrumentation::NodeEntry:
					{
						// Handle timings for events called as functions
						if (NodeEntry.ExecNode->IsCustomEvent())
						{
							// Ensure this is a different event
							if (const UK2Node_Event* EventGraphNode = Cast<UK2Node_Event>(GraphNode))
							{
								const FName NewEventName = EventGraphNode->GetFunctionName();
								if (NewEventName != EventName)
								{
									TracePath.Reset();
									EventTimings.Add(EventName) = SignalData[SignalIdx].GetTime() - SignalData[LastEventIdx].GetTime();
									EventName = NewEventName;
									EventNode = FunctionContext->GetProfilerDataForNode(EventName);
									LastEventIdx = SignalIdx;
								}
							}
						}
						NodeEntry.EntryTracePaths.Push(TracePath);
						NodeEntry.UnproccessedEvents.Add(CurrSignal);
						break;
					}
					case EScriptInstrumentation::PushState:
					{
						TraceStack.Push(TracePath);
						NodeEntry.UnproccessedEvents.Add(CurrSignal);
						break;
					}
					case EScriptInstrumentation::RestoreState:
					{
						check(TraceStack.Num());
						TracePath = TraceStack.Last();
						break;
					}
					case EScriptInstrumentation::SuspendState:
					{
						ProcessingState = EEventProcessingResult::Suspended;
						break;
					}
					case EScriptInstrumentation::PopState:
					{
						if (TraceStack.Num())
						{
							TracePath = TraceStack.Pop();
						}
						// Consolidate enclosed timings for execution sequence nodes
						if (GraphNode->IsA<UK2Node_ExecutionSequence>())
						{
							FScriptInstrumentedEvent OverrideEvent(CurrSignal);
							OverrideEvent.OverrideType(EScriptInstrumentation::NodeExit);
							NodeEntry.UnproccessedEvents.Add(OverrideEvent);
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
						}
						break;
					}
					case EScriptInstrumentation::NodeExit:
					{
						// Cleanup branching multiple exits and correct the tracepath
						if (NodeEntry.UnproccessedEvents.Num() && NodeEntry.UnproccessedEvents.Last().GetType() == EScriptInstrumentation::NodeExit)
						{
							NodeEntry.UnproccessedEvents.Pop();
							if (NodeEntry.EntryTracePaths.Num())
							{
								TracePath = NodeEntry.EntryTracePaths.Last();
							}
						}
						const int32 ScriptCodeExit = CurrSignal.GetScriptCodeOffset();
						// Process tunnel timings
						for (auto LinkIter : NodeEntry.ExecNode->GetLinkedNodes())
						{
							if (LinkIter.Key == ScriptCodeExit && LinkIter.Value->HasFlags(EScriptExecutionNodeFlags::FunctionTunnel))
							{
								TunnelEventHelper NewTunnel;
								NewTunnel.EntryTracePath = TracePath;
								NewTunnel.EntryTracePath.AddExitPin(ScriptCodeExit);
								NewTunnel.TunnelEntryPoint = LinkIter.Value;
								NewTunnel.TunnelEntryTime = CurrSignal.GetTime();
								Tunnels.Push(NewTunnel);
							}
							else if (LinkIter.Value->HasFlags(EScriptExecutionNodeFlags::TunnelExitPin))
							{
								if (Tunnels.Num())
								{
									TunnelEventHelper Tunnel = Tunnels.Pop();
									TSharedPtr<FScriptPerfData> PerfData = Tunnel.TunnelEntryPoint->GetPerfDataByInstanceAndTracePath(InstanceName, Tunnel.EntryTracePath);
									PerfData->AddEventTiming(CurrSignal.GetTime() - Tunnel.TunnelEntryTime);
									FTracePath ExitTrace(TracePath);
									ExitTrace.AddExitPin(LinkIter.Key);
									PerfData = LinkIter.Value->GetPerfDataByInstanceAndTracePath(InstanceName, ExitTrace);
									PerfData->AddEventTiming(0.0);
								}
							}
						}
						TracePath.AddExitPin(ScriptCodeExit);
						NodeEntry.UnproccessedEvents.Add(CurrSignal);
						// Process cyclic linkage - reset to root link tracepath
						TSharedPtr<FScriptExecutionNode> NextLink = NodeEntry.ExecNode->GetLinkedNodeByScriptOffset(ScriptCodeExit);
						if (NextLink.IsValid() && NextLink->HasFlags(EScriptExecutionNodeFlags::CyclicLinkage))
						{
							const UEdGraphNode* LinkedGraphNode = Cast<UEdGraphNode>(NextLink->GetObservedObject());
							if (PerNodeEventHelper* LinkedEntry = NodeMap.Find(LinkedGraphNode))
							{
								if (LinkedEntry->EntryTracePaths.Num())
								{
									TracePath = LinkedEntry->EntryTracePaths.Last();
								}
							}
						}
						break;
					}
					default:
					{
						NodeEntry.UnproccessedEvents.Add(CurrSignal);
						break;
					}
				}
			}
		}
		// Process last event timing
		double* TimingData = bEventIsResuming ? EventTimings.Find(EventName) : nullptr;
		if (TimingData)
		{
			*TimingData += SignalData[StopIdx].GetTime() - SignalData[LastEventIdx].GetTime();
		}
		else
		{
			EventTimings.Add(EventName) = SignalData[StopIdx].GetTime() - SignalData[LastEventIdx].GetTime();
		}
		// Process outstanding event timings, adding to previous timings if existing.
		if (ProcessingState != EEventProcessingResult::Suspended)
		{
			for (auto EventTiming : EventTimings)
			{
				FTracePath EventTracePath;
				const FName EventFunctionName = BlueprintContext->GetEventFunctionName(EventTiming.Key);
				FunctionContext = BlueprintContext->GetFunctionContext(EventFunctionName);
				check (FunctionContext.IsValid());
				EventNode = FunctionContext->GetProfilerDataForNode(EventTiming.Key);
				check (EventNode.IsValid());
				TSharedPtr<FScriptPerfData> PerfData = EventNode->GetPerfDataByInstanceAndTracePath(InstanceName, EventTracePath);
				PerfData->AddEventTiming(SignalData[StopIdx].GetTime() - SignalData[LastEventIdx].GetTime());
			}
			EventTimings.Reset();
		}
		// Process Node map timings -- this can probably be rolled back into submission during the initial processing and lose this extra iteration.
		for (auto NodeEntry : NodeMap)
		{
			TSharedPtr<FScriptExecutionNode> ExecNode = NodeEntry.Value.ExecNode;
			TSharedPtr<FScriptExecutionNode> PureNode;
			double PureNodeEntryTime = 0.0;
			double PureChainEntryTime = 0.0;
			double NodeEntryTime = 0.0;
			int32 TracePathIdx = 0;
			for (auto EventIter = NodeEntry.Value.UnproccessedEvents.CreateIterator(); EventIter; ++EventIter)
			{
				switch(EventIter->GetType())
				{
					case EScriptInstrumentation::PureNodeEntry:
					{
						if (PureChainEntryTime == 0.0)
						{
							PureChainEntryTime = EventIter->GetTime();
						}
						else if (PureNode.IsValid())
						{
							TSharedPtr<FScriptPerfData> PerfData = PureNode->GetPerfDataByInstanceAndTracePath(InstanceName, NodeEntry.Value.InputTracePaths.Pop());
							PerfData->AddEventTiming(EventIter->GetTime() - PureNodeEntryTime);
						}

						PureNode.Reset();
						PureNodeEntryTime = EventIter->GetTime();
						
						FunctionContext = BlueprintContext->GetFunctionContext(EventIter->GetFunctionName());
						check(FunctionContext.IsValid());

						const int32 ScriptCodeOffset = EventIter->GetScriptCodeOffset();
						if (const UEdGraphPin* Pin = FunctionContext->GetPinFromCodeLocation(ScriptCodeOffset))
						{
							if (FunctionContext->HasProfilerDataForNode(Pin->GetOwningNode()->GetFName()))
							{
								PureNode = NodeEntry.Value.FunctionContext->GetProfilerDataForNode(Pin->GetOwningNode()->GetFName());
							}
						}
						break;
					}
					case EScriptInstrumentation::NodeEntry:
					{
						if (PureNode.IsValid())
						{
							TSharedPtr<FScriptPerfData> PerfData = PureNode->GetPerfDataByInstanceAndTracePath(InstanceName, NodeEntry.Value.InputTracePaths.Pop());
							PerfData->AddEventTiming(EventIter->GetTime() - PureNodeEntryTime);

							PureNode.Reset();
							PureNodeEntryTime = 0.0;
						}

						if (NodeEntryTime == 0.0)
						{
							NodeEntryTime = EventIter->GetTime();
						}
						break;
					}
					case EScriptInstrumentation::NodeExit:
					{
						check(NodeEntryTime != 0.0 );
						const FTracePath NodeTracePath = NodeEntry.Value.EntryTracePaths[TracePathIdx++];
						TSharedPtr<FScriptPerfData> PerfData = ExecNode->GetPerfDataByInstanceAndTracePath(InstanceName, NodeTracePath);
						double PureChainDuration = PureChainEntryTime != 0.0 ? (NodeEntryTime - PureChainEntryTime) : 0.0;
						double NodeDuration = EventIter->GetTime() - NodeEntryTime;
						PerfData->AddEventTiming(NodeDuration);
						PureChainEntryTime = NodeEntryTime = 0.0;
						PerfData->AddPureChainTiming(PureChainDuration);
						TSharedPtr<FScriptExecutionNode> PureChainNode = ExecNode->GetPureChainNode();
						if (PureChainNode.IsValid())
						{
							PerfData = PureChainNode->GetPerfDataByInstanceAndTracePath(InstanceName, NodeTracePath);
							PerfData->AddEventTiming(PureChainDuration);
						}
						break;
					}
				}
			}
		}
	}
	return ProcessingState != EEventProcessingResult::Failed;
}

#undef LOCTEXT_NAMESPACE
