// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "KismetCompiler.h"

#define LOCTEXT_NAMESPACE "FKCHandler_TunnelBoundary"

//////////////////////////////////////////////////////////////////////////
// FKCHandler_TunnelBoundary

class FKCHandler_TunnelBoundary : public FNodeHandlingFunctor
{
public:
	FKCHandler_TunnelBoundary(FKismetCompilerContext& InCompilerContext)
		: FNodeHandlingFunctor(InCompilerContext)
	{
	}

	virtual void Compile(FKismetFunctionContext& Context, UEdGraphNode* Node) override
	{
		if (UK2Node_TunnelBoundary* BoundaryNode = Cast<UK2Node_TunnelBoundary>(Node))
		{
			switch(BoundaryNode->TunnelBoundaryType)
			{
				case TBT_EntrySite:
				case TBT_ExitSite:
				{
		GenerateSimpleThenGoto(Context, *Node);
					break;
				}
				case TBT_EndOfThread:
				{
					FBlueprintCompiledStatement& ExitTunnelStatement = Context.AppendStatementForNode(Node);
					ExitTunnelStatement.Type = KCST_InstrumentedTunnelEndOfThread;
					FBlueprintCompiledStatement& EOTStatement = Context.AppendStatementForNode(Node);
					EOTStatement.Type = KCST_EndOfThread;
					break;
				}
			}
		}
	}

};

//////////////////////////////////////////////////////////////////////////
// UK2Node_TunnelBoundary

UK2Node_TunnelBoundary::UK2Node_TunnelBoundary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, TunnelBoundaryType(TBT_Unknown)
{
	return;
}

FText UK2Node_TunnelBoundary::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	FFormatNamedArguments Args;
	Args.Add(TEXT("GraphName"), FText::FromName(TunnelGraphName));
	switch(TunnelBoundaryType)
	{
		case TBT_EntrySite:
		{
			Args.Add(TEXT("EntryType"), FText::FromString(TEXT("Tunnel Entry")));
			break;
		}
		case TBT_ExitSite:
		{
			Args.Add(TEXT("EntryType"), FText::FromString(TEXT("Tunnel Exit")));
			break;
		}
		case TBT_EndOfThread:
		{
			Args.Add(TEXT("EntryType"), FText::FromString(TEXT("Tunnel End Of Thread")));
			break;
		}
	}
	return FText::Format(LOCTEXT("UK2Node_TunnelBoundary_FullTitle", "{GraphName} {EntryType}"), Args);
}

FNodeHandlingFunctor* UK2Node_TunnelBoundary::CreateNodeHandler(FKismetCompilerContext& CompilerContext) const
{
	return new FKCHandler_TunnelBoundary(CompilerContext);
}

void UK2Node_TunnelBoundary::CreateBoundaryNodesForGraph(UEdGraph* TunnelGraph, class FCompilerResultsLog& MessageLog)
{
	if (UK2Node_Tunnel* OuterTunnel = TunnelGraph->GetTypedOuter<UK2Node_Tunnel>())
	{
		UK2Node_Tunnel* SourceTunnel = Cast<UK2Node_Tunnel>(MessageLog.FindSourceObject(OuterTunnel));
		CreateBoundaryNodesForTunnelInstance(SourceTunnel, TunnelGraph, MessageLog);
	}
}

