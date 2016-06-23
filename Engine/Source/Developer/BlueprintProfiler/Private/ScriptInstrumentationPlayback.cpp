// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "BlueprintProfilerPCH.h"
#include "EditorStyleSet.h"
#include "GraphEditorSettings.h"
#include "BlueprintEditorUtils.h"

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
		bIsBlueprintMapped = MapBlueprintExecution();
	}
	return bIsBlueprintMapped;
}

void FBlueprintExecutionContext::RemoveMapping()
{
	bIsBlueprintMapped = false;
	BlueprintClass.Reset();
	BlueprintNode.Reset();
	FunctionContexts.Reset();
	UbergraphFunctionName = NAME_None;
}

bool FBlueprintExecutionContext::IsEventMapped(const FName EventName) const
{
	return EventFunctionContexts.Contains(EventName);
}

void FBlueprintExecutionContext::AddEventNode(TSharedPtr<FBlueprintFunctionContext> FunctionContext, TSharedPtr<FScriptExecutionNode> EventExecNode)
{
	check(BlueprintNode.IsValid());
	BlueprintNode->AddChildNode(EventExecNode);
	EventFunctionContexts.Add(EventExecNode->GetName()) = FunctionContext;
}

void FBlueprintExecutionContext::RegisterEventContext(const FName EventName, TSharedPtr<FBlueprintFunctionContext> FunctionContext)
{
	EventFunctionContexts.Add(EventName) = FunctionContext;
}

FName FBlueprintExecutionContext::MapBlueprintInstance(const FString& InstancePath)
{
	FName InstanceName(*InstancePath);
	TWeakObjectPtr<const UObject> Instance;
	const bool bNewInstance = ResolveInstance(InstanceName, Instance);
	const bool bBlueprintMatches = Instance.IsValid() ? (Instance->GetClass() == BlueprintClass) : false;
	if (bBlueprintMatches)
	{
		if (bNewInstance)
		{
			// Create new instance node
			FScriptExecNodeParams InstanceNodeParams;
			InstanceNodeParams.NodeName = InstanceName;
			InstanceNodeParams.ObservedObject = Instance.Get();
			InstanceNodeParams.NodeFlags = EScriptExecutionNodeFlags::Instance;
			const AActor* Actor = Cast<AActor>(Instance.Get());
			InstanceNodeParams.DisplayName = Actor ? FText::FromString(Actor->GetActorLabel()) : FText::FromString(Instance.Get()->GetName());
			InstanceNodeParams.Tooltip = LOCTEXT("NavigateToInstanceHyperlink_ToolTip", "Navigate to the Instance");
			InstanceNodeParams.IconColor = FLinearColor(1.f, 1.f, 1.f, 1.f);
			InstanceNodeParams.Icon = const_cast<FSlateBrush*>(FEditorStyle::GetBrush(TEXT("BlueprintProfiler.Actor")));
			TSharedPtr<FScriptExecutionInstance> InstanceNode = MakeShareable<FScriptExecutionInstance>(new FScriptExecutionInstance(InstanceNodeParams));
			// Link to parent blueprint entry
			BlueprintNode->AddInstance(InstanceNode);
			// Fill out events from the blueprint root node
			for (int32 NodeIdx = 0; NodeIdx < BlueprintNode->GetNumChildren(); ++NodeIdx)
			{
				InstanceNode->AddChildNode(BlueprintNode->GetChildByIndex(NodeIdx));
			}
			// Broadcast change
			IBlueprintProfilerInterface& ProfilerModule = FModuleManager::LoadModuleChecked<IBlueprintProfilerInterface>("BlueprintProfiler");
			ProfilerModule.GetGraphLayoutChangedDelegate().Broadcast(Blueprint.Get());
		}
		else
		{
			TSharedPtr<FScriptExecutionInstance> InstanceNode = StaticCastSharedPtr<FScriptExecutionInstance>(BlueprintNode->GetInstanceByName(InstanceName));
			if (InstanceNode.IsValid() && PIEActorInstances.Contains(InstanceName))
			{
				InstanceNode->SetPIEInstance(PIEActorInstances.Find(InstanceName)->Get());
			}
		}
	}
	return InstanceName;
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

bool FBlueprintExecutionContext::ResolveInstance(FName& InstanceNameInOut, TWeakObjectPtr<const UObject>& ObjectInOut)
{
	bool bNewInstance = false;
	FName CorrectedName = InstanceNameInOut;
	if (const FName* SearchName = PIEInstanceNameMap.Find(InstanceNameInOut)) 
	{
		CorrectedName = *SearchName;
		InstanceNameInOut = CorrectedName;
	}
	if (TWeakObjectPtr<const UObject>* SearchResult = EditorActorInstances.Find(CorrectedName))
	{
		ObjectInOut = *SearchResult;
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
								if (EditorObject->GetClass() == BlueprintClass)
								{
									CorrectedName = FName(*EditorObject->GetPathName());
									EditorActorInstances.Add(CorrectedName) = EditorObject;
									PIEActorInstances.Add(CorrectedName) = ObjectPtr;
									PIEInstanceNameMap.Add(InstanceNameInOut) = CorrectedName;
									InstanceNameInOut = CorrectedName;
									ObjectInOut = EditorObject;
								}
								break;
							}
						}
					}
				}
				else if (!ObjectPtr->HasAnyFlags(RF_Transient) && ObjectPtr->GetClass() == BlueprintClass)
				{
					EditorActorInstances.Add(InstanceNameInOut) = ObjectPtr;
					PIEInstanceNameMap.Add(InstanceNameInOut) = InstanceNameInOut;
					ObjectInOut = ObjectPtr;
					bNewInstance = true;
				}
			}
		}
	}
	return bNewInstance;
}

FName FBlueprintExecutionContext::GetEventFunctionName(const FName EventName) const
{
	return (EventName == UEdGraphSchema_K2::FN_UserConstructionScript) ? UEdGraphSchema_K2::FN_UserConstructionScript : UbergraphFunctionName;
}

