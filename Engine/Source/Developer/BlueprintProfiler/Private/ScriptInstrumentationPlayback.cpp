// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "BlueprintProfilerPCH.h"
#include "EditorStyleSet.h"
#include "GraphEditorSettings.h"
#include "BlueprintEditorUtils.h"
#include "DelayAction.h"

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
		}
	}
	if (Blueprint.IsValid() && BlueprintClass.IsValid())
	{
		// Create new blueprint exec node
		FScriptExecNodeParams BlueprintParams;
		BlueprintParams.SampleFrequency = 1;
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
			InstanceNodeParams.SampleFrequency = 1;
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
			if (InstanceNode.IsValid())
			{
				if (const FWorldContext* PIEWorldContext = GEditor->GetPIEWorldContext())
				{
					PIEActorInstances.Add(InstanceName) = InstanceNode->GetActiveObject();
				}
			}
		}
	}
	return InstanceName;
}

TSharedPtr<FScriptExecutionInstance> FBlueprintExecutionContext::GetInstanceExecNode(const FName InstanceName)
{
	TSharedPtr<FScriptExecutionInstance> Result;
	if (BlueprintNode.IsValid())
	{
		FName RemappedInstanceName = RemapInstancePath(InstanceName);
		Result = StaticCastSharedPtr<FScriptExecutionInstance>(BlueprintNode->GetInstanceByName(RemappedInstanceName));
	}
	return Result;
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
			const bool bTransientActor = ObjectPtr->HasAnyFlags(RF_Transient);
			bool bRuntimeOnlyActor = true;
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
									bRuntimeOnlyActor = false;
								}
								break;
							}
						}
					}

				}
				if ((bRuntimeOnlyActor || !bTransientActor) && ObjectPtr->GetClass() == BlueprintClass)
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

TSharedPtr<FBlueprintFunctionContext> FBlueprintExecutionContext::GetFunctionContextForEventChecked(const FName ScopedEventName) const
{
	TSharedPtr<FBlueprintFunctionContext> Result;
	if (const TSharedPtr<FBlueprintFunctionContext>* SearchResult = EventFunctionContexts.Find(ScopedEventName))
	{
		Result = *SearchResult;
	}
	else
	{
		Result = GetFunctionContext(ScopedEventName);
	}
	check (Result.IsValid());
	return Result;
}

TSharedPtr<FBlueprintFunctionContext> FBlueprintExecutionContext::GetFunctionContext(const FName ScopedFunctionName) const
{
	TSharedPtr<FBlueprintFunctionContext> Result;
	if (const TSharedPtr<FBlueprintFunctionContext>* SearchResult = FunctionContexts.Find(ScopedFunctionName))
	{
		Result = *SearchResult;
	}
	return Result;
}

TSharedPtr<FBlueprintFunctionContext> FBlueprintExecutionContext::GetFunctionContextFromGraph(const UEdGraph* Graph) const
{
	TSharedPtr<FBlueprintFunctionContext> Result;
	if (Graph)
	{
		const FName ScopedFunctionName = GetScopedFunctionNameFromGraph(Graph);
		if (const TSharedPtr<FBlueprintFunctionContext>* SearchResult = FunctionContexts.Find(ScopedFunctionName))
		{
			Result = *SearchResult;
		}
	}
	return Result;
}

template<typename FunctionType> TSharedPtr<FunctionType> FBlueprintExecutionContext::CreateFunctionContext(const FName FunctionName, UEdGraph* Graph)
{
	TSharedPtr<FBlueprintFunctionContext>& Result = FunctionContexts.FindOrAdd(FunctionName);
	if (!Result.IsValid())
	{
		Result = MakeShareable(new FunctionType);
		Result->InitialiseContextFromGraph(AsShared(), FunctionName, Graph);
	}
	check(Result.IsValid());
	return StaticCastSharedPtr<FunctionType>(Result);
}

bool FBlueprintExecutionContext::HasProfilerDataForPin(const UEdGraphPin* GraphPin) const
{
	SCOPE_CYCLE_COUNTER(STAT_NodeLookupCost);
	const UEdGraph* OwningGraph = GraphPin ? FBlueprintFunctionContext::GetGraphFromNode(GraphPin->GetOwningNode()) : nullptr;
	if (OwningGraph)
	{
		TSharedPtr<FBlueprintFunctionContext> FunctionContext = GetFunctionContextFromGraph(OwningGraph);
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
	const UEdGraph* OwningGraph = GraphPin ? FBlueprintFunctionContext::GetGraphFromNode(GraphPin->GetOwningNode()) : nullptr;
	if (OwningGraph)
	{
		TSharedPtr<FBlueprintFunctionContext> FunctionContext = GetFunctionContextFromGraph(OwningGraph);
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
	// Do a simple outer lookup to get the nodes graph, the function context will handle tunnel instances.
	const UEdGraph* OwningGraph = GraphNode ? GraphNode->GetTypedOuter<UEdGraph>() : nullptr;
	if (OwningGraph)
	{
		TSharedPtr<FBlueprintFunctionContext> FunctionContext = GetFunctionContextFromGraph(OwningGraph);
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
	// Do a simple outer lookup to get the nodes graph, the function context will handle tunnel instances.
	const UEdGraph* OwningGraph = GraphNode ? GraphNode->GetTypedOuter<UEdGraph>() : nullptr;
	if (OwningGraph)
	{
		TSharedPtr<FBlueprintFunctionContext> FunctionContext = GetFunctionContextFromGraph(OwningGraph);
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
				TArray<UEdGraph*> Graphs;
				CurrBlueprint->GetAllGraphs(Graphs);
				for (auto Graph : Graphs)
				{
					const FName FunctionName = GetFunctionNameFromGraph(Graph);
					if (UFunction* ScriptFunction = CurrBPGC->FindFunctionByName(FunctionName))
					{
						const FName ScopedFunctionName = GetScopedFunctionNameFromGraph(Graph);
						if (!FunctionGraphs.Contains(ScopedFunctionName))
						{
							FunctionGraphs.Add(ScopedFunctionName) = Graph;
						}
					}
				}
			}
		}
		// Create function stubs
		TMap<UK2Node_Tunnel*, TSharedPtr<FBlueprintFunctionContext>> DiscoveredTunnels;
		for (auto GraphEntry : FunctionGraphs)
		{
			TSharedPtr<FBlueprintFunctionContext> NewFunctionContext = CreateFunctionContext<FBlueprintFunctionContext>(GraphEntry.Key, GraphEntry.Value);
			NewFunctionContext->DiscoverTunnels(GraphEntry.Value, DiscoveredTunnels);
		}
		// Create and map each tunnels instance as its own function context.
		TSet<UEdGraph*> MappedTunnelGraphs;
		for (auto TunnelInstance : DiscoveredTunnels)
		{
			if (UEdGraph* TunnelGraph = FBlueprintFunctionContext::GetGraphFromNode(TunnelInstance.Key, false))
			{
				// Create the tunnel instance context.
				const FName TunnelInstanceFunctionName = FBlueprintFunctionContext::GetTunnelInstanceFunctionName(TunnelInstance.Key);
				TSharedPtr<FBlueprintTunnelInstanceContext> NewTunnelInstanceContext = CreateFunctionContext<FBlueprintTunnelInstanceContext>(TunnelInstanceFunctionName, TunnelGraph);
				NewTunnelInstanceContext->MapTunnelContext(TunnelInstance.Value, TunnelInstance.Key);
				TunnelInstance.Value->MapTunnelInstance(TunnelInstance.Key);
			}
		}
		// Map the function context nodes
		for (auto Context : FunctionContexts)
		{
			Context.Value->MapFunction();
		}
		// Sort the events
		if (BlueprintNode.IsValid())
		{
			BlueprintNode->SortEvents();
		}
		bMappingSuccessful = true;
	}
	return bMappingSuccessful;
}

FName FBlueprintExecutionContext::GetFunctionNameFromGraph(const UEdGraph* Graph) const
{
	FName FunctionName = NAME_None;
	if (Graph)
	{
		UBlueprint* OwnerBlueprint = Graph->GetTypedOuter<UBlueprint>();
		UBlueprintGeneratedClass* BPGC = OwnerBlueprint ? Cast<UBlueprintGeneratedClass>(OwnerBlueprint->GeneratedClass) : nullptr;
		if (BPGC)
		{
			const bool bIsEventGraph = BPGC->UberGraphFunction && FBlueprintEditorUtils::IsEventGraph(Graph);
			FunctionName = bIsEventGraph ? BPGC->UberGraphFunction->GetFName() : Graph->GetFName();
		}
	}
	check (FunctionName != NAME_None);
	return FunctionName;
}

FName FBlueprintExecutionContext::GetScopedFunctionNameFromGraph(const UEdGraph* Graph) const
{
	FName ScopedFunctionName = NAME_None;
	if (Graph)
	{
		UBlueprint* OwnerBlueprint = Graph->GetTypedOuter<UBlueprint>();
		UBlueprintGeneratedClass* BPGC = OwnerBlueprint ? Cast<UBlueprintGeneratedClass>(OwnerBlueprint->GeneratedClass) : nullptr;
		if (BPGC)
		{
			const bool bIsEventGraph = BPGC->UberGraphFunction && FBlueprintEditorUtils::IsEventGraph(Graph);
			FName GraphName = bIsEventGraph ? BPGC->UberGraphFunction->GetFName() : Graph->GetFName();
			ScopedFunctionName = FName(*FString::Printf(TEXT("%s::%s"), *BPGC->GetName(), *GraphName.ToString()));
		}
	}
	check (ScopedFunctionName != NAME_None);
	return ScopedFunctionName;
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
			bIsInheritedContext = BlueprintContextIn->GetBlueprintClass() != BPClass;
			const bool bEventGraph = Graph->GetFName() == UEdGraphSchema_K2::GN_EventGraph;
			GraphName = Graph->GetFName();
			FunctionName = FunctionNameIn;
			// Find Function
			const FName UFunctionName = BlueprintContextIn->GetFunctionNameFromGraph(Graph);
			Function = BPClass->FindFunctionByName(UFunctionName);
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
					const FName EventName = GetScopedEventName(EventNode->GetFunctionName());
					FScriptExecNodeParams EventParams;
					EventParams.SampleFrequency = 1;
					EventParams.NodeName = EventName;
					EventParams.ObservedObject = EventNode;
					if (bIsInheritedContext)
					{
						EventParams.Tooltip = LOCTEXT("NavigateToInheritedEventLocationHyperlink_ToolTip", "Navigate to the Inherited Event");
						EventParams.IconColor = GetDefault<UGraphEditorSettings>()->ParentFunctionCallNodeTitleColor;
						EventParams.NodeFlags = EScriptExecutionNodeFlags::Event|EScriptExecutionNodeFlags::InheritedEvent;
						EventParams.DisplayName = FText::FromString(FString::Printf(TEXT("%s (%s)"), *EventNode->GetNodeTitle(ENodeTitleType::ListView).ToString(), *OwningBlueprint->GetName()));
					}
					else
					{
						EventParams.Tooltip = LOCTEXT("NavigateToEventLocationHyperlink_ToolTip", "Navigate to the Event");
						EventParams.IconColor = GetDefault<UGraphEditorSettings>()->EventNodeTitleColor;
						EventParams.NodeFlags = EScriptExecutionNodeFlags::Event;
						EventParams.DisplayName = EventNode->GetNodeTitle(ENodeTitleType::ListView);
					}
					GetNodeCustomizations(EventParams);
					const FSlateBrush* EventIcon = EventNode->ShowPaletteIconOnNode() ?	EventNode->GetIconAndTint(EventParams.IconColor).GetOptionalIcon() :
																						FEditorStyle::GetBrush(TEXT("BlueprintProfiler.BPNode"));
					EventParams.Icon = const_cast<FSlateBrush*>(EventIcon);
					TSharedPtr<FScriptExecutionNode> EventExecNode = CreateExecutionNode(EventParams);
					AddEntryPoint(EventExecNode);
					BlueprintContextIn->AddEventNode(AsShared(), EventExecNode);
				}
				// Create any compiler generated events
				FBlueprintDebugData& DebugData = BlueprintClass.Get()->GetDebugData();
				CreateDelegatePinEvents(BlueprintContextIn, DebugData.GetCompilerGeneratedEvents());
			}
			else
			{
				// Map execution paths for each function entry nodes
				TArray<UK2Node_FunctionEntry*> FunctionEntryNodes;
				Graph->GetNodesOfClass<UK2Node_FunctionEntry>(FunctionEntryNodes);
				// Create function node
				for (auto FunctionEntry : FunctionEntryNodes)
				{
					FScriptExecNodeParams FunctionNodeParams;
					FunctionNodeParams.SampleFrequency = 1;
					FunctionNodeParams.NodeName = FunctionName;
					FunctionNodeParams.ObservedObject = FunctionEntry;
					if (bIsInheritedContext)
					{
						FunctionNodeParams.Tooltip = LOCTEXT("NavigateToInheritedFunctionLocationHyperlink_ToolTip", "Navigate to the Inherited Function");
						FunctionNodeParams.IconColor = GetDefault<UGraphEditorSettings>()->ParentFunctionCallNodeTitleColor;
						FunctionNodeParams.NodeFlags = EScriptExecutionNodeFlags::Event|EScriptExecutionNodeFlags::InheritedEvent;
						FunctionNodeParams.DisplayName = FText::FromString(FString::Printf(TEXT("%s (%s)"), *GraphName.ToString(), *OwningBlueprint->GetName()));
					}
					else
					{
						FunctionNodeParams.Tooltip = LOCTEXT("NavigateToFunctionLocationHyperlink_ToolTip", "Navigate to the Function");
						FunctionNodeParams.IconColor = GetDefault<UGraphEditorSettings>()->EventNodeTitleColor;
						FunctionNodeParams.NodeFlags = EScriptExecutionNodeFlags::Event;
						FunctionNodeParams.DisplayName = FText::FromName(GraphName);
					}
					GetNodeCustomizations(FunctionNodeParams);
					const FSlateBrush* Icon = FunctionEntry->ShowPaletteIconOnNode() ? FunctionEntry->GetIconAndTint(FunctionNodeParams.IconColor).GetOptionalIcon() :
																						FEditorStyle::GetBrush(TEXT("BlueprintProfiler.BPNode"));
					FunctionNodeParams.Icon = const_cast<FSlateBrush*>(Icon);
					TSharedPtr<FScriptExecutionNode> FunctionEntryNode = CreateExecutionNode(FunctionNodeParams);
					AddEntryPoint(FunctionEntryNode);
					// Add user construction scripts as events
					if (GraphName == UEdGraphSchema_K2::FN_UserConstructionScript)
					{
						BlueprintContextIn->AddEventNode(AsShared(), FunctionEntryNode);
					}
				}
			}
		}
	}
}