void UK2Node_TunnelBoundary::CreateBoundaryNodesForTunnelInstance(UK2Node_Tunnel* TunnelInstance, UEdGraph* TunnelGraph, class FCompilerResultsLog& MessageLog)
{
	if (!TunnelGraph)
	{
		return;
	}

	TArray<UEdGraphPin*> TunnelEntryPins;
	TArray<UEdGraphPin*> TunnelExitPins;
	UEdGraphNode* TunnelExitNode = nullptr;
	TArray<UK2Node_Tunnel*> Tunnels;
	TunnelGraph->GetNodesOfClass<UK2Node_Tunnel>(Tunnels);
	for (auto TunnelNode : Tunnels)
	{
		if (IsPureTunnel(TunnelNode))
		{
			for (UEdGraphPin* Pin : TunnelNode->Pins)
			{
				if (Pin->LinkedTo.Num() && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
				{
					if (Pin->Direction == EGPD_Output)
					{
						TunnelEntryPins.Add(Pin);
					}
					else
					{
						TunnelExitNode = TunnelNode;
						TunnelExitPins.Add(Pin);
					}
				}
			}
		}
	}
	// Find the blueprint tunnel instance.
	UEdGraphNode* SourceTunnelInstance = Cast<UEdGraphNode>(MessageLog.FindSourceObject(TunnelInstance));
	SourceTunnelInstance = FindTrueSourceTunnelInstance(TunnelInstance, SourceTunnelInstance);
	// Create the Boundary nodes for each unique entry/exit site.
	TArray<UEdGraphPin*> ExecutionEndpointPins;
	TArray<UEdGraphPin*> VisitedPins;
	for (UEdGraphPin* EntryPin : TunnelEntryPins)
	{
		UK2Node_TunnelBoundary* TunnelBoundaryNode = TunnelGraph->CreateBlankNode<UK2Node_TunnelBoundary>();
		MessageLog.NotifyIntermediateObjectCreation(TunnelBoundaryNode, TunnelInstance);
		TunnelBoundaryNode->CreateNewGuid();
		MessageLog.IntermediateTunnelNodeToSourceNodeMap.Add(TunnelBoundaryNode, SourceTunnelInstance);
		TunnelBoundaryNode->WireUpTunnelEntry(TunnelInstance, EntryPin, MessageLog);
		for (auto LinkedPin : EntryPin->LinkedTo)
		{
			FindTunnelExitSiteInstances(LinkedPin, ExecutionEndpointPins, VisitedPins, TunnelExitNode);
		}
	}
	if (ExecutionEndpointPins.Num())
	{
		// Create boundary node.
		UK2Node_TunnelBoundary* TunnelBoundaryNode = TunnelGraph->CreateBlankNode<UK2Node_TunnelBoundary>();
		MessageLog.NotifyIntermediateObjectCreation(TunnelBoundaryNode, TunnelInstance);
		TunnelBoundaryNode->CreateNewGuid();
		TunnelBoundaryNode->TunnelBoundaryType = TBT_EndOfThread;
		MessageLog.IntermediateTunnelNodeToSourceNodeMap.Add(TunnelBoundaryNode, SourceTunnelInstance);
		// Create exit point pin
		UEdGraphPin* BoundaryTerminalPin = TunnelBoundaryNode->CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, TEXT(""), NULL, false, false, TEXT("TunnelEndOfThread"));
		// Wire up the endpoint unwired links to the boundary node.
		for (UEdGraphPin* TerminationPin : ExecutionEndpointPins)
		{
			TerminationPin->MakeLinkTo(BoundaryTerminalPin);
		}
	}
	for (UEdGraphPin* ExitPin : TunnelExitPins)
	{
		UK2Node_TunnelBoundary* TunnelBoundaryNode = TunnelGraph->CreateBlankNode<UK2Node_TunnelBoundary>();
		MessageLog.NotifyIntermediateObjectCreation(TunnelBoundaryNode, TunnelInstance);
		TunnelBoundaryNode->CreateNewGuid();
		MessageLog.IntermediateTunnelNodeToSourceNodeMap.Add(TunnelBoundaryNode, SourceTunnelInstance);
		TunnelBoundaryNode->WireUpTunnelExit(TunnelInstance, ExitPin, MessageLog);
	}
	// Build node guid map to locate true source node.
	TMap<FGuid, const UEdGraphNode*> TrueSourceNodeMap;
	BuildSourceNodeMap(SourceTunnelInstance, TrueSourceNodeMap);
	// Register all the nodes to the tunnel instance map
	if (SourceTunnelInstance)
	{
		for (auto Node : TunnelGraph->Nodes)
		{
			MessageLog.SourceNodeToTunnelInstanceNodeMap.Add(Node, SourceTunnelInstance);
			if (const UEdGraphNode** SearchNode = TrueSourceNodeMap.Find(Node->NodeGuid))
			{
				const UEdGraphNode* SourceGraphNode = *SearchNode;
				MessageLog.IntermediateTunnelNodeToSourceNodeMap.Add(Node, SourceGraphNode);
			}
		}
		MessageLog.SourceNodeToTunnelInstanceNodeMap.Add(TunnelInstance, SourceTunnelInstance);
	}
}