TSharedPtr<FBlueprintFunctionContext> FBlueprintExecutionContext::GetFunctionContextForEvent(const FName EventName) const
{
	TSharedPtr<FBlueprintFunctionContext> Result;
	if (const TSharedPtr<FBlueprintFunctionContext>* SearchResult = EventFunctionContexts.Find(EventName))
	{
		Result = *SearchResult;
	}
	return Result;
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

TSharedPtr<FBlueprintFunctionContext> FBlueprintExecutionContext::CreateFunctionContext(const FName FunctionName, UEdGraph* Graph)
{
	TSharedPtr<FBlueprintFunctionContext>& Result = FunctionContexts.FindOrAdd(FunctionName);
	if (!Result.IsValid())
	{
		Result = MakeShareable(new FBlueprintFunctionContext);
		Result->InitialiseContextFromGraph(AsShared(), FunctionName, Graph);
	}
	check(Result.IsValid());
	return Result;
}

bool FBlueprintExecutionContext::HasProfilerDataForPin(const UEdGraphPin* GraphPin) const
{
	SCOPE_CYCLE_COUNTER(STAT_NodeLookupCost);
	const UEdGraph* OuterGraph = GraphPin ? GraphPin->GetOwningNode()->GetTypedOuter<UEdGraph>() : nullptr;
	if (OuterGraph)
	{
		TSharedPtr<FBlueprintFunctionContext> FunctionContext = GetFunctionContext(OuterGraph->GetFName());
		if (FunctionContext.IsValid())
		{
			const FName PinName(FunctionContext->GetUniquePinName(GraphPin));
			return FunctionContext->HasProfilerDataForNode(PinName);
		}
	}
	return false;
}

TSharedPtr<FScriptExecutionNode> FBlueprintExecutionContext::GetProfilerDataForPin(const UEdGraphPin* GraphPin)
{
	SCOPE_CYCLE_COUNTER(STAT_NodeLookupCost);
	TSharedPtr<FScriptExecutionNode> Result;
	const UEdGraph* OuterGraph = GraphPin ? GraphPin->GetOwningNode()->GetTypedOuter<UEdGraph>() : nullptr;
	if (OuterGraph)
	{
		TSharedPtr<FBlueprintFunctionContext> FunctionContext = GetFunctionContext(OuterGraph->GetFName());
		if (FunctionContext.IsValid())
		{
			const FName PinName(FunctionContext->GetUniquePinName(GraphPin));
			const FName UniquePinName(*FString::Printf(TEXT("%s_%s"), *GraphPin->GetOwningNode()->GetFName().ToString(), *PinName.ToString()));
			if (FunctionContext->HasProfilerDataForNode(PinName))
			{
				Result = FunctionContext->GetProfilerDataForNode(PinName);
			}
			else
			{
				const UEdGraphNode* OwningNode = GraphPin->GetOwningNode();
				if (FunctionContext->HasProfilerDataForNode(OwningNode->GetFName()))
				{
					Result = FunctionContext->GetProfilerDataForNode(OwningNode->GetFName());
				}
			}
		}
	}
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
		// Grab all ancestral blueprints used to generate this class.
		TArray<UBlueprint*> InheritedBlueprints;
		BlueprintToMap->GetBlueprintHierarchyFromClass(BPGC, InheritedBlueprints);
		// Locate all blueprint graphs and event entry points
		TMap<FName, UEdGraph*> FunctionGraphs;
		for (auto CurrBlueprint : InheritedBlueprints)
		{
			if (UBlueprintGeneratedClass* CurrBPGC = Cast<UBlueprintGeneratedClass>(CurrBlueprint->GeneratedClass))
			{
				const FName CurrUbergraphName = Cast<UBlueprintGeneratedClass>(CurrBlueprint->GeneratedClass)->UberGraphFunction->GetFName();
				TArray<UEdGraph*> Graphs;
				CurrBlueprint->GetAllGraphs(Graphs);
				for (auto Graph : Graphs)
				{
					FName FunctionName = Graph->GetFName();
					FunctionName = FunctionName == UEdGraphSchema_K2::GN_EventGraph ? CurrUbergraphName : FunctionName;
					if (!FunctionGraphs.Contains(FunctionName))
					{
						if (UFunction* ScriptFunction = CurrBPGC->FindFunctionByName(FunctionName))
						{
							FunctionGraphs.Add(FunctionName) = Graph;
						}
					}
				}
			}
		}
		// Create function stubs
		for (auto GraphEntry : FunctionGraphs)
		{
			CreateFunctionContext(GraphEntry.Key, GraphEntry.Value);
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

void FBlueprintFunctionContext::InitialiseContextFromGraph(TSharedPtr<FBlueprintExecutionContext> BlueprintContextIn, const FName FunctionNameIn, UEdGraph* Graph)
{
	if (Graph)
	{
		// Initialise Blueprint references
		BlueprintContext = BlueprintContextIn;
		UBlueprint* Blueprint = Graph->GetTypedOuter<UBlueprint>();
		UBlueprintGeneratedClass* BPClass = Blueprint ? Cast<UBlueprintGeneratedClass>(Blueprint->GeneratedClass) : nullptr;
		check (Blueprint && BPClass);
		if (Blueprint && BPClass)
		{
			// Instantiate context
			OwningBlueprint = Blueprint;
			BlueprintClass = BPClass;
			const bool bInheritedClass = BlueprintContextIn->GetBlueprintClass() != BPClass;
			const bool bEventGraph = Graph->GetFName() == UEdGraphSchema_K2::GN_EventGraph;
			GraphName = Graph->GetFName();
			FunctionName = FunctionNameIn;
			Function = BPClass->FindFunctionByName(FunctionName);
			Function = Function.IsValid() ? Function : BPClass->UberGraphFunction;
			if (bEventGraph)
			{
				// Map events
				TArray<UK2Node_Event*> GraphEventNodes;
				for (auto EventGraph : Blueprint->UbergraphPages)
				{
					EventGraph->GetNodesOfClass<UK2Node_Event>(GraphEventNodes);
				}
				for (auto EventNode : GraphEventNodes)
				{
					FName EventName = EventNode->GetFunctionName();
					if (!BlueprintContext.Pin()->IsEventMapped(EventName))
					{
						FScriptExecNodeParams EventParams;
						EventParams.NodeName = EventName;
						EventParams.ObservedObject = EventNode;
						EventParams.DisplayName = EventNode->GetNodeTitle(ENodeTitleType::ListView);
						if (bInheritedClass)
						{
							EventParams.Tooltip = LOCTEXT("NavigateToInheritedEventLocationHyperlink_ToolTip", "Navigate to the Inherited Event");
							EventParams.IconColor = FLinearColor(0.91f, 0.16f, 0.16f, 0.4f);
						}
						else
						{
							EventParams.Tooltip = LOCTEXT("NavigateToEventLocationHyperlink_ToolTip", "Navigate to the Event");
							EventParams.IconColor = FLinearColor(0.91f, 0.16f, 0.16f);
						}
						EventParams.NodeFlags = EScriptExecutionNodeFlags::Event;
						const FSlateBrush* EventIcon = EventNode->ShowPaletteIconOnNode() ?	EventNode->GetIconAndTint(EventParams.IconColor).GetOptionalIcon() :
																							FEditorStyle::GetBrush(TEXT("BlueprintProfiler.BPNode"));
						EventParams.Icon = const_cast<FSlateBrush*>(EventIcon);
						TSharedPtr<FScriptExecutionNode> EventExecNode = CreateExecutionNode(EventParams);
						AddEntryPoint(EventExecNode);
						BlueprintContextIn->AddEventNode(AsShared(), EventExecNode);
					}
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
				// Create async task events
				TArray<UK2Node_BaseAsyncTask*> AsyncTaskNodes;
				Graph->GetNodesOfClass<UK2Node_BaseAsyncTask>(AsyncTaskNodes);
				if (AsyncTaskNodes.Num())
				{
					CreateAsyncTaskEvents(BlueprintContextIn, AsyncTaskNodes);
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
					if (bInheritedClass)
					{
						FunctionNodeParams.Tooltip = LOCTEXT("NavigateToInheritedFunctionLocationHyperlink_ToolTip", "Navigate to the Inherited Function");
						FunctionNodeParams.IconColor = FLinearColor(0.91f, 0.16f, 0.16f, 0.4f);
					}
					else
					{
						FunctionNodeParams.Tooltip = LOCTEXT("NavigateToFunctionLocationHyperlink_ToolTip", "Navigate to the Function");
						FunctionNodeParams.IconColor = FLinearColor(0.91f, 0.16f, 0.16f);
					}
					FunctionNodeParams.NodeFlags = EScriptExecutionNodeFlags::Event;
					const FSlateBrush* Icon = FunctionEntry->ShowPaletteIconOnNode() ? FunctionEntry->GetIconAndTint(FunctionNodeParams.IconColor).GetOptionalIcon() :
																						FEditorStyle::GetBrush(TEXT("BlueprintProfiler.BPNode"));
					FunctionNodeParams.Icon = const_cast<FSlateBrush*>(Icon);
					TSharedPtr<FScriptExecutionNode> FunctionEntryNode = CreateExecutionNode(FunctionNodeParams);
					AddEntryPoint(FunctionEntryNode);
					if (bConstructionScript)
					{
						BlueprintContextIn->AddEventNode(AsShared(), FunctionEntryNode);
					}
				}
			}
		}
	}
}

void FBlueprintFunctionContext::MapFunction()
{
	// Map any macro instances
	if (UBlueprint* BP = OwningBlueprint.Get())
	{
		TArray<UEdGraph*> Graphs;
		BP->GetAllGraphs(Graphs);
		for (auto Graph : Graphs)
		{
			if (Graph->GetFName() == GraphName)
			{
				TArray<UK2Node_Tunnel*> GraphTunnels;
				Graph->GetNodesOfClass<UK2Node_Tunnel>(GraphTunnels);
				// Map sub graphs / composites and macros
				for (auto Tunnel : GraphTunnels)
				{
					MapTunnelInstance(Tunnel);
				}
				break;
			}
		}
	}
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
	if (ExecNode->HasFlags(EScriptExecutionNodeFlags::PureStats))
	{
		return false;
	}
	if (Filter.Contains(ExecNode))
	{
		return true;
	}
	else
	{
		if (ExecNode->IsCyclicNode())
		{
			Filter.Add(ExecNode);
		}
		for (auto Child : ExecNode->GetChildNodes())
		{
			DetectCyclicLinks(Child, Filter);
		}
		for (auto LinkedNode : ExecNode->GetLinkedNodes())
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
			TSet<TSharedPtr<FScriptExecutionNode>> BranchFilter(Filter);
			if (DetectCyclicLinks(LinkedExecNode, BranchFilter))
			{
				// Break links and flag cycle linkage.
				FScriptExecNodeParams CycleLinkParams;
				const FString NodeName = FString::Printf(TEXT("CyclicLinkTo_%i_%s"), ExecutionNodes.Num(), *LinkedExecNode->GetName().ToString());
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
	return false;
}

void FBlueprintFunctionContext::CreateInputEvents(TSharedPtr<FBlueprintExecutionContext> BlueprintContextIn, const TArray<UK2Node*>& InputEventNodes)
{
	TMap<const UEdGraphNode*, TArray<FName>> NodeEventDescs;
	// Extract basic event info
	if (UBlueprintGeneratedClass* BPClass = BlueprintClass.Get())
	{
		for (auto EventNode : InputEventNodes)
		{
			if (EventNode)
			{
				TArray<FName>& Events = NodeEventDescs.FindOrAdd(EventNode);
				if (UK2Node_InputAction* InputActionNode = Cast<UK2Node_InputAction>(EventNode))
				{
					const FString CustomEventName = FString::Printf(TEXT("InpActEvt_%s_%s"), *InputActionNode->InputActionName.ToString());
					for (auto FunctionIter = TFieldIterator<UFunction>(BPClass); FunctionIter; ++FunctionIter)
					{
						if (FunctionIter->GetName().Contains(CustomEventName))
						{
							Events.Add(FunctionIter->GetFName());
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
							Events.Add(FunctionIter->GetFName());
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
						const FString CustomEventName = FString::Printf(TEXT("InpTchEvt_%s"), *GetPinName(Pin).ToString());
						for (auto FunctionIter = TFieldIterator<UFunction>(BPClass); FunctionIter; ++FunctionIter)
						{
							if (FunctionIter->GetName().Contains(CustomEventName))
							{
								Events.Add(FunctionIter->GetFName());
							}
						}
					}
				}
			}
		}
	}
	// Build event contexts
	for (auto NodeEvents : NodeEventDescs)
	{
		// Setup the basic exec node params.
		FScriptExecNodeParams EventParams;
		EventParams.ObservedObject = NodeEvents.Key;
		EventParams.OwningGraphName = NAME_None;
		EventParams.DisplayName = NodeEvents.Key->GetNodeTitle(ENodeTitleType::ListView);
		EventParams.Tooltip = LOCTEXT("NavigateToEventLocationHyperlink_ToolTip", "Navigate to the Event");
		EventParams.NodeFlags = EScriptExecutionNodeFlags::Event;
		EventParams.IconColor = FLinearColor(0.91f, 0.16f, 0.16f);
		const FSlateBrush* EventIcon = NodeEvents.Key->ShowPaletteIconOnNode() ? NodeEvents.Key->GetIconAndTint(EventParams.IconColor).GetOptionalIcon() :
																				 FEditorStyle::GetBrush(TEXT("BlueprintProfiler.BPNode"));
		EventParams.Icon = const_cast<FSlateBrush*>(EventIcon);
		TSharedPtr<FScriptExecutionNode> EventExecNode;
		// Create events for this node
		for (FName EventName : NodeEvents.Value)
		{
			if (!EventExecNode.IsValid())
			{
				// Create the event node.
				EventParams.NodeName = EventName;
				EventExecNode = CreateExecutionNode(EventParams);
				// Add entry points.
				AddEntryPoint(EventExecNode);
				BlueprintContextIn->AddEventNode(AsShared(), EventExecNode);
			}
			else
			{
				// Register the exec node under the event name.
				ExecutionNodes.Add(EventName) = EventExecNode;
				// Register the function context as a handler for the event.
				BlueprintContextIn->RegisterEventContext(EventName, AsShared());
			}
		}
	}
}

void FBlueprintFunctionContext::CreateAsyncTaskEvents(TSharedPtr<FBlueprintExecutionContext> BlueprintContextIn, const TArray<UK2Node_BaseAsyncTask*>& AsyncTaskNodes)
{
	struct FAsyncDelegateDesc
	{
		UEdGraphNode* DelegateNode;
		UEdGraphPin* DelegatePin;
		UFunction* EventFunction;
	};
	TArray<FAsyncDelegateDesc> AsyncDelegates;
	// Extract basic event info
	if (UBlueprintGeneratedClass* BPClass = BlueprintClass.Get())
	{
		FAsyncDelegateDesc NewDelegateDesc;
		for (auto AsyncTaskNode : AsyncTaskNodes)
		{
			if (AsyncTaskNode)
			{
				for (auto Pin : AsyncTaskNode->Pins)
				{
					if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
					{
						FName PinName(GetPinName(Pin));
						const FString PinDelegateName = FString::Printf(TEXT("%s_"), *PinName.ToString());
						if (PinName != NAME_None)
						{
							for (auto FunctionIter = TFieldIterator<UFunction>(BPClass, EFieldIteratorFlags::ExcludeSuper); FunctionIter; ++FunctionIter)
							{
								if (FunctionIter->HasAllFunctionFlags(FUNC_BlueprintEvent|FUNC_BlueprintCallable) && FunctionIter->GetName().Contains(PinDelegateName))
								{
									NewDelegateDesc.DelegateNode = AsyncTaskNode;
									NewDelegateDesc.DelegatePin = Pin;
									NewDelegateDesc.EventFunction = *FunctionIter;
									AsyncDelegates.Add(NewDelegateDesc);
									break;
								}
							}
						}
					}
				}
			}
		}
	}
	// Build event contexts
	for (auto AsyncDelegateDesc : AsyncDelegates)
	{
		UEdGraphPin* DelegatePin = AsyncDelegateDesc.DelegatePin;
		FName EventName = AsyncDelegateDesc.EventFunction->GetFName();
		FScriptExecNodeParams EventParams;
		EventParams.NodeName = EventName;
		EventParams.ObservedObject = DelegatePin->GetOwningNode();
		EventParams.ObservedPin = DelegatePin;
		EventParams.OwningGraphName = NAME_None;
		EventParams.DisplayName = DelegatePin->GetDisplayName();
		EventParams.Tooltip = LOCTEXT("NavigateToEventLocationHyperlink_ToolTip", "Navigate to the Event");
		EventParams.NodeFlags = EScriptExecutionNodeFlags::Event|EScriptExecutionNodeFlags::AsyncTaskDelegate;
		EventParams.IconColor = FLinearColor(1.f, 1.f, 1.f, 0.8f);
		const FSlateBrush* Icon = DelegatePin->LinkedTo.Num() ?	FEditorStyle::GetBrush(TEXT("BlueprintProfiler.BPPinConnected")) : 
																FEditorStyle::GetBrush(TEXT("BlueprintProfiler.BPPinDisconnected"));
		EventParams.Icon = const_cast<FSlateBrush*>(Icon);
		TSharedPtr<FScriptExecutionNode> EventExecNode = CreateExecutionNode(EventParams);
		const FName PinName = GetUniquePinName(DelegatePin);
		ExecutionNodes.Add(PinName) = EventExecNode;
		AddEntryPoint(EventExecNode);
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
			const FSlateBrush* NodeIcon = NodeToMap->ShowPaletteIconOnNode() ? NodeToMap->GetIconAndTint(NodeParams.IconColor).GetOptionalIcon() :
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
			if (const UK2Node_BaseAsyncTask* AsyncNode = Cast<UK2Node_BaseAsyncTask>(NodeToMap))
			{
				MappedNode->AddFlags(ExecPins.Num() ? (EScriptExecutionNodeFlags::AsyncTask|EScriptExecutionNodeFlags::SequentialBranch) : EScriptExecutionNodeFlags::AsyncTask);
			}
			else if (ExecPins.Num() > 1)
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
			// Pass through non-relevant (e.g. reroute) nodes.
			UK2Node* OwningNode = FBlueprintEditorUtils::FindFirstCompilerRelevantNode(LinkedPin);

			// Note: Intermediate pure nodes can have output pins that masquerade as impure node output pins when links are "moved" from the source graph (thus
			// resulting in a false association here with one or more script code offsets), so we must first ensure that the link is really to a pure node output.
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
						PureChainParams.IconColor = FLinearColor(0.2f, 1.f, 0.2f);
						const FSlateBrush* Icon = FEditorStyle::GetBrush(TEXT("BlueprintProfiler.PureNode"));
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
	const bool bAsyncTask = ExecNode->HasFlags(EScriptExecutionNodeFlags::AsyncTask);
	const bool bBranchedExecution = ExecNode->IsBranch();
	int32 NumUnwiredPins = 0;
	const UEdGraphPin* FinalExecPin = nullptr;
	for (auto Pin : Pins)
	{
		const FName PinName = GetUniquePinName(Pin);
		int32 PinScriptCodeOffset = GetCodeLocationFromPin(Pin);
		const bool bInvalidTrace = PinScriptCodeOffset == INDEX_NONE;
		PinScriptCodeOffset = bInvalidTrace ? NumUnwiredPins++ : PinScriptCodeOffset;
		TSharedPtr<FScriptExecutionNode> PinExecNode;
		// Note: Pass through non-relevant (e.g. reroute) nodes here as they're not compiled and thus do not need to be mapped for profiling.
		const UEdGraphPin* LinkedPin = Pin->LinkedTo.Num() > 0 ? FBlueprintEditorUtils::FindFirstCompilerRelevantLinkedPin(Pin->LinkedTo[0]) : nullptr;
		UEdGraphNode* LinkedPinNode = LinkedPin ? LinkedPin->GetOwningNode() : nullptr;
		const bool bTunnelBoundry = LinkedPinNode ? LinkedPinNode->IsA<UK2Node_Tunnel>() : false;
		// Create any neccesary dummy pins for branches
		if (bBranchedExecution)
		{
			if (bAsyncTask)
			{
				if (TSharedPtr<FScriptExecutionNode>* DelegateNode = ExecutionNodes.Find(PinName))
				{
					PinExecNode = *DelegateNode;
					PinExecNode->AddFlags(EScriptExecutionNodeFlags::ExecPin);
				}
			}
			if (!PinExecNode.IsValid())
			{
				FScriptExecNodeParams LinkNodeParams;
				LinkNodeParams.NodeName = PinName;
				LinkNodeParams.ObservedObject = Pin->GetOwningNode();
				LinkNodeParams.DisplayName = Pin->GetDisplayName();
				LinkNodeParams.ObservedPin = Pin;
				LinkNodeParams.Tooltip = LOCTEXT("ExecPin_ExpandExecutionPath_ToolTip", "Expand execution path");
				LinkNodeParams.NodeFlags = !bInvalidTrace ? EScriptExecutionNodeFlags::ExecPin : (EScriptExecutionNodeFlags::ExecPin|EScriptExecutionNodeFlags::InvalidTrace);
				LinkNodeParams.IconColor = FLinearColor(1.f, 1.f, 1.f, 0.8f);
				const FSlateBrush* Icon = LinkedPin ?	FEditorStyle::GetBrush(TEXT("BlueprintProfiler.BPPinConnected")) : 
														FEditorStyle::GetBrush(TEXT("BlueprintProfiler.BPPinDisconnected"));
				LinkNodeParams.Icon = const_cast<FSlateBrush*>(Icon);
				PinExecNode = CreateExecutionNode(LinkNodeParams);
			}
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
			if (bAsyncTask)
			{
				if (TSharedPtr<FScriptExecutionNode>* DelegateNode = ExecutionNodes.Find(PinName))
				{
					PinExecNode = *DelegateNode;
				}
			}
			if (!PinExecNode.IsValid())
			{
				PinExecNode = ExecNode;
			}
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
				const FName TunnelExitName(GetPinName(LinkedPin));
				TSharedPtr<FScriptExecutionNode> TunnelExitNode = GetProfilerDataForNode(TunnelExitName);
				// Check for re-entrant exit site
				if (ExecNode->HasFlags(EScriptExecutionNodeFlags::SequentialBranch))
				{
					// Locate the last linked exec pin
					if (!FinalExecPin)
					{
						for (auto SeqPin : Pins)
						{
							if (SeqPin->LinkedTo.Num())
							{
								FinalExecPin = SeqPin;
							}
						}
					}
					// Last pin in sequence is not re-entrant, but all others are assumed to be.
					TunnelExitNode->AddFlags(Pin == FinalExecPin ? EScriptExecutionNodeFlags::TunnelFinalExitPin : EScriptExecutionNodeFlags::ReEntrantTunnelPin);
				}
				if (bBranchedExecution)
				{
					PinExecNode->AddChildNode(TunnelExitNode);
				}
				else
				{
					const int32 TunnelId = GetTunnelIdFromName(TunnelExitName);
					check (TunnelId != INDEX_NONE);
					PinExecNode->AddFlags(EScriptExecutionNodeFlags::InvalidTrace);
					PinExecNode->AddLinkedNode(TunnelId, TunnelExitNode);
				}
				TArray<TSharedPtr<FScriptExecutionNode>>& TunnelExitPoints = TunnelExitPointMap.FindOrAdd(StagingTunnelName);
				TunnelExitPoints.Add(TunnelExitNode);
			}
			else
			{
				const FName TunnelName(GetPinName(LinkedPin));
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

FName FBlueprintFunctionContext::GetPinName(const UEdGraphPin* Pin) const
{
	FName PinName(NAME_None);
	if (Pin)
	{
		FString PinString = Pin->PinName;
		if (PinString.IsEmpty())
		{
			int32 PinTypeIndex = INDEX_NONE;
			for (auto NodePin : Pin->GetOwningNode()->Pins)
			{
				if (NodePin->Direction == Pin->Direction && NodePin->PinType.PinCategory == Pin->PinType.PinCategory)
				{
					PinTypeIndex++;
				}
			}
			PinString = PinTypeIndex > 0 ? FString::Printf(TEXT("%s%i"), *Pin->PinType.PinCategory, PinTypeIndex) : Pin->PinType.PinCategory;
		}
		PinName = FName(*PinString);
	}
	check (PinName != NAME_None);
	return PinName;
}

FName FBlueprintFunctionContext::GetUniquePinName(const UEdGraphPin* Pin) const
{
	FName PinName = GetPinName(Pin);
	if (Pin)
	{
		PinName = FName(*FString::Printf(TEXT("%s_%s"), *Pin->GetOwningNode()->GetFName().ToString(), *PinName.ToString()));
	}
	check (PinName != NAME_None);
	return PinName;
}

TSharedPtr<FScriptExecutionNode> FBlueprintFunctionContext::MapTunnelEntry(const FName TunnelName, const UEdGraphPin* ExecPin, const UEdGraphPin* TunnelPin, const int32 PinScriptOffset)
{
	// Retrieve tunnel function context
	const UK2Node_Tunnel* TunnelNode = Cast<UK2Node_Tunnel>(TunnelPin->GetOwningNode());
	TSharedPtr<FBlueprintFunctionContext> TunnelContext = GetTunnelContextFromNode(TunnelNode);
	const int32 TunnelId = TunnelContext->GetTunnelIdFromName(TunnelName);
	check(TunnelId != INDEX_NONE);
	// Create a tunnel instance node for the tunnel/macro to centralise stats within and aid stat lookup - for external lookup purposes by node name.
	TSharedPtr<FScriptExecutionTunnelInstance> TunnelInstance = GetTypedProfilerDataForNode<FScriptExecutionTunnelInstance>(TunnelNode->GetFName());
	if (!TunnelInstance.IsValid())
	{
		FScriptExecNodeParams TunnelInstanceParams;
		TunnelInstanceParams.NodeName = TunnelNode->GetFName();
		TunnelInstanceParams.ObservedObject = TunnelNode;
		TunnelInstanceParams.DisplayName = TunnelNode->GetNodeTitle(ENodeTitleType::ListView);
		TunnelInstanceParams.Tooltip = LOCTEXT("NavigateToNodeLocationHyperlink_ToolTip", "Navigate to the Node");
		TunnelInstanceParams.NodeFlags = EScriptExecutionNodeFlags::FunctionTunnel;
		TunnelInstanceParams.IconColor = FLinearColor(1.f, 1.f, 1.f, 0.8f);
		const FSlateBrush* TunnelIcon = TunnelNode->ShowPaletteIconOnNode() ? TunnelNode->GetIconAndTint(TunnelInstanceParams.IconColor).GetOptionalIcon() :
																			  FEditorStyle::GetBrush(TEXT("BlueprintProfiler.BPNode"));
		TunnelInstanceParams.Icon = const_cast<FSlateBrush*>(TunnelIcon);
		TunnelInstance = CreateTypedExecutionNode<FScriptExecutionTunnelInstance>(TunnelInstanceParams);
	}
	// Create a customised tunnel entry nodes,
	// This maps each unique entry point per tunnel instance so we can only include the entry and exit sites in use.
	FScriptExecNodeParams TunnelParams;
	TunnelParams.NodeName = FName(*FString::Printf(TEXT("%s_EntryPointId%i"), *TunnelNode->GetFName().ToString(), TunnelId));
	TunnelParams.ObservedObject = TunnelNode;
	TunnelParams.DisplayName = TunnelNode->GetNodeTitle(ENodeTitleType::ListView);
	TunnelParams.Tooltip = LOCTEXT("NavigateToNodeLocationHyperlink_ToolTip", "Navigate to the Node");
	TunnelParams.NodeFlags = EScriptExecutionNodeFlags::FunctionTunnel;
	TunnelParams.IconColor = FLinearColor(1.f, 1.f, 1.f, 0.8f);
	const FSlateBrush* TunnelIcon = TunnelNode->ShowPaletteIconOnNode() ? TunnelNode->GetIconAndTint(TunnelParams.IconColor).GetOptionalIcon() :
																		  FEditorStyle::GetBrush(TEXT("BlueprintProfiler.BPNode"));
	TunnelParams.Icon = const_cast<FSlateBrush*>(TunnelIcon);
	FScriptExecutionTunnelEntry::ETunnelType TunnelType = TunnelContext->GetTunnelType(TunnelName);
	TSharedPtr<FScriptExecutionTunnelEntry> CustomTunnelEntryNode = MakeShareable(new FScriptExecutionTunnelEntry(TunnelParams, TunnelNode, TunnelType));
	ExecutionNodes.FindOrAdd(TunnelParams.NodeName) = CustomTunnelEntryNode;
	// Add the custom entry point to the tunnel instance ( for stat updates )
	TunnelInstance->AddCustomEntryPoint(CustomTunnelEntryNode);
	// Add the active entry point ( pin node ) to the custom tunnel entry node
	TSharedPtr<FScriptExecutionNode> EntryPinNode = TunnelContext->GetProfilerDataForNode(TunnelName);
	check (EntryPinNode.IsValid());
	CustomTunnelEntryNode->AddChildNode(EntryPinNode);
	if (!EntryPinNode->GetNumChildren())
	{
		// Map the linked sites to the entry point on first encounter. ( this should probably be done during macro mapping )
		TSharedPtr<FScriptExecutionNode> FirstTunnelNode = TunnelContext->GetTunnelEntryPoint(TunnelName);
		check(FirstTunnelNode.IsValid());
		EntryPinNode->AddChildNode(FirstTunnelNode);
	}
	// Map tunnel exit points
	TArray<TSharedPtr<FScriptExecutionNode>> TunnelExitPoints;
	TunnelContext->GetTunnelExitPoints(TunnelName, TunnelExitPoints);
	// Build a pin name to exit pin map ( located on the tunnel node rather than in the tunnel graph )
	TMap<FName, const UEdGraphPin*> NameToPinMap;
	for (auto ExitPin : TunnelNode->Pins)
	{
		if (ExitPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec && ExitPin->LinkedTo.Num())
		{
			NameToPinMap.Add(GetPinName(ExitPin)) = ExitPin;
		}
	}
	// Register tunnel input pin node using the script offset from the first script offset emitted pin.
	// This is to try and improve tunnel entry detection for stats.
	if (const UEdGraphPin** InputPin = NameToPinMap.Find(TunnelName))
	{
		if ((*InputPin)->LinkedTo.Num())
		{
			if (const UEdGraphPin* InputLinkedPin = (*InputPin)->LinkedTo[0])
			{
				const int32 EntryScriptCodeOffset = GetCodeLocationFromPin(InputLinkedPin);
				ScriptOffsetToPins.Add(EntryScriptCodeOffset) = *InputPin;
				TunnelEntrySites.Add(EntryScriptCodeOffset) = CustomTunnelEntryNode;
			}
		}
	}
	// Map the tunnel exit sites to the exit sites for this instance and determine script offsets.
	for (auto CurrExitPoint : TunnelExitPoints)
	{
		if (const UEdGraphPin** ExitPin = NameToPinMap.Find(CurrExitPoint->GetName()))
		{
			for (auto LinkedExitPin : (*ExitPin)->LinkedTo)
			{
				// This is the unique script offset for this instance's exit site.
				const int32 ExitScriptCodeOffset = GetCodeLocationFromPin(*ExitPin);
				check (ExitScriptCodeOffset != INDEX_NONE);
				// Add entry site for exit script offset - not sure what this is used for now - if anything.
				TunnelExitSites.Add(ExitScriptCodeOffset) = CustomTunnelEntryNode;
				// Pass through any non-relevant (e.g. reroute) nodes.
				LinkedExitPin = FBlueprintEditorUtils::FindFirstCompilerRelevantLinkedPin(LinkedExitPin);
				// Continue mapping exit sites
				UEdGraphNode* LinkedExitNode = LinkedExitPin->GetOwningNode();
				TSharedPtr<FScriptExecutionNode> LinkedExecNode;
				if (LinkedExitNode->IsA<UK2Node_Tunnel>())
				{
					// Map being wired into another tunnel.
					const FName LinkedTunnelName(GetPinName(LinkedExitPin));
					LinkedExecNode = MapTunnelEntry(LinkedTunnelName, *ExitPin, LinkedExitPin, ExitScriptCodeOffset);
				}
				else
				{
					// Map forward normally.
					LinkedExecNode = MapNodeExecution(LinkedExitPin->GetOwningNode());
				}
				if (LinkedExecNode.IsValid())
				{
					if (CurrExitPoint->HasFlags(EScriptExecutionNodeFlags::ReEntrantTunnelPin))
					{
						// Add the linked nodes into the CurExitPoint
						CurrExitPoint->AddLinkedNode(ExitScriptCodeOffset, LinkedExecNode);
					}
					else
					{
						// Add the linked nodes into the CurExitPoint and as a linked node to the custom tunnel entry.
						CurrExitPoint->AddFlags(EScriptExecutionNodeFlags::TunnelFinalExitPin);
						LinkedExecNode->AddFlags(EScriptExecutionNodeFlags::TunnelFinalExitPin);
						CustomTunnelEntryNode->AddLinkedNode(ExitScriptCodeOffset, LinkedExecNode);
						CurrExitPoint->AddLinkedNode(ExitScriptCodeOffset, LinkedExecNode);
					}
					// Notify the custom tunnel entry node that this is a valid exit site for this instance.
					CustomTunnelEntryNode->AddExitSite(CurrExitPoint, ExitScriptCodeOffset);
				}
			}
		}
	}
	// Return the tunnel entry.
	return CustomTunnelEntryNode;
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

template<typename ExecNodeType> TSharedPtr<ExecNodeType> FBlueprintFunctionContext::GetTypedProfilerDataForNode(const FName NodeName)
{
	TSharedPtr<ExecNodeType> Result;
	if (TSharedPtr<FScriptExecutionNode>* SearchResult = ExecutionNodes.Find(NodeName))
	{
		Result = StaticCastSharedPtr<ExecNodeType>(*SearchResult);
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
	FEdGraphPinReference& Result = ScriptOffsetToPins.FindOrAdd(ScriptOffset);
	const UEdGraphPin* Return = Result.Get();
	if (!Return && BlueprintClass.IsValid())
	{
		if (const UEdGraphPin* GraphPin = BlueprintClass->GetDebugData().FindSourcePinFromCodeLocation(Function.Get(), ScriptOffset))
		{
			Return = GraphPin;
		}
	}
	return Return;
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

template<typename ExecNodeType> TSharedPtr<ExecNodeType> FBlueprintFunctionContext::CreateTypedExecutionNode(FScriptExecNodeParams& InitParams)
{
	TSharedPtr<FScriptExecutionNode>& ScriptExecNode = ExecutionNodes.FindOrAdd(InitParams.NodeName);
	check(!ScriptExecNode.IsValid());
	UEdGraph* OuterGraph = InitParams.ObservedObject.IsValid() ? InitParams.ObservedObject.Get()->GetTypedOuter<UEdGraph>() : nullptr;
	InitParams.OwningGraphName = OuterGraph ? OuterGraph->GetFName() : NAME_None;
	TSharedPtr<ExecNodeType> NewTypedNode = MakeShareable(new ExecNodeType(InitParams));
	ScriptExecNode = NewTypedNode;
	return NewTypedNode;
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
			// Drill down for any nested macros
			TArray<UK2Node_Tunnel*> GraphTunnels;
			TunnelGraph->GetNodesOfClass<UK2Node_Tunnel>(GraphTunnels);
			// Map sub graphs / composites and macros
			for (auto ChildTunnel : GraphTunnels)
			{
				UEdGraph* ChildTunnelGraph = nullptr;
				if (UK2Node_MacroInstance* MacroInstance = Cast<UK2Node_MacroInstance>(ChildTunnel))
				{
					ChildTunnelGraph = MacroInstance->GetMacroGraph();
				}
				else if (UK2Node_Composite* CompositeInstance = Cast<UK2Node_Composite>(ChildTunnel))
				{
					ChildTunnelGraph = CompositeInstance->BoundGraph;
				}
				if (ChildTunnelGraph && ChildTunnelGraph != TunnelGraph)
				{
					MapTunnelInstance(ChildTunnel);
				}
			}
			// Create the sub graph context
			TunnelGraphContext = MakeShareable(new FBlueprintFunctionContext);
			TunnelGraphContext->GraphName = TunnelGraph->GetFName();
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
				if (const UEdGraphPin* EntryPointPin = EntryPoint->GetObservedPin())
				{
					TunnelGraphContext->StagingTunnelName = EntryPoint->GetName();
					for (auto LinkedPin : EntryPointPin->LinkedTo)
					{
						// Pass through any non-relevant (e.g. reroute) nodes.
						LinkedPin = FBlueprintEditorUtils::FindFirstCompilerRelevantLinkedPin(LinkedPin);

						const UEdGraphNode* LinkedNode = LinkedPin->GetOwningNode();
						if (LinkedNode->IsA<UK2Node_Tunnel>())
						{
							// This handles input linked directly to output
							TSharedPtr<FScriptExecutionNode> OutputPinNode = TunnelGraphContext->GetProfilerDataForNode(GetPinName(LinkedPin));
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
		// Setup tunnel graph context from instance.
		TunnelGraphContext->BlueprintContext = BlueprintContext;
		TunnelGraphContext->Function = Function;
		TunnelGraphContext->BlueprintClass = BlueprintClass;
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

const FScriptExecutionTunnelEntry::ETunnelType FBlueprintFunctionContext::GetTunnelType(const FName TunnelName) const
{
	FScriptExecutionTunnelEntry::ETunnelType TunnelTypeResult = TunnelEntryPointMap.Num() > 1 ? FScriptExecutionTunnelEntry::SinglePathTunnel : FScriptExecutionTunnelEntry::SimpleTunnel;
	if (const TArray<TSharedPtr<FScriptExecutionNode>>* ExitSites = TunnelExitPointMap.Find(TunnelName))
	{
		TunnelTypeResult = ExitSites->Num() > 1 ? FScriptExecutionTunnelEntry::MultiIOTunnel : TunnelTypeResult;
	}
	return TunnelTypeResult;
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
					LinkParams.NodeName = GetPinName(Pin);
					LinkParams.ObservedObject = Pin->GetOwningNode();
					LinkParams.ObservedPin = Pin;
					LinkParams.DisplayName = Pin->GetDisplayName().IsEmpty() ? FText::FromName(LinkParams.NodeName) : Pin->GetDisplayName();
					LinkParams.Tooltip = LOCTEXT("ExecPin_ExpandTunnelEntryPoint_ToolTip", "Expand tunnel entry point");
					LinkParams.NodeFlags = EScriptExecutionNodeFlags::TunnelEntryPin;
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
					LinkParams.NodeName = GetPinName(Pin);
					LinkParams.ObservedObject = Pin->GetOwningNode();
					LinkParams.ObservedPin = Pin;
					LinkParams.DisplayName = Pin->GetDisplayName().IsEmpty() ? FText::FromName(LinkParams.NodeName) : Pin->GetDisplayName();
					LinkParams.Tooltip = LOCTEXT("ExecPin_ExpandTunnelExitPoint_ToolTip", "Expand tunnel exit point");
					LinkParams.NodeFlags = EScriptExecutionNodeFlags::TunnelExitPin;
					LinkParams.IconColor = FLinearColor(1.f, 1.f, 1.f, 0.8f);
					const FSlateBrush* Icon = FEditorStyle::GetBrush(TEXT("BlueprintProfiler.BPPinConnected"));
					LinkParams.Icon = const_cast<FSlateBrush*>(Icon);
					TSharedPtr<FScriptExecutionNode> ExitPoint = CreateTypedExecutionNode<FScriptExecutionTunnelExit>(LinkParams);
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

TSharedPtr<FScriptExecutionTunnelEntry> FBlueprintFunctionContext::GetTunnelEntrySite(const int32 ScriptCodeOffset)
{
	TSharedPtr<FScriptExecutionTunnelEntry> Result;
	if (TSharedPtr<FScriptExecutionTunnelEntry>* SearchResult = TunnelEntrySites.Find(ScriptCodeOffset))
	{
		Result = *SearchResult;
	}
	return Result;
}

void FBlueprintFunctionContext::GetTunnelsFromExitSite(const int32 ScriptCodeOffset, TArray<TSharedPtr<FScriptExecutionTunnelEntry>>& ResultsOut) const
{
	TunnelExitSites.MultiFind(ScriptCodeOffset, ResultsOut);
}

//////////////////////////////////////////////////////////////////////////
// FScriptEventPlayback

bool FScriptEventPlayback::Process(const TArray<FScriptInstrumentedEvent>& SignalData, const int32 StartIdx, const int32 StopIdx)
{
	const int32 NumEvents = (StopIdx+1) - StartIdx;
	ProcessingState = EEventProcessingResult::Failed;
	const bool bEventIsResuming = SignalData[StartIdx].IsResumeEvent() ;
	IBlueprintProfilerInterface* Profiler = FModuleManager::GetModulePtr<IBlueprintProfilerInterface>("BlueprintProfiler");

	if (BlueprintContext.IsValid() && InstanceName != NAME_None)
	{
		check(SignalData[StartIdx].IsEvent());
		EventName = bEventIsResuming ? EventName : SignalData[StartIdx].GetFunctionName();
		CurrentFunctionName = BlueprintContext->GetEventFunctionName(SignalData[StartIdx].GetFunctionName());
		TSharedPtr<FBlueprintFunctionContext> FunctionContext = BlueprintContext->GetFunctionContext(CurrentFunctionName);
		TSharedPtr<FScriptExecutionNode> EventNode = FunctionContext->GetProfilerDataForNode(EventName);
		// Find Associated graph nodes and submit into a node map for later processing.
		TMap<const UEdGraphNode*, NodeSignalHelper> NodeMap;
		ProcessingState = EEventProcessingResult::Success;
		int32 LastEventIdx = StartIdx;
		const int32 EventStartOffset = SignalData[StartIdx].IsResumeEvent() ? 3 : 1;

		for (int32 SignalIdx = StartIdx + EventStartOffset; SignalIdx < StopIdx; ++SignalIdx)
		{
			// Handle midstream context switches.
			const FScriptInstrumentedEvent& CurrSignal = SignalData[SignalIdx];
			if (CurrSignal.GetType() == EScriptInstrumentation::Class)
			{
				// Update the current mapped blueprint context.
				BlueprintContext = Profiler->GetBlueprintContext(CurrSignal.GetObjectPath());

				// Skip to the next signal.
				continue;
			}
			else if (CurrSignal.GetType() == EScriptInstrumentation::Instance)
			{
				// Update the current mapped instance name.
				InstanceName = BlueprintContext->MapBlueprintInstance(CurrSignal.GetObjectPath());

				// Skip to the next signal.
				continue;
			}

			// Update script function.
			if (CurrentFunctionName != CurrSignal.GetFunctionName())
			{
				CurrentFunctionName = CurrSignal.GetFunctionName();
				FunctionContext = BlueprintContext->GetFunctionContext(CurrentFunctionName);
				check(FunctionContext.IsValid());
			}
			if (const UEdGraphNode* GraphNode = FunctionContext->GetNodeFromCodeLocation(CurrSignal.GetScriptCodeOffset()))
			{
				NodeSignalHelper& CurrentNodeData = NodeMap.FindOrAdd(GraphNode);
				// Initialise the current node context.
				if (!CurrentNodeData.FunctionContext.IsValid())
				{
					CurrentNodeData.BlueprintContext = BlueprintContext;
					CurrentNodeData.FunctionContext = FunctionContext;
					CurrentNodeData.ImpureNode = FunctionContext->GetProfilerDataForNode(GraphNode->GetFName());
					check(CurrentNodeData.ImpureNode.IsValid());
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
								CurrentNodeData.InputTracePaths.Insert(TracePath, 0);
							}
						}
						CurrentNodeData.ExclusiveEvents.Add(CurrSignal);
						break;
					}
					case EScriptInstrumentation::NodeEntry:
					case EScriptInstrumentation::NodeDebugSite:
					{
						// Handle timings for events called as functions
						if (CurrentNodeData.ImpureNode->IsCustomEvent())
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
						// Process tunnel timings
						ProcessTunnelEntryPoints(FunctionContext, CurrSignal);
						CurrentNodeData.ExclusiveTracePaths.Push(TracePath);
						CurrentNodeData.ExclusiveEvents.Add(CurrSignal);
						AddToTraceHistory(CurrentNodeData.ImpureNode->GetName(), CurrSignal);
						break;
					}
					case EScriptInstrumentation::PushState:
					{
						TraceStack.Push(TracePath);
						// Process execution sequence inclusive timing
						if (GraphNode->IsA<UK2Node_ExecutionSequence>())
						{
							ProcessExecutionSequence(CurrentNodeData, CurrSignal);
						}
						break;
					}
					case EScriptInstrumentation::PopState:
					{
						if (TraceStack.Num())
						{
							TracePath = TraceStack.Pop();
							// Process execution sequence inclusive timing
							if (GraphNode->IsA<UK2Node_ExecutionSequence>())
							{
								ProcessExecutionSequence(CurrentNodeData, CurrSignal);
							}
						}
						break;
					}
					case EScriptInstrumentation::RestoreState:
					{
						check(TraceStack.Num());
						TracePath = TraceStack.Last();
						// Process execution sequence inclusive timing
						if (GraphNode->IsA<UK2Node_ExecutionSequence>())
						{
							ProcessExecutionSequence(CurrentNodeData, CurrSignal);
						}
						break;
					}
					case EScriptInstrumentation::SuspendState:
					{
						ProcessingState = EEventProcessingResult::Suspended;
						break;
					}
					case EScriptInstrumentation::NodeExit:
					{
						// Cleanup branching multiple exits and correct the tracepath
						if (CurrentNodeData.ExclusiveEvents.Num() && CurrentNodeData.ExclusiveEvents.Last().GetType() == EScriptInstrumentation::NodeExit)
						{
							CurrentNodeData.ExclusiveEvents.Pop();
							if (CurrentNodeData.ExclusiveTracePaths.Num())
							{
								TracePath = CurrentNodeData.ExclusiveTracePaths.Last();
							}
						}
						// Process tunnel timings
						ProcessTunnelEntryPoints(FunctionContext, CurrSignal);
						// Add Trace History
						AddToTraceHistory(CurrentNodeData.ImpureNode->GetName(), CurrSignal);
						// Process node exit
						const int32 ScriptCodeExit = CurrSignal.GetScriptCodeOffset();
						if (const UEdGraphPin* ValidPin = FunctionContext->GetPinFromCodeLocation(ScriptCodeExit))
						{
							TracePath.AddExitPin(ScriptCodeExit);
						}
						CurrentNodeData.ExclusiveEvents.Add(CurrSignal);
						// Process cyclic linkage - reset to root link tracepath
						TSharedPtr<FScriptExecutionNode> NextLink = CurrentNodeData.ImpureNode->GetLinkedNodeByScriptOffset(ScriptCodeExit);
						if (NextLink.IsValid() && NextLink->HasFlags(EScriptExecutionNodeFlags::CyclicLinkage))
						{
							const UEdGraphNode* LinkedGraphNode = Cast<UEdGraphNode>(NextLink->GetObservedObject());
							if (NodeSignalHelper* LinkedEntry = NodeMap.Find(LinkedGraphNode))
							{
								if (LinkedEntry->ExclusiveTracePaths.Num())
								{
									TracePath = LinkedEntry->ExclusiveTracePaths.Last();
								}
							}
						}
						break;
					}
					default:
					{
						CurrentNodeData.ExclusiveEvents.Add(CurrSignal);
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
				// The UCS, along with BP events declared as 'const' in native C++ code, are implemented as standalone functions, and not within the ubergraph function context.
				FunctionContext = BlueprintContext->GetFunctionContext(EventTiming.Key);
				if (!FunctionContext.IsValid())
				{
					FunctionContext = BlueprintContext->GetFunctionContextForEvent(EventTiming.Key);
				}
				check (FunctionContext.IsValid());
				EventNode = FunctionContext->GetProfilerDataForNode(EventTiming.Key);
				check (EventNode.IsValid());
				TSharedPtr<FScriptPerfData> PerfData = EventNode->GetOrAddPerfDataByInstanceAndTracePath(InstanceName, EventTracePath);
				PerfData->AddEventTiming(SignalData[StopIdx].GetTime() - SignalData[LastEventIdx].GetTime());
			}
			EventTimings.Reset();
		}
		// Process Node map timings -- this can probably be rolled back into submission during the initial processing and lose this extra iteration.
		for (auto CurrentNodeData : NodeMap)
		{
			TSharedPtr<FScriptExecutionNode> ExecNode = CurrentNodeData.Value.ImpureNode;
			TSharedPtr<FScriptExecutionNode> PureNode;
			double PureNodeEntryTime = 0.0;
			double PureChainEntryTime = 0.0;
			double NodeEntryTime = 0.0;
			int32 ExclTracePathIdx = 0;
			int32 InclTracePathIdx = 0;
			// Process exclusive events for this node
			for (auto EventIter = CurrentNodeData.Value.ExclusiveEvents.CreateIterator(); EventIter; ++EventIter)
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
							TSharedPtr<FScriptPerfData> PerfData = PureNode->GetOrAddPerfDataByInstanceAndTracePath(InstanceName, CurrentNodeData.Value.InputTracePaths.Pop());
							PerfData->AddEventTiming(EventIter->GetTime() - PureNodeEntryTime);
						}
						PureNode.Reset();
						PureNodeEntryTime = EventIter->GetTime();
						
						FunctionContext = CurrentNodeData.Value.BlueprintContext->GetFunctionContext(EventIter->GetFunctionName());
						check(FunctionContext.IsValid());

						const int32 ScriptCodeOffset = EventIter->GetScriptCodeOffset();
						if (const UEdGraphPin* Pin = FunctionContext->GetPinFromCodeLocation(ScriptCodeOffset))
						{
							if (FunctionContext->HasProfilerDataForNode(Pin->GetOwningNode()->GetFName()))
							{
								PureNode = CurrentNodeData.Value.FunctionContext->GetProfilerDataForNode(Pin->GetOwningNode()->GetFName());
							}
						}
						break;
					}
					case EScriptInstrumentation::NodeEntry:
					case EScriptInstrumentation::NodeDebugSite:
					{
						if (PureNode.IsValid())
						{
							TSharedPtr<FScriptPerfData> PerfData = PureNode->GetOrAddPerfDataByInstanceAndTracePath(InstanceName, CurrentNodeData.Value.InputTracePaths.Pop());
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
						const FTracePath NodeTracePath = CurrentNodeData.Value.ExclusiveTracePaths[ExclTracePathIdx++];
						TSharedPtr<FScriptPerfData> PerfData = ExecNode->GetOrAddPerfDataByInstanceAndTracePath(InstanceName, NodeTracePath);
						double PureChainDuration = PureChainEntryTime != 0.0 ? (NodeEntryTime - PureChainEntryTime) : 0.0;
						double NodeDuration = EventIter->GetTime() - NodeEntryTime;
						PerfData->AddEventTiming(NodeDuration);
						PerfData->AddInclusiveTiming(PureChainDuration, false, false);
						TSharedPtr<FScriptExecutionNode> PureChainNode = ExecNode->GetPureChainNode();
						if (PureChainNode.IsValid())
						{
							PerfData = PureChainNode->GetOrAddPerfDataByInstanceAndTracePath(InstanceName, NodeTracePath);
							PerfData->AddEventTiming(PureChainDuration);
						}
						PureChainEntryTime = NodeEntryTime = 0.0;
						break;
					}
				}
			}
			// Process inclusive events for this node
			if (CurrentNodeData.Value.InclusiveEvents.Num() > 0)
			{
				int32 EventDepth = 0;
				TArray<FScriptInstrumentedEvent> ProcessQueue;
				// Discard any nested timings, it's nasty that we have to do this extra processing.
				for (auto EventIter : CurrentNodeData.Value.InclusiveEvents)
				{
					EventDepth = EventIter.IsNodeExit() ? (EventDepth-1) : EventDepth;
					check (EventDepth>=0);
					if (EventDepth == 0)
					{
						ProcessQueue.Add(EventIter);
					}
					EventDepth = EventIter.IsNodeEntry() ? (EventDepth+1) : EventDepth;
				}
				for (auto EventIter : ProcessQueue)
				{
					switch(EventIter.GetType())
					{
						case EScriptInstrumentation::NodeEntry:
						case EScriptInstrumentation::NodeDebugSite:
						{
							if (NodeEntryTime == 0.0)
							{
								NodeEntryTime = EventIter.GetTime();
							}
							break;
						}
						case EScriptInstrumentation::NodeExit:
						{
							check(NodeEntryTime != 0.0 );
							const FTracePath NodeTracePath = CurrentNodeData.Value.InclusiveTracePaths[InclTracePathIdx++];
							TSharedPtr<FScriptPerfData> PerfData = ExecNode->GetOrAddPerfDataByInstanceAndTracePath(InstanceName, NodeTracePath);
							double NodeDuration = EventIter.GetTime() - NodeEntryTime;
							PerfData->AddInclusiveTiming(NodeDuration, false, true);
							break;
						}
					}
				}
			}
		}
	}
	return ProcessingState != EEventProcessingResult::Failed;
}

void FScriptEventPlayback::ProcessExecutionSequence(NodeSignalHelper& CurrentNodeData, const FScriptInstrumentedEvent& CurrSignal)
{
	switch(CurrSignal.GetType())
	{
		// For a sequence node a restore state represents the end of execution of a sequence pin's path (excluding the last pin)
		case EScriptInstrumentation::RestoreState:
		{
			// Convert the restore state into a node exit signal
			FScriptInstrumentedEvent OverrideEvent(CurrSignal);
			OverrideEvent.OverrideType(EScriptInstrumentation::NodeEntry);
			CurrentNodeData.ExclusiveEvents.Add(OverrideEvent);
			CurrentNodeData.ExclusiveTracePaths.Add(TracePath);
			// Add Trace History
			AddToTraceHistory(CurrentNodeData.ImpureNode->GetName(), OverrideEvent);
			break;
		}
		// For a sequence node a node entry represents the start of execution.
		case EScriptInstrumentation::PushState:
		{
			// Convert the push state into a inclusive entry signal
			FScriptInstrumentedEvent OverrideEvent(CurrSignal);
			OverrideEvent.OverrideType(EScriptInstrumentation::NodeEntry);
			CurrentNodeData.InclusiveEvents.Add(OverrideEvent);
			CurrentNodeData.InclusiveTracePaths.Add(TracePath);
			break;
		}
		// For a sequence node a restore state represents the end of execution of all the sequence pins.
		case EScriptInstrumentation::PopState:
		{
			// Convert the pop state into a inclusive exit signal
			FScriptInstrumentedEvent OverrideEvent(CurrSignal);
			OverrideEvent.OverrideType(EScriptInstrumentation::NodeExit);
			CurrentNodeData.InclusiveEvents.Add(OverrideEvent);
			CurrentNodeData.InclusiveTracePaths.Add(TracePath);
			break;
		}
	}
}

void FScriptEventPlayback::ProcessTunnelEntryPoints(TSharedPtr<FBlueprintFunctionContext> FunctionContext, const FScriptInstrumentedEvent& CurrSignal)
{
	const int32 ScriptOffset = CurrSignal.GetScriptCodeOffset();
	if (const UEdGraphPin* TunnelPin = FunctionContext->GetPinFromCodeLocation(ScriptOffset))
	{
		if (const UK2Node_Tunnel* TunnelInstance = Cast<UK2Node_Tunnel>(TunnelPin->GetOwningNode()))
		{
			TArray<TSharedPtr<FScriptExecutionTunnelEntry>> Sites;
			FunctionContext->GetTunnelsFromExitSite(ScriptOffset, Sites);
			for (auto ExitSite : Sites)
			{
				if (TunnelEventHelper* OpenTunnel = ActiveTunnels.Find(ExitSite->GetName()))
				{
					const double TunnelTiming = CurrSignal.GetTime() - OpenTunnel->TunnelEntryTime;
					TSharedPtr<FScriptExecutionTunnelEntry> TunnelEntryNode = OpenTunnel->TunnelEntryPoint;
					TunnelEntryNode->UpdateTunnelExit(InstanceName, TracePath, ScriptOffset);
					if (TunnelEntryNode->IsFinalExitSite(ScriptOffset))
					{
						TunnelEntryNode->AddTunnelTiming(InstanceName, OpenTunnel->EntryTracePath, TunnelTiming);
						TracePath = OpenTunnel->EntryTracePath;
						ActiveTunnels.Remove(ExitSite->GetName());
					}
				}
			}
			// Handle tunnel entries
			TSharedPtr<FScriptExecutionTunnelEntry> EntrySite = FunctionContext->GetTunnelEntrySite(ScriptOffset);
			if (EntrySite.IsValid())
			{
				TunnelEventHelper& NewTunnel = ActiveTunnels.Add(EntrySite->GetName());
				NewTunnel.EntryTracePath = TracePath;
				NewTunnel.EntryTracePath.AddExitPin(ScriptOffset);
				NewTunnel.TunnelEntryPoint = EntrySite;
				NewTunnel.TunnelEntryTime = CurrSignal.GetTime();
			}
		}
	}
}

void FScriptEventPlayback::AddToTraceHistory(const FName NodeName, const FScriptInstrumentedEvent& TraceSignal)
{
	// Add Trace History
	FBlueprintExecutionTrace& NewTraceEvent = BlueprintContext->AddNewTraceHistoryEvent();
	NewTraceEvent.TraceType = TraceSignal.GetType();
	NewTraceEvent.TracePath = TracePath;
	NewTraceEvent.InstanceName = InstanceName;
	NewTraceEvent.FunctionName = CurrentFunctionName;
	NewTraceEvent.NodeName = NodeName;
	NewTraceEvent.Offset = TraceSignal.GetScriptCodeOffset();
	NewTraceEvent.ObservationTime = TraceSignal.GetTime();
}

#undef LOCTEXT_NAMESPACE
