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
		GenerateSimpleThenGoto(Context, *Node);
	}

};

//////////////////////////////////////////////////////////////////////////
// UK2Node_TunnelBoundary

UK2Node_TunnelBoundary::UK2Node_TunnelBoundary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bEntryNode(false)
{
	return;
}

FText UK2Node_TunnelBoundary::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	FFormatNamedArguments Args;
	Args.Add(TEXT("GraphName"), FText::FromName(TunnelGraphName));
	Args.Add(TEXT("EntryType"), bEntryNode ? FText::FromString(TEXT("Tunnel Entry")) : FText::FromString(TEXT("Tunnel Exit")));
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
	TArray<UEdGraphPin*> TunnelEntryPins;
	TArray<UEdGraphPin*> TunnelExitPins;
	if (TunnelGraph)
	{
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
							TunnelExitPins.Add(Pin);
						}
					}
				}
			}
		}
	}
	// Create the Boundary nodes for each unique entry/exit site.
	for (UEdGraphPin* EntryPin : TunnelEntryPins)
	{
		UK2Node_TunnelBoundary* TunnelBoundaryNode = TunnelGraph->CreateBlankNode<UK2Node_TunnelBoundary>();
		MessageLog.NotifyIntermediateObjectCreation(TunnelBoundaryNode, TunnelInstance);
		TunnelBoundaryNode->CreateNewGuid();
		TunnelBoundaryNode->WireUpTunnelEntry(TunnelInstance, EntryPin, MessageLog);
	}
	for (UEdGraphPin* ExitPin : TunnelExitPins)
	{
		UK2Node_TunnelBoundary* TunnelBoundaryNode = TunnelGraph->CreateBlankNode<UK2Node_TunnelBoundary>();
		MessageLog.NotifyIntermediateObjectCreation(TunnelBoundaryNode, TunnelInstance);
		TunnelBoundaryNode->CreateNewGuid();
		TunnelBoundaryNode->WireUpTunnelExit(TunnelInstance, ExitPin, MessageLog);
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
		bEntryNode = true;
		// Grab the graph name
		DetermineTunnelGraphName(TunnelInstance);
		// Find the true source pin
		UEdGraphPin* SourcePin = TunnelInstance->FindPin(TunnelPin->PinName);
		// Wire in the node
		UEdGraphPin* OutputPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, TEXT(""), NULL, false, false, TEXT("TunnelEntryExec"));
		MessageLog.NotifyIntermediatePinCreation(OutputPin, SourcePin);
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
		bEntryNode = false;
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
		MessageLog.NotifyIntermediatePinCreation(OutputPin, SourcePin);
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

#undef LOCTEXT_NAMESPACE