bool UK2Node_TunnelBoundary::IsPureTunnel(UK2Node_Tunnel* Tunnel)
{
	return !Tunnel->IsA<UK2Node_MacroInstance>() && !Tunnel->IsA<UK2Node_Composite>();
}

void UK2Node_TunnelBoundary::WireUpTunnelEntry(UK2Node_Tunnel* TunnelInstance, UEdGraphPin* TunnelPin, FCompilerResultsLog& MessageLog)
{
	if (TunnelInstance && TunnelPin)
	{
		// Mark as entry node
		TunnelBoundaryType = TBT_EntrySite;
		// Grab the graph name
		DetermineTunnelGraphName(TunnelInstance);
		// Find the true source pin
		UEdGraphPin* SourcePin = TunnelInstance->FindPin(TunnelPin->PinName);
		// Wire in the node
		UEdGraphPin* OutputPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, TEXT(""), NULL, false, false, TEXT("TunnelEntryExec"));
		for (auto LinkedPin : TunnelPin->LinkedTo)
		{
			LinkedPin->MakeLinkTo(OutputPin);
		}
		UEdGraphPin* InputPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, TEXT(""), NULL, false, false, SourcePin->PinName);
		MessageLog.NotifyIntermediatePinCreation(InputPin, SourcePin);
		TunnelPin->BreakAllPinLinks();
		TunnelPin->MakeLinkTo(InputPin);
	}
}

void UK2Node_TunnelBoundary::WireUpTunnelExit(UK2Node_Tunnel* TunnelInstance, UEdGraphPin* TunnelPin, FCompilerResultsLog& MessageLog)
{
	if (TunnelInstance && TunnelPin)
	{
		// Mark as exit node
		TunnelBoundaryType = TBT_ExitSite;
		// Grab the graph name
		DetermineTunnelGraphName(TunnelInstance);
		// Find the true source pin
		UEdGraphPin* SourcePin = TunnelInstance->FindPin(TunnelPin->PinName);
		// Wire in the node
		UEdGraphPin* InputPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, TEXT(""), NULL, false, false, SourcePin->PinName);
		MessageLog.NotifyIntermediatePinCreation(InputPin, SourcePin);
		for (auto LinkedPin : TunnelPin->LinkedTo)
		{
			LinkedPin->MakeLinkTo(InputPin);
		}
		UEdGraphPin* OutputPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, TEXT(""), NULL, false, false, TEXT("TunnelExitExec"));
		TunnelPin->BreakAllPinLinks();
		TunnelPin->MakeLinkTo(OutputPin);
	}
}

void UK2Node_TunnelBoundary::DetermineTunnelGraphName(UK2Node_Tunnel* TunnelInstance)
{
	UEdGraph* TunnelGraph = nullptr;
	if (const UK2Node_MacroInstance* MacroInstance = Cast<UK2Node_MacroInstance>(TunnelInstance))
	{
		TunnelGraph = MacroInstance->GetMacroGraph();
	}
	else if (const UK2Node_Composite* CompositeInstance = Cast<UK2Node_Composite>(TunnelInstance))
	{
		TunnelGraph = CompositeInstance->BoundGraph;
	}
	TunnelGraphName = TunnelGraph ? TunnelGraph->GetFName() : NAME_None;
}

void UK2Node_TunnelBoundary::BuildSourceNodeMap(UEdGraphNode* Tunnel, TMap<FGuid, const UEdGraphNode*>& SourceNodeMap)
{
	// Crawl the graph and locate source node guids
	UEdGraph* TunnelGraph = nullptr;
	if (UK2Node_MacroInstance* SourceMacroInstance = Cast<UK2Node_MacroInstance>(Tunnel))
	{
		TunnelGraph = SourceMacroInstance->GetMacroGraph();
	}
	else if (UK2Node_Composite* SourceCompositeInstance = Cast<UK2Node_Composite>(Tunnel))
	{
		TunnelGraph = SourceCompositeInstance->BoundGraph;
	}
	if (TunnelGraph)
	{
		for (auto GraphNode : TunnelGraph->Nodes)
		{
			SourceNodeMap.Add(GraphNode->NodeGuid) = GraphNode;
		}
	}
}