void FBlueprintFunctionContext::DiscoverTunnels(UEdGraph* Graph, TMap<UK2Node_Tunnel*, TSharedPtr<FBlueprintFunctionContext>>& DiscoveredTunnels)
{
	if (Graph)
	{
		TArray<UK2Node_Tunnel*> GraphTunnels;
		Graph->GetNodesOfClass<UK2Node_Tunnel>(GraphTunnels);
		// Map sub graphs / composites and macros
		for (auto Tunnel : GraphTunnels)
		{
			if (UK2Node_MacroInstance* MacroInstance = Cast<UK2Node_MacroInstance>(Tunnel))
			{
				DiscoverTunnels(MacroInstance->GetMacroGraph(), DiscoveredTunnels);
				DiscoveredTunnels.Add(Tunnel, AsShared());
			}
			else if (UK2Node_Composite* CompositeInstance = Cast<UK2Node_Composite>(Tunnel))
			{
				DiscoverTunnels(CompositeInstance->BoundGraph, DiscoveredTunnels);
				DiscoveredTunnels.Add(Tunnel, AsShared());
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
		else if (const UEdGraphPin* EntryPointPin = EntryPoint->GetObservedPin())
		{
			const int32 PinOffset = GetCodeLocationFromPin(EntryPointPin);
			for (auto LinkedPin : EntryPointPin->LinkedTo)
			{
				TSharedPtr<FScriptExecutionNode> ExecutionNode = MapNodeExecution(LinkedPin->GetOwningNode());
				if (ExecutionNode.IsValid())
				{
					EntryPoint->AddChildNode(ExecutionNode);
				}
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
		if (!ExecNode->CanLinkAsCyclicNode())
		{
			Filter.Add(ExecNode);
		}
		for (TSharedPtr<FScriptExecutionNode>& Child : ExecNode->GetChildNodes())
		{
			if (DetectCyclicLinks(Child, Filter))
			{
				// Replace child with cyclic link node.
				FScriptExecNodeParams CycleLinkParams;
				CycleLinkParams.SampleFrequency = 1;
				const FString NodeName = FString::Printf(TEXT("CyclicLinkTo_%i_%s"), ExecutionNodes.Num(), *Child->GetName().ToString());
				CycleLinkParams.NodeName = FName(*NodeName);
				CycleLinkParams.ObservedObject = Child->GetObservedObject();
				CycleLinkParams.DisplayName = Child->GetDisplayName();
				CycleLinkParams.Tooltip = LOCTEXT("CyclicLink_ToolTip", "Cyclic Link");
				CycleLinkParams.NodeFlags = EScriptExecutionNodeFlags::CyclicLinkage|Child->GetFlags();
				CycleLinkParams.IconColor = Child->GetIconColor();
				CycleLinkParams.IconColor.A = 0.15f;
				const FSlateBrush* LinkIcon = Child->GetIcon();
				CycleLinkParams.Icon = const_cast<FSlateBrush*>(LinkIcon);
				Child = CreateExecutionNode(CycleLinkParams);
			}
		}
		for (auto LinkedNode : ExecNode->GetLinkedNodes())
		{
			TSharedPtr<FScriptExecutionNode> LinkedExecNode = LinkedNode.Value;
			if (LinkedExecNode->HasFlags(EScriptExecutionNodeFlags::TunnelInstance))
			{
				TSharedPtr<FScriptExecutionNode> TunnelBoundary = LinkedExecNode->GetLinkedNodeByScriptOffset(LinkedNode.Key);
				if (TunnelBoundary.IsValid())
				{
					TSharedPtr<FScriptExecutionNode> TunnelExec = TunnelBoundary->GetLinkedNodeByScriptOffset(LinkedNode.Key);
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
				CycleLinkParams.SampleFrequency = 1;
				const FString NodeName = FString::Printf(TEXT("CyclicLinkTo_%i_%s"), ExecutionNodes.Num(), *LinkedExecNode->GetName().ToString());
				CycleLinkParams.NodeName = FName(*NodeName);
				CycleLinkParams.ObservedObject = LinkedExecNode->GetObservedObject();
				CycleLinkParams.DisplayName = LinkedExecNode->GetDisplayName();
				CycleLinkParams.Tooltip = LOCTEXT("CyclicLink_ToolTip", "Cyclic Link");
				CycleLinkParams.NodeFlags = EScriptExecutionNodeFlags::CyclicLinkage|EScriptExecutionNodeFlags::ExecPin|EScriptExecutionNodeFlags::InvalidTrace;
				CycleLinkParams.IconColor = LinkedExecNode->GetIconColor();
				CycleLinkParams.IconColor.A = 0.15f;
				const FSlateBrush* LinkIcon = LinkedExecNode->GetIcon();
				CycleLinkParams.Icon = const_cast<FSlateBrush*>(LinkIcon);
				TSharedPtr<FScriptExecutionNode> NewLink = CreateExecutionNode(CycleLinkParams);
				ExecNode->AddLinkedNode(LinkedNode.Key, NewLink);
			}
		}
	}
	return false;
}

void FBlueprintFunctionContext::CreateDelegatePinEvents(TSharedPtr<FBlueprintExecutionContext> BlueprintContextIn, const TMap<FName, FEdGraphPinReference>& PinEvents)
{
	struct FPinDelegateDesc
	{
		FPinDelegateDesc(const FName EventNameIn, UEdGraphPin* DelegatePinIn)
			: EventName(EventNameIn)
			, DelegatePin(DelegatePinIn)
		{
		}

		FName EventName;
		UEdGraphPin* DelegatePin;
	};
	if (PinEvents.Num())
	{
		TMap<const UEdGraphNode*, TArray<FPinDelegateDesc>> NodeEventDescs;
		TSharedPtr<FBlueprintFunctionContext> FunctionContext = AsShared();
		// Build event contexts per node
		for (auto PinEvent : PinEvents)
		{
			if (UEdGraphPin* DelegatePin = PinEvent.Value.Get())
			{
				const UEdGraphNode* OwningNode = DelegatePin->GetOwningNode();
				TArray<FPinDelegateDesc>& Events = NodeEventDescs.FindOrAdd(OwningNode);
				const FName ScopedEventName = GetScopedEventName(PinEvent.Key);
				Events.Add(FPinDelegateDesc(ScopedEventName, DelegatePin));
			}
		}
		// Generate the event exec nodes
		for (auto NodeEvents : NodeEventDescs)
		{
			// Check if this node requires an event node creating.
			bool bCreateEventNode = true;
			for (auto Pin : NodeEvents.Key->Pins)
			{
				if (Pin->Direction == EGPD_Input && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
				{
					bCreateEventNode = false;
					break;
				}
			}
			if (bCreateEventNode)
			{
				// Setup the basic exec node params.
				FScriptExecNodeParams EventParams;
				EventParams.SampleFrequency = 1;
				EventParams.NodeName = FName(*FString::Printf(TEXT("%s::DummyEvent"), *NodeEvents.Key->GetName()));
				EventParams.ObservedObject = NodeEvents.Key;
				EventParams.OwningGraphName = NAME_None;
				EventParams.DisplayName = NodeEvents.Key->GetNodeTitle(ENodeTitleType::ListView);
				EventParams.Tooltip = LOCTEXT("NavigateToEventLocationHyperlink_ToolTip", "Navigate to the Event");
				EventParams.NodeFlags = FunctionContext->IsInheritedContext() ? (EScriptExecutionNodeFlags::Event|EScriptExecutionNodeFlags::InheritedEvent) : EScriptExecutionNodeFlags::Event;
				GetNodeCustomizations(EventParams);
				const FSlateBrush* EventIcon = NodeEvents.Key->ShowPaletteIconOnNode() ? NodeEvents.Key->GetIconAndTint(EventParams.IconColor).GetOptionalIcon() :
																						 FEditorStyle::GetBrush(TEXT("BlueprintProfiler.BPNode"));
				EventParams.Icon = const_cast<FSlateBrush*>(EventIcon);
				TSharedPtr<FScriptExecutionNode> EventExecNode = CreateExecutionNode(EventParams);
				// Add entry points.
				AddEntryPoint(EventExecNode);
				BlueprintContextIn->AddEventNode(FunctionContext, EventExecNode);
			}
			// Create the events for the pins
			for (FPinDelegateDesc EventDesc : NodeEvents.Value)
			{
				FScriptExecNodeParams PinParams;
				PinParams.SampleFrequency = 1;
				PinParams.NodeName = EventDesc.EventName;
				PinParams.ObservedPin = EventDesc.DelegatePin;
				PinParams.OwningGraphName = NAME_None;
				PinParams.DisplayName = EventDesc.DelegatePin->GetDisplayName();
				PinParams.Tooltip = LOCTEXT("ExecPin_ExpandExecutionPath_ToolTip", "Expand execution path");
				PinParams.NodeFlags = EScriptExecutionNodeFlags::ExecPin|EScriptExecutionNodeFlags::EventPin;
				PinParams.IconColor = GetDefault<UGraphEditorSettings>()->DelegatePinTypeColor;
				const bool bPinLinked = EventDesc.DelegatePin->LinkedTo.Num() > 0;
				const FSlateBrush* Icon = bPinLinked ?	FEditorStyle::GetBrush(TEXT("BlueprintProfiler.BPPinConnected")) : 
														FEditorStyle::GetBrush(TEXT("BlueprintProfiler.BPPinDisconnected"));
				PinParams.Icon = const_cast<FSlateBrush*>(Icon);
				TSharedPtr<FScriptExecutionNode> PinExecNode = CreateExecutionNode(PinParams);
				AddEntryPoint(PinExecNode);
				// Register the function context as a handler for the event.
				BlueprintContextIn->RegisterEventContext(EventDesc.EventName, FunctionContext);
				// Register exec node under pin name
				const FName PinName = GetUniquePinName(EventDesc.DelegatePin);
				ExecutionNodes.Add(PinName) = PinExecNode;
			}
		}
	}
}

void FBlueprintFunctionContext::GetNodeCustomizations(FScriptExecNodeParams& ParamsInOut) const
{
	// Pick a color based on flags.
	if ((ParamsInOut.NodeFlags & (EScriptExecutionNodeFlags::InheritedEvent|EScriptExecutionNodeFlags::ParentFunctionCall)) != 0U)
	{
		// Inherited events and calls
		ParamsInOut.IconColor = GetDefault<UGraphEditorSettings>()->ParentFunctionCallNodeTitleColor;
	}
	else if ((ParamsInOut.NodeFlags & EScriptExecutionNodeFlags::Event) != 0U)
	{
		// Events and custom events
		ParamsInOut.IconColor = GetDefault<UGraphEditorSettings>()->EventNodeTitleColor;
	}
	else if ((ParamsInOut.NodeFlags & EScriptExecutionNodeFlags::FunctionCall) != 0U)
	{
		// Function calls
		ParamsInOut.IconColor = GetDefault<UGraphEditorSettings>()->FunctionCallNodeTitleColor;
	}
	else
	{
		// Set as the default node color.
		ParamsInOut.IconColor = FLinearColor(1.f, 1.f, 1.f, 0.8f);
		// Check for any final specialisations
		if ((ParamsInOut.NodeFlags & (EScriptExecutionNodeFlags::PureNode|EScriptExecutionNodeFlags::PureChain)) != 0U)
		{
			// Pure nodes
			ParamsInOut.IconColor = GetDefault<UGraphEditorSettings>()->PureFunctionCallNodeTitleColor;
		}
		else if ((ParamsInOut.NodeFlags & EScriptExecutionNodeFlags::SequentialBranch) != 0U)
		{
			// Sequential branches
			ParamsInOut.IconColor = GetDefault<UGraphEditorSettings>()->ExecSequenceNodeTitleColor;
		}
		else if ((ParamsInOut.NodeFlags & EScriptExecutionNodeFlags::ConditionalBranch) != 0U)
		{
			// Consditional branches
			ParamsInOut.IconColor = GetDefault<UGraphEditorSettings>()->ExecBranchNodeTitleColor;
		}
		else if (ParamsInOut.ObservedObject->IsA<UK2Node_Event>() || ParamsInOut.ObservedObject->IsA<UK2Node_FunctionEntry>())
		{
			// Differentiate between execution path and entry node when they have the same name
			FFormatNamedArguments Args;
			Args.Add(TEXT("NodeName"), ParamsInOut.DisplayName);
			Args.Add(TEXT("EntryNode"), LOCTEXT("EntryNode", "Entry Node"));
			ParamsInOut.DisplayName = FText::Format(FText::FromString("{NodeName} ({EntryNode})"), Args);
		}
	}
}

void FBlueprintFunctionContext::DetermineGraphNodeCharacteristics(const UEdGraphNode* GraphNode, TArray<UEdGraphPin*>& InputPins, TArray<UEdGraphPin*>& ExecPins, FScriptExecNodeParams& NodeParams)
{
	// Set the standard sample base
	NodeParams.SampleFrequency = 1;
	// Evaluate Execution and Input Pins
	int32 ConnectedExecPins = 0;
	for (auto Pin : GraphNode->Pins)
	{
		if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
		{
			ExecPins.Add(Pin);
			if (Pin->LinkedTo.Num())
			{
				ConnectedExecPins++;
			}
		}
		else if (Pin->Direction == EGPD_Input && Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
		{
			InputPins.Add(Pin);
		}
	}
	// Identify branching types and pure nodes based on pin layout.
	if (ExecPins.Num() == 0 && !GraphNode->IsA<UK2Node_FunctionResult>())
	{
		NodeParams.NodeFlags |= EScriptExecutionNodeFlags::PureNode;
	}
	else if (ExecPins.Num() > 1)
	{
		if (GraphNode->IsA<UK2Node_ExecutionSequence>())
		{
			// Update the node params with the expected base sample rate.
			NodeParams.SampleFrequency = ConnectedExecPins;
			NodeParams.NodeFlags |= EScriptExecutionNodeFlags::SequentialBranch;
		}
		else
		{
			NodeParams.NodeFlags |= EScriptExecutionNodeFlags::ConditionalBranch;
		}
	}
	// Identify function calls and custom events
	if (GraphNode->IsA<UK2Node_CallParentFunction>())
	{
		NodeParams.NodeFlags |= EScriptExecutionNodeFlags::ParentFunctionCall;
	}
	else if (GraphNode->IsA<UK2Node_CallFunction>())
	{
		NodeParams.NodeFlags |= EScriptExecutionNodeFlags::FunctionCall;
	}
	else if (GraphNode->IsA<UK2Node_CustomEvent>())
	{
		NodeParams.NodeFlags |= EScriptExecutionNodeFlags::CustomEvent;
	}
}

TSharedPtr<FScriptExecutionNode> FBlueprintFunctionContext::MapNodeExecution(UEdGraphNode* NodeToMap)
{
	TSharedPtr<FScriptExecutionNode> MappedNode;
	if (NodeToMap)
	{
		// Lookup existing mapped node
		MappedNode = GetProfilerDataForGraphNode(NodeToMap);
		// Map if not existing.
		if (!MappedNode.IsValid())
		{
			// Determine node characteristics
			FScriptExecNodeParams NodeParams;
			NodeParams.NodeFlags = EScriptExecutionNodeFlags::Node;
			TArray<UEdGraphPin*> ExecPins;
			TArray<UEdGraphPin*> InputPins;
			DetermineGraphNodeCharacteristics(NodeToMap, InputPins, ExecPins, NodeParams);
			NodeParams.NodeName = NodeToMap->GetFName();
			NodeParams.ObservedObject = NodeToMap;
			NodeParams.DisplayName = NodeToMap->GetNodeTitle(ENodeTitleType::ListView);
			NodeParams.Tooltip = LOCTEXT("NavigateToNodeLocationHyperlink_ToolTip", "Navigate to the Node");

			GetNodeCustomizations(NodeParams);
			const FSlateBrush* NodeIcon = NodeToMap->ShowPaletteIconOnNode() ? NodeToMap->GetIconAndTint(NodeParams.IconColor).GetOptionalIcon() :
																				FEditorStyle::GetBrush(TEXT("BlueprintProfiler.BPNode"));
			NodeParams.Icon = const_cast<FSlateBrush*>(NodeIcon);
			MappedNode = CreateExecutionNode(NodeParams);
			// Discover pure node script ranges
			if (!MappedNode->IsPureNode())
			{
				MappedNode->SetPureNodeScriptCodeRange(GetPureNodeScriptCodeRange(NodeToMap));
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
			// Locate any pure nodes from the root impure node.
			if (!MappedNode->IsPureNode())
			{
				TSharedPtr<FScriptExecutionNode> PureChainNode = MappedNode->GetPureChainNode();
				if (PureChainNode.IsValid())
				{
					TMap<int32, TSharedPtr<FScriptExecutionNode>> AllPureNodes;
					PureChainNode->GetAllPureNodes(AllPureNodes);
					// Patch pure node chains into function context for lookup later.
					for (auto PureNode : AllPureNodes)
					{
						if (!ExecutionNodes.Contains(PureNode.Value->GetName()))
						{
							ExecutionNodes.Add(PureNode.Value->GetName()) = PureNode.Value;
						}
					}
				}
			}
			// Evaluate Children for call sites
			if (MappedNode->IsFunctionCallSite()||MappedNode->IsParentFunctionCallSite())
			{
				if (UK2Node_CallFunction* FunctionCallSite = Cast<UK2Node_CallFunction>(NodeToMap))
				{
					const UEdGraphNode* FunctionNode = nullptr; 
					if (UEdGraph* CalledGraph = FunctionCallSite->GetFunctionGraph(FunctionNode))
					{
						// Update Exec node
						const bool bEventCall = FunctionNode ? FunctionNode->IsA<UK2Node_Event>() : false;
						MappedNode->SetToolTipText(LOCTEXT("NavigateToFunctionCallsiteHyperlink_ToolTip", "Navigate to the Function Callsite"));
						// Don't add entry points for events.
						if (!bEventCall)
						{
							// Update the function context
							TSharedPtr<FBlueprintFunctionContext> NewFunctionContext = BlueprintContext.Pin()->GetFunctionContextFromGraph(CalledGraph);
							if (NewFunctionContext.IsValid())
							{
								NewFunctionContext->AddCallSiteEntryPointsToNode(MappedNode);
								ChildFunctionContexts.Add(NewFunctionContext->GetFunctionName()) = NewFunctionContext;
							}
						}
					}
				}
			}
		}
	}
	return MappedNode;
}

TSharedPtr<FScriptExecutionNode> FBlueprintFunctionContext::FindOrCreatePureChainRoot(TSharedPtr<FScriptExecutionNode> ExecNode)
{
	TSharedPtr<FScriptExecutionNode> PureChainRootNode;
	// Add a pure chain container node as the root, if it's not already in place.
	if (ExecNode.IsValid() && !ExecNode->IsPureNode())
	{
		static const FString PureChainNodeNameSuffix = TEXT("__PROFILER_InputPureTime");
		const FName PureChainNodeName = FName(*(ExecNode->GetName().ToString() + PureChainNodeNameSuffix));
		// Try to find the root first
		for (auto ChildNode : ExecNode->GetChildNodes())
		{
			if (ChildNode->GetName() == PureChainNodeName)
			{
				PureChainRootNode = ChildNode;
			}
		}
		// Otherwise create one
		if (!PureChainRootNode.IsValid())
		{
			FScriptExecNodeParams PureChainParams;
			PureChainParams.SampleFrequency = 1;
			PureChainParams.NodeName = PureChainNodeName;
			PureChainParams.ObservedObject = ExecNode->GetObservedObject();
			PureChainParams.DisplayName = LOCTEXT("PureChain_DisplayName", "Pure Time");
			PureChainParams.Tooltip = LOCTEXT("PureChain_ToolTip", "Expand pure node timing");
			PureChainParams.NodeFlags = EScriptExecutionNodeFlags::PureChain;
			GetNodeCustomizations(PureChainParams);
			const FSlateBrush* Icon = FEditorStyle::GetBrush(TEXT("BlueprintProfiler.PureNode"));
			PureChainParams.Icon = const_cast<FSlateBrush*>(Icon);
			PureChainRootNode = CreateExecutionNode(PureChainParams);
			PureChainRootNode->SetPureNodeScriptCodeRange(ExecNode->GetPureNodeScriptCodeRange());
			ExecNode->AddChildNode(PureChainRootNode);
		}
	}
	return PureChainRootNode;
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

			// If this is a tunnel node - we need to map through the tunnel
			if (OwningNode && OwningNode->IsA<UK2Node_Tunnel>())
			{
				if (FBlueprintEditorUtils::IsTunnelInstanceNode(OwningNode))
				{
					// Find the tunnel instance context
					const FName ScopedTunnelContextName = GetTunnelInstanceFunctionName(OwningNode);
					TSharedPtr<FBlueprintTunnelInstanceContext> TunnelContext = StaticCastSharedPtr<FBlueprintTunnelInstanceContext>(BlueprintContext.Pin()->GetFunctionContext(ScopedTunnelContextName));
					check (TunnelContext.IsValid());
					TSharedPtr<FScriptExecutionNode> TunnelPureNode = TunnelContext->GetPureNodeFromTunnelPin(LinkedPin);
					// Add the linked pure nodes
					if (TunnelPureNode.IsValid() && TunnelPureNode->IsPureChain())
					{
						// Grab the pure chain root.
						PureChainRootNode = FindOrCreatePureChainRoot(ExecNode);
						if (!PureChainRootNode.IsValid())
						{
							PureChainRootNode = ExecNode;
						}
						// Copy relevant pure chain links
						FInt32Range TunnelPureRange = PureChainRootNode->GetPureNodeScriptCodeRange();
						for (auto LinkedNode : TunnelPureNode->GetLinkedNodes())
						{
							if (TunnelPureRange.Contains(LinkedNode.Key))
							{
								PureChainRootNode->AddLinkedNode(LinkedNode.Key, LinkedNode.Value);
							}
						}
					}
				}
			}

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
						PureChainRootNode = FindOrCreatePureChainRoot(ExecNode);
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
		const bool bTunnelBoundary = LinkedPinNode ? LinkedPinNode->IsA<UK2Node_Tunnel>() : false;
		// Try locate an already mapped pin.
		if (TSharedPtr<FScriptExecutionNode>* MappedPin = ExecutionNodes.Find(PinName))
		{
			PinExecNode = *MappedPin;
			ExecNode->AddLinkedNode(PinScriptCodeOffset, PinExecNode);
			continue;
		}
		// Create any neccesary dummy pins for branches
		if (bBranchedExecution)
		{
			FScriptExecNodeParams LinkNodeParams;
			LinkNodeParams.SampleFrequency = 1;
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
			ExecNode->AddLinkedNode(PinScriptCodeOffset, PinExecNode);
		}
		else if (!PinExecNode.IsValid())
		{
			PinExecNode = ExecNode;
		}
		// Continue mapping forward.
		TSharedPtr<FScriptExecutionNode> LinkedPinExecNode = bTunnelBoundary ? MapTunnelBoundary(LinkedPin) : MapNodeExecution(LinkedPinNode);
		if (LinkedPinExecNode.IsValid())
		{
			if (bBranchedExecution)
			{
				PinExecNode->AddChildNode(LinkedPinExecNode);
			}
			else
			{
				PinExecNode->AddLinkedNode(PinScriptCodeOffset, LinkedPinExecNode);
			}
		}
	}
}

TSharedPtr<FScriptExecutionNode> FBlueprintFunctionContext::GetTunnelBoundaryNodeChecked(const UEdGraphPin* TunnelPin)
{
	TSharedPtr<FScriptExecutionNode> Result;
	if (const UK2Node_Tunnel* TunnelNode = Cast<UK2Node_Tunnel>(TunnelPin->GetOwningNode()))
	{
		if (FBlueprintEditorUtils::IsTunnelInstanceNode(TunnelNode))
		{
			// Lookup external tunnel boundary.
			const FName TunnelInstanceFunctionName = GetTunnelInstanceFunctionName(TunnelNode);
			TSharedPtr<FBlueprintFunctionContext> TunnelContext = BlueprintContext.Pin()->GetFunctionContext(TunnelInstanceFunctionName);
			if (TunnelContext.IsValid())
			{
				Result = TunnelContext->GetProfilerDataForNode(GetTunnelBoundaryName(TunnelPin));
			}
		}
		else
		{
			// Internal boundary lookup, faster path.
			Result = GetProfilerDataForNode(GetPinName(TunnelPin));
		}
	}
	check (Result.IsValid());
	return Result;
}

TSharedPtr<FScriptExecutionNode> FBlueprintFunctionContext::MapTunnelBoundary(const UEdGraphPin* TunnelPin)
{
	TSharedPtr<FScriptExecutionNode> TunnelBoundaryNode;
	if (TunnelPin)
	{
		TunnelBoundaryNode = GetTunnelBoundaryNodeChecked(TunnelPin);
		if (TunnelBoundaryNode->IsTunnelEntry())
		{
			TSharedPtr<FScriptExecutionTunnelEntry> TunnelEntryInstance = StaticCastSharedPtr<FScriptExecutionTunnelEntry>(TunnelBoundaryNode);
			for (auto ExitSite : TunnelEntryInstance->GetLinkedNodes())
			{
				if (ExitSite.Value->IsTunnelExit())
				{
					TSharedPtr<FScriptExecutionTunnelExit> TunnelExit = TunnelEntryInstance->GetExitSite(ExitSite.Key);
					const UEdGraphPin* TunnelInstanceExitPin = TunnelExit->GetExternalPin();
					for (auto LinkedPin : TunnelInstanceExitPin->LinkedTo)
					{
						UEdGraphNode* LinkedNode = LinkedPin->GetOwningNode();
						TSharedPtr<FScriptExecutionNode> LinkedExecNode;
						// Need to be careful here because a tunnel instance exit site can link to a tunnel Boundary too.
						if (LinkedNode->IsA<UK2Node_Tunnel>())
						{
							LinkedExecNode = MapTunnelBoundary(LinkedPin);
						}
						else
						{
							LinkedExecNode = GetProfilerDataForNode(LinkedNode->GetFName());
						}
						if (!LinkedExecNode.IsValid())
						{
							LinkedExecNode = MapNodeExecution(LinkedNode);
						}
						TunnelExit->AddLinkedNode(ExitSite.Key, LinkedExecNode);
					}
				}
			}
		}
	}
	return TunnelBoundaryNode;
}

FName FBlueprintFunctionContext::GetPinName(const UEdGraphPin* Pin)
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
		UEdGraphNode* OwningNode = Pin->GetOwningNode();
		if (OwningNode->IsA<UK2Node_Tunnel>())
		{
			// Tunnel pins have to be unique so we need the node name in addition to pin name.
			UEdGraph* TunnelGraph = FBlueprintFunctionContext::GetGraphFromNode(OwningNode, false);
			TunnelGraph = TunnelGraph ? TunnelGraph : OwningNode->GetTypedOuter<UEdGraph>();
			FName TunnelName = TunnelGraph->GetFName();
			PinString = FString::Printf(TEXT("%s_%s"), *TunnelName.ToString(), *PinString);
		}
		PinName = FName(*PinString);
	}
	check (PinName != NAME_None);
	return PinName;
}

FName FBlueprintFunctionContext::GetUniquePinName(const UEdGraphPin* Pin)
{
	FName PinName = GetPinName(Pin);
	if (Pin)
	{
		PinName = FName(*FString::Printf(TEXT("%s_%s"), *Pin->GetOwningNode()->GetFName().ToString(), *PinName.ToString()));
	}
	check (PinName != NAME_None);
	return PinName;
}

FName FBlueprintFunctionContext::GetTunnelBoundaryName(const UEdGraphPin* Pin)
{
	FName PinName(NAME_None);
	if (Pin)
	{
		UEdGraphNode* OwningNode = Pin->GetOwningNode();
		const FString PinString = Pin->PinName.IsEmpty() ? Pin->PinType.PinCategory : Pin->PinName;
		PinName = FName(*FString::Printf(TEXT("%s_%s"), *OwningNode->GetFName().ToString(), *PinString));
	}
	check (PinName != NAME_None);
	return PinName;
}

UEdGraphPin* FBlueprintFunctionContext::FindMatchingPin(const UEdGraphNode* NodeToSearch, const UEdGraphPin* PinToFind, const bool bIgnoreDirection)
{
	UEdGraphPin* MatchingPin = nullptr;
	if (NodeToSearch && PinToFind)
	{
		// Quick simple lookup, works the bulk of the time.
		MatchingPin = NodeToSearch->FindPin(PinToFind->PinName);
		// More exhaustive search, for Boundary cases
		if (!MatchingPin)
		{
			const FName ForcedPinName = GetPinName(PinToFind);
			for (auto SearchPin : NodeToSearch->Pins)
			{
				if (PinToFind->PinName == SearchPin->PinName && PinToFind->PinType.PinCategory == SearchPin->PinType.PinCategory)
				{
					if (bIgnoreDirection || PinToFind->Direction == SearchPin->Direction)
					{
						MatchingPin = SearchPin;
						break;
					}
				}
			}
		}
	}
	return MatchingPin;
}

UEdGraph* FBlueprintFunctionContext::GetGraphFromNode(const UEdGraphNode* GraphNode, const bool bAllowNonTunnel)
{
	UEdGraph* OwningGraph = nullptr;
	if (GraphNode)
	{
		if (const UK2Node_MacroInstance* MacroInstance = Cast<UK2Node_MacroInstance>(GraphNode))
		{
			OwningGraph = MacroInstance->GetMacroGraph();
		}
		else if (const UK2Node_Composite* CompositeNode = Cast<UK2Node_Composite>(GraphNode))
		{
			OwningGraph = CompositeNode->BoundGraph;
		}
		else if (bAllowNonTunnel)
		{
			OwningGraph = GraphNode->GetTypedOuter<UEdGraph>();
		}
	}
	return OwningGraph;
}

FName FBlueprintFunctionContext::GetTunnelInstanceFunctionName(const UEdGraphNode* GraphNode)
{
	FName ScopedFunctionName = NAME_None;
	UEdGraph* OuterGraph = GraphNode ? GraphNode->GetTypedOuter<UEdGraph>() : nullptr;
	if (OuterGraph)
	{
		// To identify instances exactly, we need the owning graph name and the node name to avoid collisions.
		ScopedFunctionName = FName(*FString::Printf(TEXT("%s::%s"), *OuterGraph->GetName(), *GraphNode->GetName()));
	}
	check (ScopedFunctionName != NAME_None);
	return ScopedFunctionName;
}

FName FBlueprintFunctionContext::GetScopedFunctionNameFromNode(const UEdGraphNode* GraphNode) const
{
	FName ScopedFunctionName = NAME_None;
	if (GraphNode)
	{
		if (const UEdGraphNode* TunnelNode = GetTunnelNodeFromGraphNode(GraphNode))
		{
			ScopedFunctionName = GetTunnelInstanceFunctionName(TunnelNode);
		}
		else if (const UEdGraph* Graph = GetGraphFromNode(GraphNode))
		{
			UBlueprint* OwnerBlueprint = Graph->GetTypedOuter<UBlueprint>();
			UBlueprintGeneratedClass* BPGC = OwnerBlueprint ? Cast<UBlueprintGeneratedClass>(OwnerBlueprint->GeneratedClass) : nullptr;
			if (BPGC)
			{
				const bool bIsEventGraph = BPGC->UberGraphFunction && FBlueprintEditorUtils::IsEventGraph(Graph);
				FName NodeGraphName = bIsEventGraph ? BPGC->UberGraphFunction->GetFName() : Graph->GetFName();
				ScopedFunctionName = FName(*FString::Printf(TEXT("%s::%s"), *BPGC->GetName(), *NodeGraphName.ToString()));
			}
		}
	}
	check (ScopedFunctionName != NAME_None);
	return ScopedFunctionName;
}

void FBlueprintFunctionContext::MapTunnelExits(TSharedPtr<FScriptExecutionTunnelEntry> TunnelEntryPoint)
{
	if (TunnelEntryPoint.IsValid())
	{
		for (auto ExitSite : TunnelEntryPoint->GetLinkedNodes())
		{
			if (const UEdGraphPin* ExitPin = ExitSite.Value->GetObservedPin())
			{
				for (UEdGraphPin* LinkedPin : ExitPin->LinkedTo)
				{
					TSharedPtr<FScriptExecutionNode> LinkedExecNode = MapNodeExecution(LinkedPin->GetOwningNode());
					ExitSite.Value->AddChildNode(LinkedExecNode);
				}
			}
		}
	}
}

bool FBlueprintFunctionContext::HasProfilerDataForNode(const FName NodeName) const
{
	TSharedPtr<FScriptExecutionNode> Result = GetProfilerDataForNode(NodeName);
	return Result.IsValid();
}

bool FBlueprintFunctionContext::HasProfilerDataForGraphNode(const UEdGraphNode* Node) const
{
	TSharedPtr<FScriptExecutionNode> Result = GetProfilerDataForGraphNode(Node);
	return Result.IsValid();
}

TSharedPtr<FScriptExecutionNode> FBlueprintFunctionContext::GetProfilerDataForNode(const FName NodeName) const
{
	TSharedPtr<FScriptExecutionNode> Result;
	if (const TSharedPtr<FScriptExecutionNode>* SearchResult = ExecutionNodes.Find(NodeName))
	{
		Result = *SearchResult;
	}
	else
	{
		// Check child function contexts
		for (auto ChildFunctionContext : ChildFunctionContexts)
		{
			if (ChildFunctionContext.Value.IsValid())
			{
				Result = ChildFunctionContext.Value.Pin()->GetProfilerDataForNode(NodeName);
				if (Result.IsValid())
				{
					break;
				}
			}
		}
	}
	return Result;
}

TSharedPtr<FScriptExecutionNode> FBlueprintFunctionContext::GetProfilerDataForNodeChecked(const FName NodeName) const
{
	TSharedPtr<FScriptExecutionNode> Result = GetProfilerDataForNode(NodeName);
	check (Result.IsValid());
	return Result;
}

TSharedPtr<FScriptExecutionNode> FBlueprintFunctionContext::GetProfilerDataForGraphPin(const UEdGraphPin* Pin) const
{
	TSharedPtr<FScriptExecutionNode> Result;
	if (Pin)
	{
		// Do a local lookup, this could be a tunnel/macro node and the graph wouldn't match.
		if (const TSharedPtr<FScriptExecutionNode>* SearchResult = ExecutionNodes.Find(GetPinName(Pin)))
		{
			Result = *SearchResult;
		}
		// Check objects match
		if (Result.IsValid())
		{
			if (Pin != Result->GetObservedPin())
			{
				// If the observed object doesn't match we need to look elsewhere.
				Result.Reset();
			}
		}
		// Perform a more exhaustive search.
		if (!Result.IsValid())
		{
			const FName ObjectFunctionName = GetScopedFunctionNameFromNode(Pin->GetOwningNode());
			if (ObjectFunctionName != FunctionName)
			{
				// Lookup function context otherwise search child contexts
				if (const TWeakPtr<FBlueprintFunctionContext>* FunctionContext = ChildFunctionContexts.Find(ObjectFunctionName))
				{
					if (FunctionContext->IsValid())
					{
						Result = FunctionContext->Pin()->GetProfilerDataForGraphPin(Pin);
					}
				}
			}
		}
	}
	return Result;
}

TSharedPtr<FScriptExecutionNode> FBlueprintFunctionContext::GetProfilerDataForGraphNode(const UEdGraphNode* Node) const
{
	TSharedPtr<FScriptExecutionNode> Result;
	if (Node)
	{
		// Do a local lookup, this could be a tunnel/macro node and the graph wouldn't match.
		if (const TSharedPtr<FScriptExecutionNode>* SearchResult = ExecutionNodes.Find(Node->GetFName()))
		{
			Result = *SearchResult;
		}
		// Check objects match
		if (Result.IsValid())
		{
			if (Node != Result->GetObservedObject())
			{
				// If the observed object doesn't match we need to look elsewhere.
				Result.Reset();
			}
		}
		// Perform a more exhaustive search.
		if (!Result.IsValid())
		{
			const FName ObjectFunctionName = GetScopedFunctionNameFromNode(Node);
			if (ObjectFunctionName != FunctionName)
			{
				// Lookup function context otherwise search child contexts
				if (const TWeakPtr<FBlueprintFunctionContext>* FunctionContext = ChildFunctionContexts.Find(ObjectFunctionName))
				{
					if (FunctionContext->IsValid())
					{
						Result = FunctionContext->Pin()->GetProfilerDataForGraphNode(Node);
					}
				}
			}
		}
	}
	return Result;
}

template<typename ExecNodeType> TSharedPtr<ExecNodeType> FBlueprintFunctionContext::GetTypedProfilerDataForNode(const FName NodeName)
{
	TSharedPtr<ExecNodeType> Result;
	TSharedPtr<FScriptExecutionNode> SearchResult = GetProfilerDataForNode(NodeName);
	if (SearchResult.IsValid())
	{
		Result = StaticCastSharedPtr<ExecNodeType>(SearchResult);
	}
	return Result;
}

template<typename ExecNodeType> TSharedPtr<ExecNodeType> FBlueprintFunctionContext::GetTypedProfilerDataForGraphNode(const UEdGraphNode* Node)
{
	TSharedPtr<ExecNodeType> Result;
	TSharedPtr<FScriptExecutionNode> SearchResult = GetProfilerDataForGraphNode(Node);
	if (SearchResult.IsValid())
	{
		Result = StaticCastSharedPtr<ExecNodeType>(SearchResult);
	}
	return Result;
}

bool FBlueprintFunctionContext::GetProfilerContextFromScriptOffset(const int32 ScriptOffset, TSharedPtr<FScriptExecutionNode>& ExecNode, TSharedPtr<FBlueprintFunctionContext>& FunctionContext)
{
	if (const UEdGraphNode* GraphNode = GetNodeFromCodeLocation(ScriptOffset))
	{
		// get the fully scoped function name
		const FName FunctionContextName = GetScopedFunctionNameFromNode(GraphNode);
		if (FunctionContextName == FunctionName)
		{
			FunctionContext = AsShared();
			ExecNode = GetProfilerDataForNode(GraphNode->GetFName());
		}
		else
		{
			if (TWeakPtr<FBlueprintFunctionContext>* FunctionContextPtr = ChildFunctionContexts.Find(FunctionContextName))
			{
				if (FunctionContextPtr->IsValid())
				{
					// We now have the source tunnel graph context, use this to find the instance and then the node.
					FunctionContextPtr->Pin()->GetProfilerContextFromScriptOffset(ScriptOffset, ExecNode, FunctionContext);
				}
			}
		}
	}
	return ExecNode.IsValid() && FunctionContext.IsValid();
}

const UEdGraphNode* FBlueprintFunctionContext::GetNodeFromCodeLocation(const int32 ScriptOffset)
{
	TWeakObjectPtr<const UEdGraphNode>& Result = ScriptOffsetToNodes.FindOrAdd(ScriptOffset);
	if (!Result.IsValid() && BlueprintClass.IsValid())
	{
		// First pass lookup, if inside a tunnel this will return the tunnel instance node.
		Result = BlueprintClass->GetDebugData().FindSourceNodeFromCodeLocation(Function.Get(), ScriptOffset, true);
		if (const UEdGraphNode* PotentialTunnelNode = Result.Get())
		{
			// Check for tunnel nodes
			if (FBlueprintEditorUtils::IsTunnelInstanceNode(PotentialTunnelNode))
			{
				// Find the true source node.
				if (const UEdGraphNode* TrueGraphNode = BlueprintClass->GetDebugData().FindMacroSourceNodeFromCodeLocation(Function.Get(), ScriptOffset))
				{
					// Cache the tunnel node.
					if (TrueGraphNode != PotentialTunnelNode || FBlueprintEditorUtils::IsTunnelInstanceNode(TrueGraphNode))
					{
						NodeToTunnelNode.Add(TrueGraphNode) = PotentialTunnelNode;
						Result = TrueGraphNode;
					}
				}
			}
		}
	}
	return Result.Get();
}

const UEdGraphNode* FBlueprintFunctionContext::GetTunnelNodeFromCodeLocation(const int32 ScriptOffset)
{
	TWeakObjectPtr<const UEdGraphNode> Result;
	if (TWeakObjectPtr<const UEdGraphNode>* NodeSearch = ScriptOffsetToNodes.Find(ScriptOffset))
	{
		if (TWeakObjectPtr<const UEdGraphNode>* TunnelNodeSearch = NodeToTunnelNode.Find(*NodeSearch))
		{
			Result = *TunnelNodeSearch;
		}
	}
	return Result.Get();
}

const UEdGraphNode* FBlueprintFunctionContext::GetTunnelNodeFromGraphNode(const UEdGraphNode* GraphNode) const
{
	TWeakObjectPtr<const UEdGraphNode> Result;
	if (const TWeakObjectPtr<const UEdGraphNode>* SearchResult = NodeToTunnelNode.Find(GraphNode))
	{
		Result = *SearchResult;
	}
	return Result.Get();
}

bool FBlueprintFunctionContext::IsNodeFromTunnelGraph(const UEdGraphNode* Node) const
{
	return NodeToTunnelNode.Contains(Node);
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
	const bool bPureChain = (InitParams.NodeFlags & EScriptExecutionNodeFlags::PureChain) != 0U;
	if (bPureChain)
	{
		ScriptExecNode = MakeShareable(new FScriptExecutionPureNode(InitParams));
	}
	else
	{
		ScriptExecNode = MakeShareable(new FScriptExecutionNode(InitParams));
	}
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

void FBlueprintFunctionContext::MapTunnelInstance(UK2Node_Tunnel* TunnelInstance)
{
	// Grab tunnel context from instance name
	if (TunnelInstance)
	{
		const FName ScopedFunctionName = GetTunnelInstanceFunctionName(TunnelInstance);
		TSharedPtr<FBlueprintFunctionContext> TunnelFunctionContext = BlueprintContext.Pin()->GetFunctionContext(ScopedFunctionName);
		if (TunnelFunctionContext.IsValid())
		{
			ChildFunctionContexts.Add(ScopedFunctionName) = TunnelFunctionContext;
		}
	}
}

void FBlueprintFunctionContext::AddEntryPoint(TSharedPtr<FScriptExecutionNode> EntryPoint)
{
	EntryPoints.Add(EntryPoint);
}

void FBlueprintFunctionContext::AddExitPoint(TSharedPtr<FScriptExecutionNode> ExitPoint)
{
	ExitPoints.Add(ExitPoint);
}

FName FBlueprintFunctionContext::GetScopedEventName(const FName EventName) const
{
	return FName(*FString::Printf(TEXT("%s::%s"), *BlueprintClass.Get()->GetName(), *EventName.ToString()));
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

//////////////////////////////////////////////////////////////////////
// FBlueprintTunnelInstanceContext

void FBlueprintTunnelInstanceContext::InitialiseContextFromGraph(TSharedPtr<FBlueprintExecutionContext> BlueprintContextIn, const FName TunnelInstanceName, UEdGraph* TunnelGraph)
{
	GraphName = TunnelGraph->GetFName();
	FunctionName = TunnelInstanceName;
	BlueprintContext = BlueprintContextIn;
}

void FBlueprintTunnelInstanceContext::MapTunnelContext(TSharedPtr<FBlueprintFunctionContext> CallingFunctionContext, UK2Node_Tunnel* TunnelInstance)
{
	if (!TunnelInstanceNode.IsValid())
	{
		// Set the mapping context.
		Function = CallingFunctionContext->GetUFunction();
		BlueprintClass = CallingFunctionContext->GetBlueprintClass();
		TunnelInstanceNode = TunnelInstance;
		// Find the function context that represents the graph and not the instance
		UEdGraph* TunnelGraph = GetGraphFromNode(TunnelInstance, false);
		// Map tunnel Input/Output
		MapTunnelIO();
		// Create tunnel instance node.
		FScriptExecNodeParams TunnelInstanceParams;
		TunnelInstanceParams.SampleFrequency = 1;
		TunnelInstanceParams.NodeName = TunnelInstance->GetFName();
		TunnelInstanceParams.ObservedObject = TunnelInstance;
		TunnelInstanceParams.DisplayName = TunnelInstance->GetNodeTitle(ENodeTitleType::ListView);
		TunnelInstanceParams.Tooltip = LOCTEXT("NavigateToNodeLocationHyperlink_ToolTip", "Navigate to the Node");
		TunnelInstanceParams.NodeFlags = EScriptExecutionNodeFlags::TunnelInstance;
		TunnelInstanceParams.IconColor = FLinearColor(1.f, 1.f, 1.f, 0.8f);
		if (TunnelInstance->IsA<UK2Node_Composite>())
		{
			const FSlateBrush* TunnelIcon = FEditorStyle::GetBrush(TEXT("GraphEditor.SubGraph_16x"));
			TunnelInstanceParams.Icon = const_cast<FSlateBrush*>(TunnelIcon);
		}
		else
		{
			const FSlateBrush* TunnelIcon = TunnelInstance->ShowPaletteIconOnNode() ? TunnelInstance->GetIconAndTint(TunnelInstanceParams.IconColor).GetOptionalIcon() :
																					  FEditorStyle::GetBrush(TEXT("GraphEditor.SubGraph_16x"));
			TunnelInstanceParams.Icon = const_cast<FSlateBrush*>(TunnelIcon);
		}
		TSharedPtr<FScriptExecutionTunnelInstance> TunnelInstanceExecNode = CreateTypedExecutionNode<FScriptExecutionTunnelInstance>(TunnelInstanceParams);
		// Map child tunnel instances now, because we require that the instance exec node is setup.
		TMap<UK2Node_Tunnel*, TSharedPtr<FBlueprintFunctionContext>> ChildTunnels;
		DiscoverTunnels(TunnelGraph, ChildTunnels);
		for (auto ChildTunnel : ChildTunnels)
		{
			MapTunnelInstance(ChildTunnel.Key);
		}
		// Find tunnel instance entry sites
		TMap<FName, UEdGraphPin*> InstanceEntrySites;
		for (auto Pin : TunnelInstance->Pins)
		{
			if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
			{
				if (Pin->LinkedTo.Num())
				{
					const FName EntryPointName = GetPinName(Pin);
					if (Pin->Direction == EGPD_Input)
					{
						InstanceEntrySites.Add(EntryPointName) = Pin;
					}
				}
			}
		}
		// Map tunnel entry site to exit sites
		for (auto InstanceEntryPoint : InstanceEntrySites)
		{
			const int32 EntryPointScriptOffset = GetCodeLocationFromPin(InstanceEntryPoint.Value);
			TSharedPtr<FScriptExecutionNode> EntryPoint = GetProfilerDataForNode(InstanceEntryPoint.Key);
			if (EntryPoint.IsValid() && EntryPointScriptOffset != INDEX_NONE)
			{
				// Create custom exec node for each tunnel instance entry, this is so we can customise the appearance of the node.
				FName BoundaryName = GetTunnelBoundaryName(InstanceEntryPoint.Value);
				FScriptExecNodeParams TunnelEntryParams;
				TunnelEntryParams.SampleFrequency = 1;
				TunnelEntryParams.NodeName = BoundaryName;
				TunnelEntryParams.ObservedObject = TunnelInstance;
				TunnelEntryParams.ObservedPin = InstanceEntryPoint.Value;
				TunnelEntryParams.DisplayName = TunnelInstance->GetNodeTitle(ENodeTitleType::ListView);
				TunnelEntryParams.Tooltip = LOCTEXT("NavigateToNodeLocationHyperlink_ToolTip", "Navigate to the Tunnel Instance");
				TunnelEntryParams.NodeFlags = EScriptExecutionNodeFlags::TunnelEntryPinInstance;
				TunnelEntryParams.IconColor = FLinearColor(1.f, 1.f, 1.f, 0.8f);
				TunnelEntryParams.Icon = TunnelInstanceParams.Icon;
				TSharedPtr<FScriptExecutionTunnelEntry> InstanceTunnelEntry = CreateTypedExecutionNode<FScriptExecutionTunnelEntry>(TunnelEntryParams);
				// Add internal entrypoint as child.
				InstanceTunnelEntry->AddChildNode(EntryPoint);
				// Register the external entry point
				TunnelInstanceExecNode->AddEntrySite(EntryPointScriptOffset, InstanceTunnelEntry);
				// Set the current entry point we are mapping.
				StagingEntryPoint = InstanceTunnelEntry;
				// Map this tunnel entry
				if (const UEdGraphPin* InternalEntryPointPin = EntryPoint->GetObservedPin())
				{
					for (auto LinkedPin : InternalEntryPointPin->LinkedTo)
					{
						// Pass through any non-relevant (e.g. reroute) nodes.
						LinkedPin = FBlueprintEditorUtils::FindFirstCompilerRelevantLinkedPin(LinkedPin);
						// Grab the owning node
						UEdGraphNode* LinkedNode = LinkedPin->GetOwningNode();
						// Check for linkage to another tunnel, this could be another tunnel or an exit site for this one.
						bool bLinkedToTunnel = false;
						bool bInternalTunnel = false;
						if (const UK2Node_Tunnel* LinkedTunnelNode = Cast<UK2Node_Tunnel>(LinkedNode))
						{
							// Marked as linked to another tunnel.
							bLinkedToTunnel = true;
							// Check if the tunnel is a tunnel instance node or not.
							bInternalTunnel = !FBlueprintEditorUtils::IsTunnelInstanceNode(LinkedTunnelNode);
						}
						// Map the linked pin/node whilst being aware of directly linked tunnel boundaries.
						TSharedPtr<FScriptExecutionNode> ExecNode = bLinkedToTunnel ? MapTunnelBoundary(LinkedPin) : MapNodeExecution(LinkedNode);
						// Only add the new node if it is not an internal tunnel.
						if (!bInternalTunnel && ExecNode.IsValid())
						{
							EntryPoint->AddChildNode(ExecNode);
						}
					}
				}
				// Add exit sites to instance
				for (auto ExitSite : InstanceTunnelEntry->GetLinkedNodes())
				{
					if (ExitSite.Value->IsTunnelExit())
					{
						TunnelInstanceExecNode->AddExitSite(ExitSite.Key, StaticCastSharedPtr<FScriptExecutionTunnelExit>(ExitSite.Value));
					}
				}
				// Reset staged entry point
				StagingEntryPoint.Reset();
			}
		}
	}
}

TSharedPtr<FScriptExecutionNode> FBlueprintTunnelInstanceContext::GetPureNodeFromTunnelPin(const UEdGraphPin* TunnelPin) const
{
	TSharedPtr<FScriptExecutionNode> Result;
	if (const TSharedPtr<FScriptExecutionNode>* SearchResult = PureLinkNodes.Find(TunnelPin))
	{
		Result = *SearchResult;
	}
	return Result;
}

TSharedPtr<FScriptExecutionNode> FBlueprintTunnelInstanceContext::MapNodeExecution(UEdGraphNode* NodeToMap)
{
	TSharedPtr<FScriptExecutionNode> MappedNode;
	if (NodeToMap)
	{
		MappedNode = GetProfilerDataForGraphNode(NodeToMap);
		if (!MappedNode.IsValid())
		{
			// First encounter, map execution.
			MappedNode = FBlueprintFunctionContext::MapNodeExecution(NodeToMap);
		}
		else
		{
			// Discover already mapped exit sites
			DiscoverExitSites(MappedNode);
		}
	}
	return MappedNode;
}

void FBlueprintTunnelInstanceContext::MapInputPins(TSharedPtr<FScriptExecutionNode> ExecNode, const TArray<UEdGraphPin*>& Pins)
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

			// If this is a tunnel node - we need to map through the tunnel
			if (OwningNode && OwningNode->IsA<UK2Node_Tunnel>())
			{
				TSharedPtr<FScriptExecutionNode> TunnelPureNode;

				if (FBlueprintEditorUtils::IsTunnelInstanceNode(OwningNode))
				{
					// Find the tunnel instance context
					const FName ScopedTunnelContextName = GetTunnelInstanceFunctionName(OwningNode);
					TSharedPtr<FBlueprintTunnelInstanceContext> TunnelContext = StaticCastSharedPtr<FBlueprintTunnelInstanceContext>(BlueprintContext.Pin()->GetFunctionContext(ScopedTunnelContextName));
					check (TunnelContext.IsValid());
					TunnelPureNode = TunnelContext->GetPureNodeFromTunnelPin(LinkedPin);
				}
				else
				{
					if (const UEdGraphPin* ExternalPin = FindMatchingPin(TunnelInstanceNode.Get(), LinkedPin))
					{
						if (TSharedPtr<FScriptExecutionNode>* SearchResult = PureLinkNodes.Find(ExternalPin))
						{
							TunnelPureNode = *SearchResult;
						}
					}
				}
				// Link up the pure nodes
				if (TunnelPureNode.IsValid() && TunnelPureNode->IsPureChain())
				{
					// Grab the pure chain root.
					PureChainRootNode = FindOrCreatePureChainRoot(ExecNode);
					if (!PureChainRootNode.IsValid())
					{
						PureChainRootNode = ExecNode;
					}
					// Copy pure chain links
					for (auto LinkedPureNode : TunnelPureNode->GetLinkedNodes())
					{
						ExecNode->AddLinkedNode(LinkedPureNode.Key, LinkedPureNode.Value);
					}
				}
				continue;
			}

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
						PureChainRootNode = FindOrCreatePureChainRoot(ExecNode);
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

bool FBlueprintTunnelInstanceContext::IsTunnelNodeInternal(const UEdGraphNode* TunnelNode)
{
	if (!FBlueprintEditorUtils::IsTunnelInstanceNode(TunnelNode))
	{
		return TunnelNode->GetTypedOuter<UEdGraph>()->GetFName() == GraphName;
	}
	return false;
}

bool FBlueprintTunnelInstanceContext::IsPinFromThisTunnel(const UEdGraphPin* TunnelPin) const
{
	bool bIsFromThisTunnel = false;
	const UEdGraphNode* PinNode = TunnelPin ? TunnelPin->GetOwningNode() : nullptr;
	if (PinNode)
	{
		if (UEdGraph* PinGraph = GetGraphFromNode(PinNode))
		{
			bIsFromThisTunnel = GraphName == PinGraph->GetFName();
		}
	}
	return bIsFromThisTunnel;
}

void FBlueprintTunnelInstanceContext::DiscoverExitSites(TSharedPtr<FScriptExecutionNode> MappedNode)
{
	if (MappedNode->IsTunnelExit())
	{
		// Map tunnel exit because we are mapping inside a tunnel discovering exit pins.
		if (StagingEntryPoint.IsValid())
		{
			TSharedPtr<FScriptExecutionTunnelExit> ExitNode = StaticCastSharedPtr<FScriptExecutionTunnelExit>(MappedNode);
			if (UEdGraphPin* ExternalPin = ExitNode->GetExternalPin())
			{
				const int32 ScriptCodeOffset = GetCodeLocationFromPin(ExternalPin);
				StagingEntryPoint->AddLinkedNode(ScriptCodeOffset, ExitNode);
			}
		}
	}
	else
	{
		for (auto ChildIter : MappedNode->GetChildNodes())
		{
			DiscoverExitSites(ChildIter);
		}
		for (auto LinkIter : MappedNode->GetLinkedNodes())
		{
			DiscoverExitSites(LinkIter.Value);
		}
	}
}

TSharedPtr<FScriptExecutionNode> FBlueprintTunnelInstanceContext::MapTunnelBoundary(const UEdGraphPin* TunnelPin)
{
	TSharedPtr<FScriptExecutionNode> TunnelBoundaryNode = GetTunnelBoundaryNodeChecked(TunnelPin);
	if (TunnelBoundaryNode->IsTunnelExit())
	{
		// Map tunnel exit because we are mapping inside a tunnel discovering exit pins.
		TSharedPtr<FScriptExecutionTunnelExit> ExitNode = StaticCastSharedPtr<FScriptExecutionTunnelExit>(TunnelBoundaryNode);
		if (UEdGraphPin* ExternalPin = ExitNode->GetExternalPin())
		{
			const int32 ScriptCodeOffset = GetCodeLocationFromPin(ExternalPin);
			StagingEntryPoint->AddLinkedNode(ScriptCodeOffset, ExitNode);
		}
	}
	else if (TunnelBoundaryNode->IsTunnelEntry())
	{
		TSharedPtr<FScriptExecutionTunnelEntry> TunnelEntryInstance = StaticCastSharedPtr<FScriptExecutionTunnelEntry>(TunnelBoundaryNode);
		for (auto ExitSite : TunnelEntryInstance->GetLinkedNodes())
		{
			if (ExitSite.Value->IsTunnelExit())
			{
				TSharedPtr<FScriptExecutionTunnelExit> TunnelExit = TunnelEntryInstance->GetExitSite(ExitSite.Key);
				const UEdGraphPin* TunnelInstanceExitPin = TunnelExit->GetExternalPin();
				for (auto LinkedPin : TunnelInstanceExitPin->LinkedTo)
				{
					UEdGraphNode* LinkedNode = LinkedPin->GetOwningNode();
					TSharedPtr<FScriptExecutionNode> LinkedExecNode;
					// Need to be careful here because a tunnel instance exit site can link to a tunnel Boundary too.
					if (LinkedNode->IsA<UK2Node_Tunnel>())
					{
						LinkedExecNode = MapTunnelBoundary(LinkedPin);
					}
					else
					{
						LinkedExecNode = GetProfilerDataForNode(LinkedNode->GetFName());
					}
					if (!LinkedExecNode.IsValid())
					{
						LinkedExecNode = MapNodeExecution(LinkedNode);
					}
					TunnelExit->AddLinkedNode(ExitSite.Key, LinkedExecNode);
				}
			}
		}
	}
	return TunnelBoundaryNode;
}

void FBlueprintTunnelInstanceContext::MapTunnelIO()
{
	if (UK2Node_Tunnel* TunnelInstanceGraphNode = TunnelInstanceNode.Get())
	{
		// Map internal tunnel pins to tunnel instance pins
		TMap<UEdGraphPin*, UEdGraphPin*> EntryPins;
		TMap<UEdGraphPin*, UEdGraphPin*> ExitPins;
		TMap<UEdGraphPin*, UEdGraphPin*> PurePins;
		TArray<UK2Node_Tunnel*> GraphTunnels;
		UEdGraph* TunnelGraph = GetGraphFromNode(TunnelInstanceGraphNode, false);
		TunnelGraph->GetNodesOfClass<UK2Node_Tunnel>(GraphTunnels);
		// Build tunnel pin sets.
		for (auto Tunnel : GraphTunnels)
		{
			if (IsTunnelNodeInternal(Tunnel))
			{
				for (UEdGraphPin* InternalPin : Tunnel->Pins)
				{
					UEdGraphPin* TunnelInstancePin = FindMatchingPin(TunnelInstanceGraphNode, InternalPin);
					if (InternalPin->LinkedTo.Num())
					{
						if (InternalPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
						{
							// Map linked exec pins
							if (TunnelInstancePin)
							{
								if (InternalPin->Direction == EGPD_Output)
								{
									EntryPins.Add(InternalPin) = TunnelInstancePin;
								}
								else
								{
									ExitPins.Add(InternalPin) = TunnelInstancePin;
								}
							}
						}
						else
						{
							// Map pure pin sites
							PurePins.Add(InternalPin) = TunnelInstancePin;
						}
					}
				}
			}
		}
		// Create instance entry points
		for (auto EntryPinSet : EntryPins)
		{
			FScriptExecNodeParams LinkParams;
			LinkParams.SampleFrequency = 1;
			LinkParams.NodeName = GetPinName(EntryPinSet.Key);
			LinkParams.ObservedObject = EntryPinSet.Key->GetOwningNode();
			LinkParams.ObservedPin = EntryPinSet.Key;
			LinkParams.DisplayName = EntryPinSet.Key->GetDisplayName();
			LinkParams.Tooltip = LOCTEXT("ExecPin_ExpandTunnelEntryPoint_ToolTip", "Expand tunnel entry point");
			LinkParams.NodeFlags = EScriptExecutionNodeFlags::TunnelEntryPin|EScriptExecutionNodeFlags::InvalidTrace;
			LinkParams.IconColor = FLinearColor(1.f, 1.f, 1.f, 0.8f);
			const bool bPinLinked = EntryPinSet.Key->LinkedTo.Num() > 0;
			const FSlateBrush* Icon = bPinLinked ?	FEditorStyle::GetBrush(TEXT("BlueprintProfiler.BPPinConnected")) : 
													FEditorStyle::GetBrush(TEXT("BlueprintProfiler.BPPinDisconnected"));
			LinkParams.Icon = const_cast<FSlateBrush*>(Icon);
			TSharedPtr<FScriptExecutionNode> EntryPoint = CreateExecutionNode(LinkParams);
			AddEntryPoint(EntryPoint);
		}
		// Create instance exit points
		for (auto ExitPinSet : ExitPins)
		{
			FScriptExecNodeParams LinkParams;
			LinkParams.SampleFrequency = 1;
			LinkParams.NodeName = GetPinName(ExitPinSet.Key);
			LinkParams.ObservedObject = ExitPinSet.Key->GetOwningNode();
			LinkParams.ObservedPin = ExitPinSet.Key;
			LinkParams.DisplayName = ExitPinSet.Key->GetDisplayName();
			LinkParams.Tooltip = LOCTEXT("ExecPin_ExpandTunnelExitPoint_ToolTip", "Expand tunnel exit point");
			LinkParams.NodeFlags = EScriptExecutionNodeFlags::TunnelExitPin;
			LinkParams.IconColor = FLinearColor(1.f, 1.f, 1.f, 0.8f);
			const bool bPinLinked = ExitPinSet.Key->LinkedTo.Num() > 0;
			const FSlateBrush* Icon = bPinLinked ?	FEditorStyle::GetBrush(TEXT("BlueprintProfiler.BPPinConnected")) : 
													FEditorStyle::GetBrush(TEXT("BlueprintProfiler.BPPinDisconnected"));
			LinkParams.Icon = const_cast<FSlateBrush*>(Icon);
			TSharedPtr<FScriptExecutionTunnelExit> ExitPoint = CreateTypedExecutionNode<FScriptExecutionTunnelExit>(LinkParams);
			const FName TunnelBoundaryName = GetTunnelBoundaryName(ExitPinSet.Value);
			// Add exit point under internal pin name for mapping.
			ExitPoint->SetExternalPin(ExitPinSet.Value);
			AddExitPoint(ExitPoint);
		}
		// Map pure pin links
		for (auto PurePinSet : PurePins)
		{
			// Use the external pin if tunnel entry otherwise use the internal pin.
			const UEdGraphPin* PinToTrace = PurePinSet.Key->Direction == EGPD_Output ? PurePinSet.Value : PurePinSet.Key;
			// Create a pure node chain for the pin
			FScriptExecNodeParams PureChainParams;
			static const FString PureChainNodeNameSuffix = TEXT("__PROFILER_InputPureTime");
			FString PureChainNodeNameString = PinToTrace->GetName() + PureChainNodeNameSuffix;
			PureChainParams.SampleFrequency = 1;
			PureChainParams.NodeName = FName(*PureChainNodeNameString);
			PureChainParams.ObservedPin = PinToTrace;
			PureChainParams.DisplayName = LOCTEXT("PureChain_DisplayName", "Pure Time");
			PureChainParams.Tooltip = LOCTEXT("PureChain_ToolTip", "Expand pure node timing");
			PureChainParams.NodeFlags = EScriptExecutionNodeFlags::PureChain;
			GetNodeCustomizations(PureChainParams);
			const FSlateBrush* Icon = FEditorStyle::GetBrush(TEXT("BlueprintProfiler.PureNode"));
			PureChainParams.Icon = const_cast<FSlateBrush*>(Icon);
			TSharedPtr<FScriptExecutionNode> PureChainRootNode = CreateExecutionNode(PureChainParams);
			PureLinkNodes.Add(PurePinSet.Value) = PureChainRootNode;
			// Map pins
			for (auto LinkedPin : PinToTrace->LinkedTo)
			{
				// Pass through non-relevant (e.g. reroute) nodes.
				UK2Node* OwningNode = FBlueprintEditorUtils::FindFirstCompilerRelevantNode(LinkedPin);
				// Map the execution
				TSharedPtr<FScriptExecutionNode> PureNode = MapNodeExecution(OwningNode);
				if (PureNode.IsValid() && PureNode->IsPureNode())
				{
					TArray<int32> CodeLocations;
					GetAllCodeLocationsFromPin(LinkedPin, CodeLocations);
					for (int32 CodeLocation : CodeLocations)
					{
						PureChainRootNode->AddLinkedNode(CodeLocation, PureNode);
					}
				}
			}
		}
	}
}

bool FBlueprintTunnelInstanceContext::GetProfilerContextFromScriptOffset(const int32 ScriptOffset, TSharedPtr<FScriptExecutionNode>& ExecNode, TSharedPtr<FBlueprintFunctionContext>& FunctionContext)
{
	if (const UEdGraphNode* GraphNode = GetNodeFromCodeLocation(ScriptOffset))
	{
		FunctionContext = AsShared();
		ExecNode = GetProfilerDataForNode(GraphNode->GetFName());
	}
	return ExecNode.IsValid() && FunctionContext.IsValid();
}

//////////////////////////////////////////////////////////////////////////
// FScriptEventPlayback

bool FScriptEventPlayback::Process(const TArray<FScriptInstrumentedEvent>& SignalData, const int32 StartIdx, const int32 StopIdx)
{
	const int32 NumEvents = (StopIdx+1) - StartIdx;
	bool bProcessingSuccess = false;
	const bool bEventIsResuming = SignalData[StartIdx].IsResumeEvent() ;
	IBlueprintProfilerInterface* Profiler = FModuleManager::GetModulePtr<IBlueprintProfilerInterface>("BlueprintProfiler");

	if (BlueprintContext.IsValid() && InstanceName != NAME_None)
	{
		check(SignalData[StartIdx].IsEvent());
		EventName = bEventIsResuming ? EventName : SignalData[StartIdx].GetScopedFunctionName();
		TSharedPtr<FBlueprintFunctionContext> FunctionContext = BlueprintContext->GetFunctionContextForEventChecked(EventName);
		CurrentFunctionName = FunctionContext->GetFunctionName();
		TSharedPtr<FScriptExecutionNode> EventNode = FunctionContext->GetProfilerDataForNode(EventName);
		// Find Associated graph nodes and submit into a node map for later processing.
		TMap<const UEdGraphNode*, NodeSignalHelper> NodeMap;
		bProcessingSuccess = true;
		int32 LastEventIdx = StartIdx;
		const int32 EventStartOffset = SignalData[StartIdx].IsResumeEvent() ? 3 : 1;
		LatentLinkId = bEventIsResuming ? INDEX_NONE : LatentLinkId;
		// If we have sub calls into inherited events we don't have properly formed events, find the correct event now.
		if (!EventNode.IsValid() && FunctionContext.IsValid())
		{
			const FScriptInstrumentedEvent& FirstValidSignal = SignalData[StartIdx + EventStartOffset];
			if (const UK2Node_Event* EventGraphNode = Cast<UK2Node_Event>(FunctionContext->GetNodeFromCodeLocation(FirstValidSignal.GetScriptCodeOffset())))
			{
				EventName = FunctionContext->GetScopedEventName(EventGraphNode->GetFunctionName());
				EventNode = FunctionContext->GetProfilerDataForNode(EventName);
			}
		}

		for (int32 SignalIdx = StartIdx + EventStartOffset; SignalIdx < StopIdx; ++SignalIdx)
		{
			// Handle midstream context switches.
			const FScriptInstrumentedEvent& CurrSignal = SignalData[SignalIdx];
			if (CurrSignal.GetType() == EScriptInstrumentation::Class)
			{
				// Update the current mapped blueprint context.
				BlueprintContext = Profiler->GetBlueprintContext(CurrSignal.GetBlueprintClassPath());

				// Skip to the next signal.
				continue;
			}
			else if (CurrSignal.GetType() == EScriptInstrumentation::Instance)
			{
				// Update the current mapped instance name.
				InstanceName = BlueprintContext->MapBlueprintInstance(CurrSignal.GetInstancePath());

				// Skip to the next signal.
				continue;
			}

			// Update script function.
			if (CurrentFunctionName != CurrSignal.GetScopedFunctionName())
			{
				CurrentFunctionName = CurrSignal.GetScopedFunctionName();
				FunctionContext = BlueprintContext->GetFunctionContext(CurrentFunctionName);
				check(FunctionContext.IsValid());
			}
			if (const UEdGraphNode* GraphNode = FunctionContext->GetNodeFromCodeLocation(CurrSignal.GetScriptCodeOffset()))
			{
				NodeSignalHelper& CurrentNodeData = NodeMap.FindOrAdd(GraphNode);
				// Initialise the current node context.
				if (!CurrentNodeData.IsValid())
				{
					FunctionContext->GetProfilerContextFromScriptOffset(CurrSignal.GetScriptCodeOffset(), CurrentNodeData.ImpureNode, CurrentNodeData.FunctionContext);
					check(CurrentNodeData.IsValid());
				}
				// Check for tunnel boundries and process here
				if (CurrentNodeData.ImpureNode->IsTunnelInstance())
				{
					ProcessTunnelBoundary(CurrentNodeData, CurrSignal);
					continue;
				}
				// Process node data
				switch (CurrSignal.GetType())
				{
					case EScriptInstrumentation::PureNodeEntry:
					{
						if (const UEdGraphPin* Pin = FunctionContext->GetPinFromCodeLocation(CurrSignal.GetScriptCodeOffset()))
						{
							// Use the impure nodes function context to look up the pure graph node because it may not be cached yet.
							if (CurrentNodeData.FunctionContext->HasProfilerDataForGraphNode(Pin->GetOwningNode()))
							{
								TracePath.AddExitPin(CurrSignal.GetScriptCodeOffset());
								CurrentNodeData.InputTracePaths.Insert(TracePath, 0);
							}
						}
						CurrentNodeData.AverageEvents.Add(CurrSignal);
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
								const FName NewEventName = FunctionContext->GetScopedEventName(EventGraphNode->GetFunctionName());
								if (NewEventName != EventName)
								{
									TracePath.Reset();
									if (EventNode.IsValid())
									{
										EventTimings.Add(EventName) = SignalData[SignalIdx].GetTime() - SignalData[LastEventIdx].GetTime();
									}
									EventName = NewEventName;
									EventNode = FunctionContext->GetProfilerDataForNode(EventName);
									LastEventIdx = SignalIdx;
								}
							}
						}
						// Add node timings
						CurrentNodeData.AverageTracePaths.Push(TracePath);
						CurrentNodeData.AverageEvents.Add(CurrSignal);
						AddToTraceHistory(CurrentNodeData.ImpureNode, CurrSignal);
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
						// Process tunnel if present
						TSharedPtr<FScriptExecutionTunnelEntry> TunnelEntry = TracePath.GetTunnel();
						if (TunnelEntry.IsValid())
						{
							TSharedPtr<FScriptExecutionTunnelInstance> TunnelInstance = TunnelEntry->GetTunnelInstance();
							check (TunnelInstance.IsValid());
							const UEdGraphNode* TunnelNode = TunnelInstance->GetTypedObservedObject<UEdGraphNode>();
							NodeSignalHelper& TunnelNodeData = NodeMap.FindOrAdd(TunnelNode);
							FScriptInstrumentedEvent OverrideEvent(CurrSignal);
							FTracePath TunnelTrace;
							TracePath.GetTunnelTracePath(TunnelTrace);
							OverrideEvent.OverrideType(EScriptInstrumentation::NodeEntry);
							TunnelNodeData.AverageEvents.Add(OverrideEvent);
							TunnelNodeData.AverageTracePaths.Add(TunnelTrace);
							TunnelTraceStack.Push(TunnelTrace);
						}
						break;
					}
					case EScriptInstrumentation::SuspendState:
					{
						// Handle latent suspends - make use of the link id to match re-entry.
						TSharedPtr<FScriptExecutionInstance> InstanceNode = BlueprintContext->GetInstanceExecNode(InstanceName);
						if (UObject* InstanceObj = InstanceNode->GetActiveObject())
						{
							if (UWorld* WorldForEvent = InstanceObj->GetWorld())
							{
								FLatentActionManager& LatentActionManager = WorldForEvent->GetLatentActionManager();
								TSet<int32> UUIDSet;
								int32 UUID = INDEX_NONE;
								LatentActionManager.GetActiveUUIDs(InstanceObj, UUIDSet);
								for (auto SetEntry : UUIDSet)
								{
									UUID = SetEntry;
								}
								if (UUID != INDEX_NONE)
								{
									if (FDelayAction* Action = LatentActionManager.FindExistingAction<FDelayAction>(InstanceObj, UUID))
									{
										LatentLinkId = Action->OutputLink;
									}
								}
							}
						}
						break;
					}
					case EScriptInstrumentation::NodeExit:
					{
						// Cleanup branching multiple exits and correct the tracepath
						if (CurrentNodeData.AverageEvents.Num() && CurrentNodeData.AverageEvents.Last().GetType() == EScriptInstrumentation::NodeExit)
						{
							CurrentNodeData.AverageEvents.Pop();
							if (CurrentNodeData.AverageTracePaths.Num())
							{
								TracePath = CurrentNodeData.AverageTracePaths.Last();
							}
						}
						// Add Trace History
						AddToTraceHistory(CurrentNodeData.ImpureNode, CurrSignal);
						// Process node exit
						const int32 ScriptCodeExit = CurrSignal.GetScriptCodeOffset();
						if (const UEdGraphPin* ValidPin = FunctionContext->GetPinFromCodeLocation(ScriptCodeExit))
						{
							TracePath.AddExitPin(ScriptCodeExit);
						}
						CurrentNodeData.AverageEvents.Add(CurrSignal);
						// Process cyclic linkage - reset to root link tracepath
						TSharedPtr<FScriptExecutionNode> NextLink = CurrentNodeData.ImpureNode->GetLinkedNodeByScriptOffset(ScriptCodeExit);
						if (NextLink.IsValid() && NextLink->HasCyclicLinkage())
						{
							const UEdGraphNode* LinkedGraphNode = Cast<UEdGraphNode>(NextLink->GetObservedObject());
							if (NodeSignalHelper* LinkedEntry = NodeMap.Find(LinkedGraphNode))
							{
								if (LinkedEntry->AverageTracePaths.Num())
								{
									TracePath = LinkedEntry->AverageTracePaths.Last();
								}
							}
						}
						break;
					}
					default:
					{
						CurrentNodeData.AverageEvents.Add(CurrSignal);
						break;
					}
				}
			}
		}
		// Process last event timing
		if (EventNode.IsValid())
		{
			double* TimingData = bEventIsResuming ? EventTimings.Find(EventName) : nullptr;
			if (TimingData)
			{
				*TimingData += SignalData[StopIdx].GetTime() - SignalData[LastEventIdx].GetTime();
			}
			else
			{
				EventTimings.Add(EventName) = SignalData[StopIdx].GetTime() - SignalData[LastEventIdx].GetTime();
			}
		}
		// Process outstanding event timings, adding to previous timings if existing.
		for (auto EventTiming : EventTimings)
		{
			FTracePath EventTracePath;
			// The UCS, along with BP events declared as 'const' in native C++ code, are implemented as standalone functions, and not within the ubergraph function context.
			FunctionContext = BlueprintContext->GetFunctionContextForEventChecked(EventTiming.Key);
			EventNode = FunctionContext->GetProfilerDataForNode(EventTiming.Key);
			check (EventNode.IsValid());
			TSharedPtr<FScriptPerfData> PerfData = EventNode->GetOrAddPerfDataByInstanceAndTracePath(InstanceName, EventTracePath);
			PerfData->AddEventTiming(SignalData[StopIdx].GetTime() - SignalData[LastEventIdx].GetTime());
		}
		EventTimings.Reset();
		// Process Node map timings -- this can probably be rolled back into submission during the initial processing and lose this extra iteration.
		for (auto CurrentNodeData : NodeMap)
		{
			TSharedPtr<FScriptExecutionNode> ExecNode = CurrentNodeData.Value.ImpureNode;
			TSharedPtr<FScriptExecutionNode> PureNode;
			TSharedPtr<FBlueprintFunctionContext> NodeFunctionContext = CurrentNodeData.Value.FunctionContext;
			check(NodeFunctionContext.IsValid());
			double PureNodeEntryTime = 0.0;
			double PureChainEntryTime = 0.0;
			double NodeEntryTime = 0.0;
			int32 ExclTracePathIdx = 0;
			int32 InclTracePathIdx = 0;
			// Process exclusive events for this node
			for (auto EventIter = CurrentNodeData.Value.AverageEvents.CreateIterator(); EventIter; ++EventIter)
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
						
						const int32 ScriptCodeOffset = EventIter->GetScriptCodeOffset();
						if (const UEdGraphPin* Pin = NodeFunctionContext->GetPinFromCodeLocation(ScriptCodeOffset))
						{
							if (NodeFunctionContext->HasProfilerDataForNode(Pin->GetOwningNode()->GetFName()))
							{
								PureNode = NodeFunctionContext->GetProfilerDataForNode(Pin->GetOwningNode()->GetFName());
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
						const FTracePath NodeTracePath = CurrentNodeData.Value.AverageTracePaths[ExclTracePathIdx++];
						TSharedPtr<FScriptPerfData> PerfData = ExecNode->GetOrAddPerfDataByInstanceAndTracePath(InstanceName, NodeTracePath);
						double PureChainDuration = PureChainEntryTime != 0.0 ? (NodeEntryTime - PureChainEntryTime) : 0.0;
						double NodeDuration = EventIter->GetTime() - NodeEntryTime;
						PerfData->AddEventTiming(NodeDuration);
						PerfData->AddInclusiveTiming(PureChainDuration, false);
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
							PerfData->AddInclusiveTiming(NodeDuration, false);
							break;
						}
					}
				}
			}
		}
	}
	return bProcessingSuccess;
}

void FScriptEventPlayback::ProcessTunnelBoundary(NodeSignalHelper& CurrentNodeData, const FScriptInstrumentedEvent& CurrSignal)
{
	// Find grab the tunnel instance exec node.
	TSharedPtr<FScriptExecutionTunnelInstance> TunnelInstance = StaticCastSharedPtr<FScriptExecutionTunnelInstance>(CurrentNodeData.ImpureNode);
	// Then the Boundary node by script offset.
	const int32 ScriptCodeOffset = CurrSignal.GetScriptCodeOffset();
	TSharedPtr<FScriptExecutionNode> TunnelBoundary = TunnelInstance->FindBoundarySite(ScriptCodeOffset);
	if (TunnelBoundary.IsValid())
	{
		if (TunnelBoundary->HasFlags(EScriptExecutionNodeFlags::TunnelEntry))
		{
			// Grab the tunnel entry
			TSharedPtr<FScriptExecutionTunnelEntry> TunnelEntry = StaticCastSharedPtr<FScriptExecutionTunnelEntry>(TunnelBoundary);
			// Process tunnel entry sites
			TunnelTraceStack.Push(TracePath);
			TracePath = FTracePath(TracePath, TunnelEntry);
			CurrentNodeData.AverageEvents.Add(CurrSignal);
			// Update the tunnel entry count so we can fixup stat samples later.
			TunnelEntry->IncrementTunnelEntryCount();
			// Add Trace History
			AddToTraceHistory(TunnelBoundary, CurrSignal);
		}
		else if (TunnelBoundary->HasFlags(EScriptExecutionNodeFlags::TunnelExit))
		{
			// Process tunel exit sites.
			if (CurrentNodeData.AverageEvents.Num())
			{
				TSharedPtr<FScriptExecutionTunnelEntry> Tunnel = TracePath.GetTunnel();
				FTracePath InternalExitTrace = TracePath;
				TracePath = TunnelTraceStack.Pop();
				if (Tunnel.IsValid())
				{
					// Update the entry and both exit sites ( one inside the tunnel and one exit site )
					const double TunnelTiming = CurrSignal.GetTime() - CurrentNodeData.AverageEvents.Last().GetTime();
					Tunnel->AddTunnelTiming(InstanceName, TracePath, InternalExitTrace, ScriptCodeOffset, TunnelTiming);
				}
				TracePath.AddExitPin(ScriptCodeOffset);
				CurrentNodeData.AverageEvents.Reset();
				// Add Trace History
				AddToTraceHistory(TunnelBoundary, CurrSignal);
			}
		}
	}
}

void FScriptEventPlayback::ProcessExecutionSequence(NodeSignalHelper& CurrentNodeData, const FScriptInstrumentedEvent& CurrSignal)
{
	switch(CurrSignal.GetType())
	{
		// For a sequence node a push state represents the start of execution.
		case EScriptInstrumentation::PushState:
		{
			// Convert the push state into a inclusive entry signal
			FScriptInstrumentedEvent OverrideEvent(CurrSignal);
			OverrideEvent.OverrideType(EScriptInstrumentation::NodeEntry);
			CurrentNodeData.InclusiveEvents.Add(OverrideEvent);
			CurrentNodeData.InclusiveTracePaths.Add(TracePath);
			break;
		}
		// For a sequence node a restore state represents the end of execution of a sequence pin's path (excluding the last pin)
		case EScriptInstrumentation::RestoreState:
		{
			// Convert the restore state into a node exit signal
			FScriptInstrumentedEvent OverrideEvent(CurrSignal);
			OverrideEvent.OverrideType(EScriptInstrumentation::NodeEntry);
			CurrentNodeData.AverageEvents.Add(OverrideEvent);
			CurrentNodeData.AverageTracePaths.Add(TracePath);
			// Add Trace History
			AddToTraceHistory(CurrentNodeData.ImpureNode, OverrideEvent);
			break;
		}
		// For a sequence node a pop state represents the end of execution of all the sequence pins.
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

void FScriptEventPlayback::AddToTraceHistory(const TSharedPtr<FScriptExecutionNode> ProfilerNode, const FScriptInstrumentedEvent& TraceSignal)
{
	// Add Trace History
	FBlueprintExecutionTrace& NewTraceEvent = BlueprintContext->AddNewTraceHistoryEvent();
	NewTraceEvent.TraceType = TraceSignal.GetType();
	NewTraceEvent.TracePath = TracePath;
	NewTraceEvent.InstanceName = InstanceName;
	NewTraceEvent.FunctionName = CurrentFunctionName;
	NewTraceEvent.ProfilerNode = ProfilerNode;
	NewTraceEvent.Offset = TraceSignal.GetScriptCodeOffset();
	NewTraceEvent.ObservationTime = TraceSignal.GetTime();
}

#undef LOCTEXT_NAMESPACE