UEdGraphNode* UK2Node_TunnelBoundary::FindTrueSourceTunnelInstance(UEdGraphNode* Tunnel, UEdGraphNode* SourceTunnelInstance)
{
	UEdGraphNode* SourceNode = nullptr;
	if (Tunnel && SourceTunnelInstance)
	{
		if (Tunnel->NodeGuid == SourceTunnelInstance->NodeGuid)
		{
			SourceNode = SourceTunnelInstance;
		}
		else
		{
			UEdGraph* TunnelGraph = nullptr;
			if (UK2Node_MacroInstance* SourceMacroInstance = Cast<UK2Node_MacroInstance>(SourceTunnelInstance))
			{
				TunnelGraph = SourceMacroInstance->GetMacroGraph();
			}
			else if (UK2Node_Composite* SourceCompositeInstance = Cast<UK2Node_Composite>(SourceTunnelInstance))
			{
				TunnelGraph = SourceCompositeInstance->BoundGraph;
			}
			if (TunnelGraph)
			{
				for (auto GraphNode : TunnelGraph->Nodes)
				{
					if (GraphNode->NodeGuid == Tunnel->NodeGuid)
					{
						SourceNode = GraphNode;
						break;
					}

					if (GraphNode->IsA<UK2Node_Composite>() || GraphNode->IsA<UK2Node_MacroInstance>())
					{
						SourceNode = FindTrueSourceTunnelInstance(Tunnel, GraphNode);
					if (SourceNode)
					{
						break;
					}
				}
			}
		}
	}
	}
	return SourceNode;
}

void UK2Node_TunnelBoundary::FindTunnelExitSiteInstances(UEdGraphPin* NodeEntryPin, TArray<UEdGraphPin*>& ExitPins, TArray<UEdGraphPin*>& VisitedPins, const UEdGraphNode* TunnelExitNode)
{
	UEdGraphNode* PinNode = NodeEntryPin->GetOwningNode();

	// Grab pin node
	if (PinNode != nullptr && PinNode != TunnelExitNode && !VisitedPins.Contains(NodeEntryPin))
	{
		VisitedPins.Push(NodeEntryPin);

		// Find all exec out pins
		TArray<UEdGraphPin*> ExecPins;
		TArray<UEdGraphPin*> ConnectedExecPins;
		for (auto Pin : PinNode->Pins)
		{
			if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
			{
				ExecPins.Add(Pin);
				if (Pin->LinkedTo.Num() > 0)
				{
					ConnectedExecPins.Add(Pin);
				}
			}
		}
		if (ConnectedExecPins.Num() > 1)
		{
//			if (FEdGraphUtilities::IsExecutionSequence(PinNode))
			if (PinNode->IsA<UK2Node_ExecutionSequence>())
			{
				// Execution sequence style nodes, follow the last connected pin
				for (auto LinkedPin : ConnectedExecPins.Last()->LinkedTo)
				{
					FindTunnelExitSiteInstances(LinkedPin, ExitPins, VisitedPins);
				}
			}
			else
			{
				// Branch style nodes, more tricky
				for (auto ExecPin : ConnectedExecPins)
				{
					for (auto LinkedPin : ExecPin->LinkedTo)
					{
						FindTunnelExitSiteInstances(LinkedPin, ExitPins, VisitedPins);
					}
				}
			}
		}
		else if (ConnectedExecPins.Num() == 1)
		{
			for (auto LinkedPin : ConnectedExecPins[0]->LinkedTo)
			{
				FindTunnelExitSiteInstances(LinkedPin, ExitPins, VisitedPins);
			}
		}
		else if (ExecPins.Num() > 0)
		{
			ExitPins.Add(ExecPins.Last());
		}

		VisitedPins.Pop(false);
	}
}

#undef LOCTEXT_NAMESPACE
