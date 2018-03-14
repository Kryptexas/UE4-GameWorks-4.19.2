// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NiagaraStackGraphUtilities.h"
#include "NiagaraParameterMapHistory.h"
#include "NiagaraSystemViewModel.h"
#include "NiagaraSystemScriptViewModel.h"
#include "NiagaraGraph.h"
#include "NiagaraNode.h"
#include "NiagaraNodeOutput.h"
#include "NiagaraNodeInput.h"
#include "NiagaraNodeFunctionCall.h"
#include "NiagaraNodeAssignment.h"
#include "NiagaraMaterialParameterNode.h"
#include "NiagaraNodeParameterMapGet.h"
#include "NiagaraNodeParameterMapSet.h"
#include "NiagaraScriptSource.h"
#include "EdGraphSchema_Niagara.h"
#include "NiagaraStackEntry.h"
#include "NiagaraStackFunctionInputCollection.h"
#include "NiagaraStackFunctionInput.h"
#include "NiagaraSystem.h"
#include "NiagaraEditorModule.h"
#include "NiagaraEditorUtilities.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "NiagaraEmitterViewModel.h"

DECLARE_CYCLE_STAT(TEXT("Niagara - StackGraphUtilities - RelayoutGraph"), STAT_NiagaraEditor_StackGraphUtilities_RelayoutGraph, STATGROUP_NiagaraEditor);

#define LOCTEXT_NAMESPACE "NiagaraStackGraphUtilities"

void FNiagaraStackGraphUtilities::RelayoutGraph(UEdGraph& Graph)
{
	SCOPE_CYCLE_COUNTER(STAT_NiagaraEditor_StackGraphUtilities_RelayoutGraph);
	TArray<TArray<TArray<UEdGraphNode*>>> OutputNodeTraversalStacks;
	TArray<UNiagaraNodeOutput*> OutputNodes;
	Graph.GetNodesOfClass(OutputNodes);
	TSet<UEdGraphNode*> AllTraversedNodes;
	for (UNiagaraNodeOutput* OutputNode : OutputNodes)
	{
		TSet<UEdGraphNode*> TraversedNodes;
		TArray<TArray<UEdGraphNode*>> TraversalStack;
		TArray<UEdGraphNode*> CurrentNodesToTraverse;
		CurrentNodesToTraverse.Add(OutputNode);
		while (CurrentNodesToTraverse.Num() > 0)
		{
			TArray<UEdGraphNode*> TraversedNodesThisLevel;
			TArray<UEdGraphNode*> NextNodesToTraverse;
			for (UEdGraphNode* CurrentNodeToTraverse : CurrentNodesToTraverse)
			{
				if (TraversedNodes.Contains(CurrentNodeToTraverse))
				{
					continue;
				}
				
				for (UEdGraphPin* Pin : CurrentNodeToTraverse->GetAllPins())
				{
					if (Pin->Direction == EGPD_Input)
					{
						for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
						{
							if (LinkedPin->GetOwningNode() != nullptr)
							{
								NextNodesToTraverse.Add(LinkedPin->GetOwningNode());
							}
						}
					}
				}
				TraversedNodes.Add(CurrentNodeToTraverse);
				TraversedNodesThisLevel.Add(CurrentNodeToTraverse);
			}
			TraversalStack.Add(TraversedNodesThisLevel);
			CurrentNodesToTraverse.Empty();
			CurrentNodesToTraverse.Append(NextNodesToTraverse);
		}
		OutputNodeTraversalStacks.Add(TraversalStack);
		AllTraversedNodes = AllTraversedNodes.Union(TraversedNodes);
	}

	// Find all nodes which were not traversed and put them them in a separate traversal stack.
	TArray<UEdGraphNode*> UntraversedNodes;
	for (UEdGraphNode* Node : Graph.Nodes)
	{
		if (AllTraversedNodes.Contains(Node) == false)
		{
			UntraversedNodes.Add(Node);
		}
	}
	TArray<TArray<UEdGraphNode*>> UntraversedNodeStack;
	for (UEdGraphNode* UntraversedNode : UntraversedNodes)
	{
		TArray<UEdGraphNode*> UntraversedStackItem;
		UntraversedStackItem.Add(UntraversedNode);
		UntraversedNodeStack.Add(UntraversedStackItem);
	}
	OutputNodeTraversalStacks.Add(UntraversedNodeStack);

	// Layout the traversed node stacks.
	float YOffset = 0;
	float XDistance = 400;
	float YDistance = 50;
	float YPinDistance = 50;
	for (const TArray<TArray<UEdGraphNode*>>& TraversalStack : OutputNodeTraversalStacks)
	{
		float CurrentXOffset = 0;
		float MaxYOffset = YOffset;
		for (const TArray<UEdGraphNode*> TraversalLevel : TraversalStack)
		{
			float CurrentYOffset = YOffset;
			for (UEdGraphNode* Node : TraversalLevel)
			{
				Node->Modify();
				Node->NodePosX = CurrentXOffset;
				Node->NodePosY = CurrentYOffset;
				int NumInputPins = 0;
				int NumOutputPins = 0;
				for (UEdGraphPin* Pin : Node->GetAllPins())
				{
					if (Pin->Direction == EGPD_Input)
					{
						NumInputPins++;
					}
					else
					{
						NumOutputPins++;
					}
				}
				int MaxPins = FMath::Max(NumInputPins, NumOutputPins);
				CurrentYOffset += YDistance + (MaxPins * YPinDistance);
			}
			MaxYOffset = FMath::Max(MaxYOffset, CurrentYOffset);
			CurrentXOffset -= XDistance;
		}
		YOffset = MaxYOffset + YDistance;
	}

	Graph.NotifyGraphChanged();
}

void FNiagaraStackGraphUtilities::GetWrittenVariablesForGraph(UEdGraph& Graph, TArray<FNiagaraVariable>& OutWrittenVariables)
{
	TArray<UNiagaraNodeOutput*> OutputNodes;
	Graph.GetNodesOfClass<UNiagaraNodeOutput>(OutputNodes);
	for (UNiagaraNodeOutput* OutputNode : OutputNodes)
	{
		TArray<UEdGraphPin*> InputPins;
		OutputNode->GetInputPins(InputPins);
		if (InputPins.Num() == 1)
		{
			FNiagaraParameterMapHistoryBuilder Builder;
			Builder.BuildParameterMaps(OutputNode, true);
			check(Builder.Histories.Num() == 1);
			for (int32 i = 0; i < Builder.Histories[0].Variables.Num(); i++)
			{
				if (Builder.Histories[0].PerVariableWriteHistory[i].Num() > 0)
				{
					OutWrittenVariables.Add(Builder.Histories[0].Variables[i]);
				}
			}
		}
	}
}

void FNiagaraStackGraphUtilities::ConnectPinToInputNode(UEdGraphPin& Pin, UNiagaraNodeInput& InputNode)
{
	TArray<UEdGraphPin*> InputPins;
	InputNode.GetOutputPins(InputPins);
	if (InputPins.Num() == 1)
	{
		Pin.MakeLinkTo(InputPins[0]);
	}
}

UEdGraphPin* GetParameterMapPin(const TArray<UEdGraphPin*>& Pins)
{
	auto IsParameterMapPin = [](const UEdGraphPin* Pin)
	{
		const UEdGraphSchema_Niagara* NiagaraSchema = CastChecked<UEdGraphSchema_Niagara>(Pin->GetSchema());
		FNiagaraTypeDefinition PinDefinition = NiagaraSchema->PinToTypeDefinition(Pin);
		return PinDefinition == FNiagaraTypeDefinition::GetParameterMapDef();
	};

	UEdGraphPin*const* ParameterMapPinPtr = Pins.FindByPredicate(IsParameterMapPin);

	return ParameterMapPinPtr != nullptr ? *ParameterMapPinPtr : nullptr;
}

UEdGraphPin* FNiagaraStackGraphUtilities::GetParameterMapInputPin(UNiagaraNode& Node)
{
	TArray<UEdGraphPin*> InputPins;
	Node.GetInputPins(InputPins);
	return GetParameterMapPin(InputPins);
}

UEdGraphPin* FNiagaraStackGraphUtilities::GetParameterMapOutputPin(UNiagaraNode& Node)
{
	TArray<UEdGraphPin*> OutputPins;
	Node.GetOutputPins(OutputPins);
	return GetParameterMapPin(OutputPins);
}

void FNiagaraStackGraphUtilities::GetOrderedModuleNodes(UNiagaraNodeOutput& OutputNode, TArray<UNiagaraNodeFunctionCall*>& ModuleNodes)
{
	UNiagaraNode* PreviousNode = &OutputNode;
	while (PreviousNode != nullptr)
	{
		UEdGraphPin* PreviousNodeInputPin = FNiagaraStackGraphUtilities::GetParameterMapInputPin(*PreviousNode);
		if (PreviousNodeInputPin != nullptr && PreviousNodeInputPin->LinkedTo.Num() == 1)
		{
			UNiagaraNode* CurrentNode = Cast<UNiagaraNode>(PreviousNodeInputPin->LinkedTo[0]->GetOwningNode());
			UNiagaraNodeFunctionCall* ModuleNode = Cast<UNiagaraNodeFunctionCall>(CurrentNode);
			if (ModuleNode != nullptr)
			{
				ModuleNodes.Insert(ModuleNode, 0);
			}
			PreviousNode = CurrentNode;
		}
		else
		{
			PreviousNode = nullptr;
		}
	}
}

UNiagaraNodeFunctionCall* FNiagaraStackGraphUtilities::GetPreviousModuleNode(UNiagaraNodeFunctionCall& CurrentNode)
{
	UNiagaraNodeOutput* OutputNode = GetEmitterOutputNodeForStackNode(CurrentNode);
	if (OutputNode != nullptr)
	{
		TArray<UNiagaraNodeFunctionCall*> ModuleNodes;
		GetOrderedModuleNodes(*OutputNode, ModuleNodes);

		int32 ModuleIndex;
		ModuleNodes.Find(&CurrentNode, ModuleIndex);
		return ModuleIndex > 0 ? ModuleNodes[ModuleIndex - 1] : nullptr;
	}
	return nullptr;
}

UNiagaraNodeFunctionCall* FNiagaraStackGraphUtilities::GetNextModuleNode(UNiagaraNodeFunctionCall& CurrentNode)
{
	UNiagaraNodeOutput* OutputNode = GetEmitterOutputNodeForStackNode(CurrentNode);
	if (OutputNode != nullptr)
	{
		TArray<UNiagaraNodeFunctionCall*> ModuleNodes;
		GetOrderedModuleNodes(*OutputNode, ModuleNodes);

		int32 ModuleIndex;
		ModuleNodes.Find(&CurrentNode, ModuleIndex);
		return ModuleIndex < ModuleNodes.Num() - 2 ? ModuleNodes[ModuleIndex + 1] : nullptr;
	}
	return nullptr;
}

UNiagaraNodeOutput* FNiagaraStackGraphUtilities::GetEmitterOutputNodeForStackNode(UNiagaraNode& StackNode)
{
	TArray<UNiagaraNode*> NodesToCheck;
	NodesToCheck.Add(&StackNode);
	while (NodesToCheck.Num() > 0)
	{
		UNiagaraNode* NodeToCheck = NodesToCheck[0];
		NodesToCheck.RemoveAt(0);
		if (NodeToCheck->GetClass() == UNiagaraNodeOutput::StaticClass())
		{
			return CastChecked<UNiagaraNodeOutput>(NodeToCheck);
		}
		
		TArray<UEdGraphPin*> OutputPins;
		NodeToCheck->GetOutputPins(OutputPins);
		for (UEdGraphPin* OutputPin : OutputPins)
		{
			for (UEdGraphPin* LinkedPin : OutputPin->LinkedTo)
			{
				UNiagaraNode* LinkedNiagaraNode = Cast<UNiagaraNode>(LinkedPin->GetOwningNode());
				if (LinkedNiagaraNode != nullptr)
				{
					NodesToCheck.Add(LinkedNiagaraNode);
				}
			}
		}
	}
	return nullptr;
}

UNiagaraNodeInput* FNiagaraStackGraphUtilities::GetEmitterInputNodeForStackNode(UNiagaraNode& StackNode)
{
	// Since the stack graph can have arbitrary branches when traversing inputs, the only safe way to get the initial input
	// is to start at the output node and then trace only parameter map inputs.
	UNiagaraNodeOutput* OutputNode = GetEmitterOutputNodeForStackNode(StackNode);

	UNiagaraNode* PreviousNode = OutputNode;
	while (PreviousNode != nullptr)
	{
		UEdGraphPin* PreviousNodeInputPin = FNiagaraStackGraphUtilities::GetParameterMapInputPin(*PreviousNode);
		if (PreviousNodeInputPin != nullptr && PreviousNodeInputPin->LinkedTo.Num() == 1)
		{
			UNiagaraNode* CurrentNode = Cast<UNiagaraNode>(PreviousNodeInputPin->LinkedTo[0]->GetOwningNode());
			UNiagaraNodeInput* InputNode = Cast<UNiagaraNodeInput>(CurrentNode);
			if (InputNode != nullptr)
			{
				return InputNode;
			}
			PreviousNode = CurrentNode;
		}
		else
		{
			PreviousNode = nullptr;
		}
	}
	return nullptr;
}

void GetGroupNodesRecursive(const TArray<UNiagaraNode*>& CurrentStartNodes, UNiagaraNode* EndNode, TArray<UNiagaraNode*>& OutAllNodes)
{
	for (UNiagaraNode* CurrentStartNode : CurrentStartNodes)
	{
		if (OutAllNodes.Contains(CurrentStartNode) == false)
		{
			OutAllNodes.Add(CurrentStartNode);
			if (CurrentStartNode != EndNode)
			{
				TArray<UNiagaraNode*> LinkedNodes;
				TArray<UEdGraphPin*> OutputPins;
				CurrentStartNode->GetOutputPins(OutputPins);
				for (UEdGraphPin* OutputPin : OutputPins)
				{
					for (UEdGraphPin* LinkedPin : OutputPin->LinkedTo)
					{
						UNiagaraNode* LinkedNode = Cast<UNiagaraNode>(LinkedPin->GetOwningNode());
						if (LinkedNode != nullptr)
						{
							LinkedNodes.Add(LinkedNode);
						}
					}
				}
				GetGroupNodesRecursive(LinkedNodes, EndNode, OutAllNodes);
			}
		}
	}
}

void FNiagaraStackGraphUtilities::FStackNodeGroup::GetAllNodesInGroup(TArray<UNiagaraNode*>& OutAllNodes) const
{
	GetGroupNodesRecursive(StartNodes, EndNode, OutAllNodes);
}

void FNiagaraStackGraphUtilities::GetStackNodeGroups(UNiagaraNode& StackNode, TArray<FNiagaraStackGraphUtilities::FStackNodeGroup>& OutStackNodeGroups)
{
	UNiagaraNodeOutput* OutputNode = GetEmitterOutputNodeForStackNode(StackNode);
	if (OutputNode != nullptr)
	{
		UNiagaraNodeInput* InputNode = GetEmitterInputNodeForStackNode(*OutputNode);
		if (InputNode != nullptr)
		{
			FStackNodeGroup InputGroup;
			InputGroup.StartNodes.Add(InputNode);
			InputGroup.EndNode = InputNode;
			OutStackNodeGroups.Add(InputGroup);

			TArray<UNiagaraNodeFunctionCall*> ModuleNodes;
			GetOrderedModuleNodes(*OutputNode, ModuleNodes);
			for (UNiagaraNodeFunctionCall* ModuleNode : ModuleNodes)
			{
				FStackNodeGroup ModuleGroup;
				UEdGraphPin* PreviousOutputPin = GetParameterMapOutputPin(*OutStackNodeGroups.Last().EndNode);
				for (UEdGraphPin* PreviousOutputPinLinkedPin : PreviousOutputPin->LinkedTo)
				{
					ModuleGroup.StartNodes.Add(CastChecked<UNiagaraNode>(PreviousOutputPinLinkedPin->GetOwningNode()));
				}
				ModuleGroup.EndNode = ModuleNode;
				OutStackNodeGroups.Add(ModuleGroup);
			}

			FStackNodeGroup OutputGroup;
			UEdGraphPin* PreviousOutputPin = GetParameterMapOutputPin(*OutStackNodeGroups.Last().EndNode);
			for (UEdGraphPin* PreviousOutputPinLinkedPin : PreviousOutputPin->LinkedTo)
			{
				OutputGroup.StartNodes.Add(CastChecked<UNiagaraNode>(PreviousOutputPinLinkedPin->GetOwningNode()));
			}
			OutputGroup.EndNode = OutputNode;
			OutStackNodeGroups.Add(OutputGroup);
		}
	}
}

void FNiagaraStackGraphUtilities::DisconnectStackNodeGroup(const FStackNodeGroup& DisconnectGroup, const FStackNodeGroup& PreviousGroup, const FStackNodeGroup& NextGroup)
{
	UEdGraphPin* PreviousOutputPin = FNiagaraStackGraphUtilities::GetParameterMapOutputPin(*PreviousGroup.EndNode);
	PreviousOutputPin->BreakAllPinLinks();

	UEdGraphPin* DisconnectOutputPin = FNiagaraStackGraphUtilities::GetParameterMapOutputPin(*DisconnectGroup.EndNode);
	DisconnectOutputPin->BreakAllPinLinks();

	for (UNiagaraNode* NextStartNode : NextGroup.StartNodes)
	{
		UEdGraphPin* NextStartInputPin = FNiagaraStackGraphUtilities::GetParameterMapInputPin(*NextStartNode);
		PreviousOutputPin->MakeLinkTo(NextStartInputPin);
	}
}

void FNiagaraStackGraphUtilities::ConnectStackNodeGroup(const FStackNodeGroup& ConnectGroup, const FStackNodeGroup& NewPreviousGroup, const FStackNodeGroup& NewNextGroup)
{
	UEdGraphPin* NewPreviousOutputPin = FNiagaraStackGraphUtilities::GetParameterMapOutputPin(*NewPreviousGroup.EndNode);
	NewPreviousOutputPin->BreakAllPinLinks();

	for (UNiagaraNode* ConnectStartNode : ConnectGroup.StartNodes)
	{
		UEdGraphPin* ConnectInputPin = FNiagaraStackGraphUtilities::GetParameterMapInputPin(*ConnectStartNode);
		NewPreviousOutputPin->MakeLinkTo(ConnectInputPin);
	}

	UEdGraphPin* ConnectOutputPin = FNiagaraStackGraphUtilities::GetParameterMapOutputPin(*ConnectGroup.EndNode);

	for (UNiagaraNode* NewNextStartNode : NewNextGroup.StartNodes)
	{
		UEdGraphPin* NewNextStartInputPin = FNiagaraStackGraphUtilities::GetParameterMapInputPin(*NewNextStartNode);
		ConnectOutputPin->MakeLinkTo(NewNextStartInputPin);
	}
}

void FNiagaraStackGraphUtilities::InitializeDataInterfaceInputs(TSharedRef<FNiagaraSystemViewModel> SystemViewModel, TSharedRef<FNiagaraEmitterViewModel> EmitterViewModel, UNiagaraStackEditorData& StackEditorData, UNiagaraNodeFunctionCall& ModuleNode, UNiagaraNodeFunctionCall& InputFunctionCallNode)
{
	UNiagaraStackFunctionInputCollection* FunctionInputCollection = NewObject<UNiagaraStackFunctionInputCollection>(GetTransientPackage()); 
	FunctionInputCollection->Initialize(SystemViewModel, EmitterViewModel, StackEditorData, ModuleNode, InputFunctionCallNode, UNiagaraStackFunctionInputCollection::FDisplayOptions());
	FunctionInputCollection->RefreshChildren();

	TArray<UNiagaraStackEntry*> Children;
	FunctionInputCollection->GetUnfilteredChildren(Children);
	for (UNiagaraStackEntry* Child : Children)
	{
		UNiagaraStackFunctionInput* FunctionInput = Cast<UNiagaraStackFunctionInput>(Child);
		if (FunctionInput != nullptr && FunctionInput->GetInputType().GetClass() != nullptr)
		{
			FunctionInput->Reset();
		}
	}

	SystemViewModel->NotifyDataObjectChanged(nullptr);
}

FString FNiagaraStackGraphUtilities::GenerateStackFunctionInputEditorDataKey(UNiagaraNodeFunctionCall& FunctionCallNode, FNiagaraParameterHandle InputParameterHandle)
{
	return FunctionCallNode.GetFunctionName() + InputParameterHandle.GetParameterHandleString().ToString();
}

FString FNiagaraStackGraphUtilities::GenerateStackModuleEditorDataKey(UNiagaraNodeFunctionCall& ModuleNode)
{
	return ModuleNode.GetFunctionName();
}

void FNiagaraStackGraphUtilities::GetStackFunctionInputPins(UNiagaraNodeFunctionCall& FunctionCallNode, TArray<const UEdGraphPin*>& OutInputPins, ENiagaraGetStackFunctionInputPinsOptions Options, bool bIgnoreDisabled)
{
	FNiagaraParameterMapHistoryBuilder Builder;
	Builder.SetIgnoreDisabled(bIgnoreDisabled);
	FunctionCallNode.BuildParameterMapHistory(Builder, false);
	
	if (Builder.Histories.Num() == 1)
	{
		for (int32 i = 0; i < Builder.Histories[0].Variables.Num(); i++)
		{
			FNiagaraVariable& Variable = Builder.Histories[0].Variables[i];
			TArray<TTuple<const UEdGraphPin*, const UEdGraphPin*>>& ReadHistory = Builder.Histories[0].PerVariableReadHistory[i];

			// A read is only really exposed if it's the first read and it has no corresponding write.
			if (ReadHistory.Num() > 0 && ReadHistory[0].Get<1>() == nullptr)
			{
				const UEdGraphPin* InputPin = ReadHistory[0].Get<0>();
				if (Options == ENiagaraGetStackFunctionInputPinsOptions::AllInputs || FNiagaraParameterHandle(InputPin->PinName).IsModuleHandle())
				{
					OutInputPins.Add(InputPin);
				}
			}
		}
	}
}

UNiagaraNodeParameterMapSet* FNiagaraStackGraphUtilities::GetStackFunctionOverrideNode(UNiagaraNodeFunctionCall& FunctionCallNode)
{
	UEdGraphPin* ParameterMapInput = FNiagaraStackGraphUtilities::GetParameterMapInputPin(FunctionCallNode);
	if (ParameterMapInput != nullptr && ParameterMapInput->LinkedTo.Num() == 1)
	{
		return Cast<UNiagaraNodeParameterMapSet>(ParameterMapInput->LinkedTo[0]->GetOwningNode());
	}
	return nullptr;
}

UNiagaraNodeParameterMapSet& FNiagaraStackGraphUtilities::GetOrCreateStackFunctionOverrideNode(UNiagaraNodeFunctionCall& StackFunctionCall)
{
	UNiagaraNodeParameterMapSet* OverrideNode = GetStackFunctionOverrideNode(StackFunctionCall);
	if (OverrideNode == nullptr)
	{
		UEdGraph* Graph = StackFunctionCall.GetGraph();
		FGraphNodeCreator<UNiagaraNodeParameterMapSet> ParameterMapSetNodeCreator(*Graph);
		OverrideNode = ParameterMapSetNodeCreator.CreateNode();
		ParameterMapSetNodeCreator.Finalize();
		OverrideNode->SetEnabledState(StackFunctionCall.GetDesiredEnabledState(), StackFunctionCall.HasUserSetTheEnabledState());

		UEdGraphPin* OverrideNodeInputPin = FNiagaraStackGraphUtilities::GetParameterMapInputPin(*OverrideNode);
		UEdGraphPin* OverrideNodeOutputPin = FNiagaraStackGraphUtilities::GetParameterMapOutputPin(*OverrideNode);

		UEdGraphPin* OwningFunctionCallInputPin = FNiagaraStackGraphUtilities::GetParameterMapInputPin(StackFunctionCall);
		UEdGraphPin* PreviousStackNodeOutputPin = OwningFunctionCallInputPin->LinkedTo[0];

		OwningFunctionCallInputPin->BreakAllPinLinks();
		OwningFunctionCallInputPin->MakeLinkTo(OverrideNodeOutputPin);
		for (UEdGraphPin* PreviousStackNodeOutputLinkedPin : PreviousStackNodeOutputPin->LinkedTo)
		{
			PreviousStackNodeOutputLinkedPin->MakeLinkTo(OverrideNodeOutputPin);
		}
		PreviousStackNodeOutputPin->BreakAllPinLinks();
		PreviousStackNodeOutputPin->MakeLinkTo(OverrideNodeInputPin);
	}
	return *OverrideNode;
}

UEdGraphPin* FNiagaraStackGraphUtilities::GetStackFunctionInputOverridePin(UNiagaraNodeFunctionCall& StackFunctionCall, FNiagaraParameterHandle AliasedInputParameterHandle)
{
	UNiagaraNodeParameterMapSet* OverrideNode = GetStackFunctionOverrideNode(StackFunctionCall);
	if (OverrideNode != nullptr)
	{
		TArray<UEdGraphPin*> InputPins;
		OverrideNode->GetInputPins(InputPins);
		UEdGraphPin** OverridePinPtr = InputPins.FindByPredicate([&](const UEdGraphPin* Pin) { return Pin->PinName == AliasedInputParameterHandle.GetParameterHandleString(); });
		if (OverridePinPtr != nullptr)
		{
			return *OverridePinPtr;
		}
	}
	return nullptr;
}

UEdGraphPin& FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(UNiagaraNodeFunctionCall& StackFunctionCall, FNiagaraParameterHandle AliasedInputParameterHandle, FNiagaraTypeDefinition InputType)
{
	UEdGraphPin* OverridePin = GetStackFunctionInputOverridePin(StackFunctionCall, AliasedInputParameterHandle);
	if (OverridePin == nullptr)
	{
		UNiagaraNodeParameterMapSet& OverrideNode = GetOrCreateStackFunctionOverrideNode(StackFunctionCall);
		OverrideNode.Modify();

		TArray<UEdGraphPin*> OverrideInputPins;
		OverrideNode.GetInputPins(OverrideInputPins);

		const UEdGraphSchema_Niagara* NiagaraSchema = GetDefault<UEdGraphSchema_Niagara>();
		FEdGraphPinType PinType = NiagaraSchema->TypeDefinitionToPinType(InputType);
		OverridePin = OverrideNode.CreatePin(EEdGraphPinDirection::EGPD_Input, PinType, AliasedInputParameterHandle.GetParameterHandleString(), OverrideInputPins.Num() - 1);
	}
	return *OverridePin;
}

void FNiagaraStackGraphUtilities::RemoveNodesForStackFunctionInputOverridePin(UEdGraphPin& StackFunctionInputOverridePin)
{
	TArray<TWeakObjectPtr<UNiagaraDataInterface>> RemovedDataObjects;
	RemoveNodesForStackFunctionInputOverridePin(StackFunctionInputOverridePin, RemovedDataObjects);
}

void FNiagaraStackGraphUtilities::RemoveNodesForStackFunctionInputOverridePin(UEdGraphPin& StackFunctionInputOverridePin, TArray<TWeakObjectPtr<UNiagaraDataInterface>>& OutRemovedDataObjects)
{
	if (StackFunctionInputOverridePin.LinkedTo.Num() == 1)
	{
		UEdGraphNode* OverrideValueNode = StackFunctionInputOverridePin.LinkedTo[0]->GetOwningNode();
		UEdGraph* Graph = OverrideValueNode->GetGraph();
		if (OverrideValueNode->IsA<UNiagaraNodeInput>() || OverrideValueNode->IsA<UNiagaraNodeParameterMapGet>())
		{
			UNiagaraNodeInput* InputNode = Cast<UNiagaraNodeInput>(OverrideValueNode);
			if (InputNode != nullptr && InputNode->GetDataInterface() != nullptr)
			{
				OutRemovedDataObjects.Add(InputNode->GetDataInterface());
			}
			Graph->RemoveNode(OverrideValueNode);
		}
		else if (OverrideValueNode->IsA<UNiagaraNodeFunctionCall>())
		{
			UNiagaraNodeFunctionCall* DynamicInputNode = CastChecked<UNiagaraNodeFunctionCall>(OverrideValueNode);
			UEdGraphPin* DynamicInputNodeInputPin = FNiagaraStackGraphUtilities::GetParameterMapInputPin(*DynamicInputNode);
			UNiagaraNodeParameterMapSet* DynamicInputNodeOverrideNode = Cast<UNiagaraNodeParameterMapSet>(DynamicInputNodeInputPin->LinkedTo[0]->GetOwningNode());
			if (DynamicInputNodeOverrideNode != nullptr)
			{
				TArray<UEdGraphPin*> InputPins;
				DynamicInputNodeOverrideNode->GetInputPins(InputPins);
				for (UEdGraphPin* InputPin : InputPins)
				{
					if (InputPin->PinName.ToString().StartsWith(DynamicInputNode->GetFunctionName()))
					{
						RemoveNodesForStackFunctionInputOverridePin(*InputPin, OutRemovedDataObjects);
						DynamicInputNodeOverrideNode->RemovePin(InputPin);
					}
				}

				TArray<UEdGraphPin*> NewInputPins;
				DynamicInputNodeOverrideNode->GetInputPins(NewInputPins);
				if (NewInputPins.Num() == 2)
				{
					// If there are only 2 pins, they are the parameter map input and the add pin, so the dynamic input's override node can be removed.  This
					// not always be the case when removing dynamic input nodes because they share the same override node.
					UEdGraphPin* InputPin = FNiagaraStackGraphUtilities::GetParameterMapInputPin(*DynamicInputNodeOverrideNode);
					UEdGraphPin* OutputPin = FNiagaraStackGraphUtilities::GetParameterMapOutputPin(*DynamicInputNodeOverrideNode);

					if (ensureMsgf(InputPin != nullptr && InputPin->LinkedTo.Num() == 1 &&
						OutputPin != nullptr && OutputPin->LinkedTo.Num() == 2,
						TEXT("Invalid Stack - Dynamic input node override node not connected correctly.")))
					{
						// The DynamicInputOverrideNode will have a single input which is the previous module or override map set, and
						// two output links, one to the dynamic input node and one to the next override map set.
						UEdGraphPin* LinkedInputPin = InputPin->LinkedTo[0];
						UEdGraphPin* LinkedOutputPin = OutputPin->LinkedTo[0]->GetOwningNode() != DynamicInputNode
							? OutputPin->LinkedTo[0]
							: OutputPin->LinkedTo[1];
						InputPin->BreakAllPinLinks();
						OutputPin->BreakAllPinLinks();
						LinkedInputPin->MakeLinkTo(LinkedOutputPin);
						Graph->RemoveNode(DynamicInputNodeOverrideNode);
					}
				}
			}

			Graph->RemoveNode(DynamicInputNode);
		}
	}
}

void FNiagaraStackGraphUtilities::SetLinkedValueHandleForFunctionInput(UEdGraphPin& OverridePin, FNiagaraParameterHandle LinkedParameterHandle)
{
	checkf(OverridePin.LinkedTo.Num() == 0, TEXT("Can't set a linked value handle when the override pin already has a value."));

	UNiagaraNodeParameterMapSet* OverrideNode = CastChecked<UNiagaraNodeParameterMapSet>(OverridePin.GetOwningNode());
	UEdGraph* Graph = OverrideNode->GetGraph();
	FGraphNodeCreator<UNiagaraNodeParameterMapGet> GetNodeCreator(*Graph);
	UNiagaraNodeParameterMapGet* GetNode = GetNodeCreator.CreateNode();
	GetNodeCreator.Finalize();
	GetNode->SetEnabledState(OverrideNode->GetDesiredEnabledState(), OverrideNode->HasUserSetTheEnabledState());

	UEdGraphPin* GetInputPin = FNiagaraStackGraphUtilities::GetParameterMapInputPin(*GetNode);
	checkf(GetInputPin != nullptr, TEXT("Parameter map get node was missing it's parameter map input pin."));

	UEdGraphPin* OverrideNodeInputPin = FNiagaraStackGraphUtilities::GetParameterMapInputPin(*OverrideNode);
	UEdGraphPin* PreviousStackNodeOutputPin = OverrideNodeInputPin->LinkedTo[0];
	checkf(PreviousStackNodeOutputPin != nullptr, TEXT("Invalid Stack Graph - No previous stack node."));

	const UEdGraphSchema_Niagara* NiagaraSchema = GetDefault<UEdGraphSchema_Niagara>();
	FNiagaraTypeDefinition InputType = NiagaraSchema->PinToTypeDefinition(&OverridePin);
	UEdGraphPin* GetOutputPin = GetNode->RequestNewTypedPin(EGPD_Output, InputType, LinkedParameterHandle.GetParameterHandleString());
	GetInputPin->MakeLinkTo(PreviousStackNodeOutputPin);
	GetOutputPin->MakeLinkTo(&OverridePin);
}

void FNiagaraStackGraphUtilities::SetDataValueObjectForFunctionInput(UEdGraphPin& OverridePin, UClass* DataObjectType, FString DataObjectName, UNiagaraDataInterface*& OutDataObject)
{
	checkf(OverridePin.LinkedTo.Num() == 0, TEXT("Can't set a data value when the override pin already has a value."));
	checkf(DataObjectType->IsChildOf<UNiagaraDataInterface>(), TEXT("Can only set a function input to a data interface value object"));

	UNiagaraNodeParameterMapSet* OverrideNode = CastChecked<UNiagaraNodeParameterMapSet>(OverridePin.GetOwningNode());
	UEdGraph* Graph = OverrideNode->GetGraph();
	FGraphNodeCreator<UNiagaraNodeInput> InputNodeCreator(*Graph);
	UNiagaraNodeInput* InputNode = InputNodeCreator.CreateNode();
	FNiagaraEditorUtilities::InitializeParameterInputNode(*InputNode, FNiagaraTypeDefinition(DataObjectType), CastChecked<UNiagaraGraph>(Graph), *DataObjectName);

	OutDataObject = NewObject<UNiagaraDataInterface>(InputNode, DataObjectType, *DataObjectName, RF_Transactional);
	InputNode->SetDataInterface(OutDataObject);

	InputNodeCreator.Finalize();
	FNiagaraStackGraphUtilities::ConnectPinToInputNode(OverridePin, *InputNode);
}

void FNiagaraStackGraphUtilities::SetDynamicInputForFunctionInput(UEdGraphPin& OverridePin, UNiagaraScript* DynamicInput, UNiagaraNodeFunctionCall*& OutDynamicInputFunctionCall)
{
	checkf(OverridePin.LinkedTo.Num() == 0, TEXT("Can't set a data value when the override pin already has a value."));

	UNiagaraNodeParameterMapSet* OverrideNode = CastChecked<UNiagaraNodeParameterMapSet>(OverridePin.GetOwningNode());
	UEdGraph* Graph = OverrideNode->GetGraph();
	FGraphNodeCreator<UNiagaraNodeFunctionCall> FunctionCallNodeCreator(*Graph);
	UNiagaraNodeFunctionCall* FunctionCallNode = FunctionCallNodeCreator.CreateNode();
	FunctionCallNode->FunctionScript = DynamicInput;
	FunctionCallNodeCreator.Finalize();
	FunctionCallNode->SetEnabledState(OverrideNode->GetDesiredEnabledState(), OverrideNode->HasUserSetTheEnabledState());

	UEdGraphPin* FunctionCallInputPin = FNiagaraStackGraphUtilities::GetParameterMapInputPin(*FunctionCallNode);
	TArray<UEdGraphPin*> FunctionCallOutputPins;
	FunctionCallNode->GetOutputPins(FunctionCallOutputPins);

	const UEdGraphSchema_Niagara* NiagaraSchema = GetDefault<UEdGraphSchema_Niagara>();

	FNiagaraTypeDefinition InputType = NiagaraSchema->PinToTypeDefinition(&OverridePin);
	checkf(FunctionCallInputPin != nullptr, TEXT("Dynamic Input function call did not have a parameter map input pin."));
	checkf(FunctionCallOutputPins.Num() == 1 && NiagaraSchema->PinToTypeDefinition(FunctionCallOutputPins[0]) == InputType, TEXT("Invalid Stack Graph - Dynamic Input function did not have the correct typed output pin"));

	UEdGraphPin* OverrideNodeInputPin = FNiagaraStackGraphUtilities::GetParameterMapInputPin(*OverrideNode);
	UEdGraphPin* PreviousStackNodeOutputPin = OverrideNodeInputPin->LinkedTo[0];
	checkf(PreviousStackNodeOutputPin != nullptr, TEXT("Invalid Stack Graph - No previous stack node."));

	FunctionCallInputPin->MakeLinkTo(PreviousStackNodeOutputPin);
	FunctionCallOutputPins[0]->MakeLinkTo(&OverridePin);

	OutDynamicInputFunctionCall = FunctionCallNode;
}

bool FNiagaraStackGraphUtilities::RemoveModuleFromStack(UNiagaraNodeFunctionCall& ModuleNode)
{
	TArray<FNiagaraStackGraphUtilities::FStackNodeGroup> StackNodeGroups;
	FNiagaraStackGraphUtilities::GetStackNodeGroups(ModuleNode, StackNodeGroups);

	int32 ModuleStackIndex = StackNodeGroups.IndexOfByPredicate([&](const FNiagaraStackGraphUtilities::FStackNodeGroup& StackNodeGroup) { return StackNodeGroup.EndNode == &ModuleNode; });
	if (ModuleStackIndex == INDEX_NONE)
	{
		return false;
	}

	// Disconnect the group from the stack first to make collecting the nodes to remove easier.
	FNiagaraStackGraphUtilities::DisconnectStackNodeGroup(StackNodeGroups[ModuleStackIndex], StackNodeGroups[ModuleStackIndex - 1], StackNodeGroups[ModuleStackIndex + 1]);

	// Traverse all of the nodes in the group to find the nodes to remove.
	FNiagaraStackGraphUtilities::FStackNodeGroup ModuleGroup = StackNodeGroups[ModuleStackIndex];
	TArray<UNiagaraNode*> NodesToRemove;
	TArray<UNiagaraNode*> NodesToCheck;
	NodesToCheck.Add(ModuleGroup.EndNode);
	while (NodesToCheck.Num() > 0)
	{
		UNiagaraNode* NodeToRemove = NodesToCheck[0];
		NodesToCheck.RemoveAt(0);
		NodesToRemove.Add(NodeToRemove);

		TArray<UEdGraphPin*> InputPins;
		NodeToRemove->GetInputPins(InputPins);
		for (UEdGraphPin* InputPin : InputPins)
		{
			if (InputPin->LinkedTo.Num() == 1)
			{
				UNiagaraNode* LinkedNode = Cast<UNiagaraNode>(InputPin->LinkedTo[0]->GetOwningNode());
				if (LinkedNode != nullptr)
				{
					NodesToCheck.Add(LinkedNode);
				}
			}
		}
	}

	// Remove the nodes in the group from the graph.
	UNiagaraGraph* Graph = ModuleNode.GetNiagaraGraph();
	for (UNiagaraNode* NodeToRemove : NodesToRemove)
	{
		Graph->RemoveNode(NodeToRemove);
	}

	return true;
}

void ConnectModuleNode(UNiagaraNodeFunctionCall& ModuleNode, UNiagaraNodeOutput& TargetOutputNode, int32 TargetIndex)
{
	FNiagaraStackGraphUtilities::FStackNodeGroup ModuleGroup;
	ModuleGroup.StartNodes.Add(&ModuleNode);
	ModuleGroup.EndNode = &ModuleNode;

	TArray<FNiagaraStackGraphUtilities::FStackNodeGroup> StackNodeGroups;
	FNiagaraStackGraphUtilities::GetStackNodeGroups(TargetOutputNode, StackNodeGroups);
	checkf(StackNodeGroups.Num() >= 2, TEXT("Stack graph is invalid, can not connect module"));

	int32 InsertIndex;
	if (TargetIndex != INDEX_NONE)
	{
		// The first stack node group is always the input node so we add one to the target module index to get the insertion index.
		InsertIndex = FMath::Clamp(TargetIndex + 1, 1, StackNodeGroups.Num() - 1);
	}
	else
	{
		// If no insert index was specified, add the module at the end.
		InsertIndex = StackNodeGroups.Num() - 1;
	}

	FNiagaraStackGraphUtilities::FStackNodeGroup& TargetInsertGroup = StackNodeGroups[InsertIndex];
	FNiagaraStackGraphUtilities::FStackNodeGroup& TargetInsertPreviousGroup = StackNodeGroups[InsertIndex - 1];
	FNiagaraStackGraphUtilities::ConnectStackNodeGroup(ModuleGroup, TargetInsertPreviousGroup, TargetInsertGroup);
}

bool FNiagaraStackGraphUtilities::FindScriptModulesInStack(FAssetData ModuleScriptAsset, UNiagaraNodeOutput& TargetOutputNode, TArray<UNiagaraNodeFunctionCall*> OutFunctionCalls)
{
	UNiagaraGraph* Graph = TargetOutputNode.GetNiagaraGraph();
	TArray<UNiagaraNode*> Nodes;
	Graph->BuildTraversal(Nodes, &TargetOutputNode);

	OutFunctionCalls.Empty();
	FString ModuleObjectName = ModuleScriptAsset.ObjectPath.ToString();
	for (UEdGraphNode* Node : Nodes)
	{
		if (UNiagaraNodeFunctionCall* FunctionCallNode = Cast<UNiagaraNodeFunctionCall>(Node))
		{
			if (FunctionCallNode->FunctionScriptAssetObjectPath == ModuleScriptAsset.ObjectPath ||
				(FunctionCallNode->FunctionScript != nullptr && FunctionCallNode->FunctionScript->GetPathName() == ModuleObjectName))
			{
				OutFunctionCalls.Add(FunctionCallNode);
			}
		}
	}

	return OutFunctionCalls.Num() > 0;
}

UNiagaraNodeFunctionCall* FNiagaraStackGraphUtilities::AddScriptModuleToStack(FAssetData ModuleScriptAsset, UNiagaraNodeOutput& TargetOutputNode, int32 TargetIndex)
{
	UEdGraph* Graph = TargetOutputNode.GetGraph();
	Graph->Modify();

	FGraphNodeCreator<UNiagaraNodeFunctionCall> ModuleNodeCreator(*Graph);
	UNiagaraNodeFunctionCall* NewModuleNode = ModuleNodeCreator.CreateNode();
	NewModuleNode->FunctionScriptAssetObjectPath = ModuleScriptAsset.ObjectPath;
	ModuleNodeCreator.Finalize();

	ConnectModuleNode(*NewModuleNode, TargetOutputNode, TargetIndex);
	return NewModuleNode;
}

UNiagaraNodeAssignment* FNiagaraStackGraphUtilities::AddParameterModuleToStack(FNiagaraVariable ParameterVariable, UNiagaraNodeOutput& TargetOutputNode, int32 TargetIndex, const FString* InDefaultValue)
{
	UEdGraph* Graph = TargetOutputNode.GetGraph();
	Graph->Modify();

	FGraphNodeCreator<UNiagaraNodeAssignment> AssignmentNodeCreator(*Graph);
	UNiagaraNodeAssignment* NewAssignmentNode = AssignmentNodeCreator.CreateNode();
	NewAssignmentNode->AssignmentTarget = ParameterVariable;
	if (InDefaultValue)
	{
		NewAssignmentNode->AssignmentDefaultValue = *InDefaultValue;
	}
	AssignmentNodeCreator.Finalize();

	ConnectModuleNode(*NewAssignmentNode, TargetOutputNode, TargetIndex);
	return NewAssignmentNode;
}

UNiagaraMaterialParameterNode* FNiagaraStackGraphUtilities::AddMaterialParameterModuleToStack(TSharedRef<FNiagaraEmitterViewModel>& EmitterViewModel, UNiagaraNodeOutput& TargetOutputNode, int32 TargetIndex)
{
	UEdGraph* Graph = TargetOutputNode.GetGraph();
	Graph->Modify();

	FGraphNodeCreator<UNiagaraMaterialParameterNode> MaterialParameterNodeCreator(*Graph);
	UNiagaraMaterialParameterNode* NewMaterialParameterNode = MaterialParameterNodeCreator.CreateNode();
	NewMaterialParameterNode->SetEmitter(EmitterViewModel->GetEmitter());
	MaterialParameterNodeCreator.Finalize();
	
	ConnectModuleNode(*NewMaterialParameterNode, TargetOutputNode, TargetIndex);
	return NewMaterialParameterNode;
}

void GetAllNodesForModule(UNiagaraNodeFunctionCall& ModuleFunctionCall, TArray<UNiagaraNode*>& ModuleNodes)
{
	TArray<FNiagaraStackGraphUtilities::FStackNodeGroup> StackNodeGroups;
	FNiagaraStackGraphUtilities::GetStackNodeGroups(ModuleFunctionCall, StackNodeGroups);

	int32 ThisGroupIndex = StackNodeGroups.IndexOfByPredicate([&](const FNiagaraStackGraphUtilities::FStackNodeGroup& Group) { return Group.EndNode == &ModuleFunctionCall; });
	checkf(ThisGroupIndex > 0 && ThisGroupIndex < StackNodeGroups.Num() - 1, TEXT("Stack graph is invalid"));

	TArray<UNiagaraNode*> AllGroupNodes;
	StackNodeGroups[ThisGroupIndex].GetAllNodesInGroup(ModuleNodes);
}

TOptional<bool> FNiagaraStackGraphUtilities::GetModuleIsEnabled(UNiagaraNodeFunctionCall& FunctionCallNode)
{
	TArray<UNiagaraNode*> AllModuleNodes;
	GetAllNodesForModule(FunctionCallNode, AllModuleNodes);
	bool bIsEnabled = AllModuleNodes[0]->IsNodeEnabled();
	for (int32 i = 1; i < AllModuleNodes.Num(); i++)
	{
		if (AllModuleNodes[i]->IsNodeEnabled() != bIsEnabled)
		{
			return TOptional<bool>();
		}
	}
	return TOptional<bool>(bIsEnabled);
}

void FNiagaraStackGraphUtilities::SetModuleIsEnabled(UNiagaraNodeFunctionCall& FunctionCallNode, bool bIsEnabled)
{
	TArray<UNiagaraNode*> ModuleNodes;
	GetAllNodesForModule(FunctionCallNode, ModuleNodes);
	for (UNiagaraNode* ModuleNode : ModuleNodes)
	{
		ModuleNode->SetEnabledState(bIsEnabled ? ENodeEnabledState::Enabled : ENodeEnabledState::Disabled, true);
	}
	FunctionCallNode.GetNiagaraGraph()->NotifyGraphNeedsRecompile();
}

bool FNiagaraStackGraphUtilities::ValidateGraphForOutput(UNiagaraGraph& NiagaraGraph, ENiagaraScriptUsage ScriptUsage, FGuid ScriptUsageId, FText& ErrorMessage)
{
	UNiagaraNodeOutput* OutputNode = NiagaraGraph.FindOutputNode(ScriptUsage, ScriptUsageId);
	if (OutputNode == nullptr)
	{
		ErrorMessage = LOCTEXT("ValidateNoOutputMessage", "Output node doesn't exist for script.");
		return false;
	}

	TArray<FStackNodeGroup> NodeGroups;
	GetStackNodeGroups(*OutputNode, NodeGroups);
	
	if (NodeGroups.Num() < 2 || NodeGroups[0].EndNode->IsA<UNiagaraNodeInput>() == false || NodeGroups.Last().EndNode->IsA<UNiagaraNodeOutput>() == false)
	{
		ErrorMessage = LOCTEXT("ValidateInvalidStackMessage", "Stack graph does not include an input node connected to an output node.");
		return false;
	}

	return true;
}

UNiagaraNodeOutput* FNiagaraStackGraphUtilities::ResetGraphForOutput(UNiagaraGraph& NiagaraGraph, ENiagaraScriptUsage ScriptUsage, FGuid ScriptUsageId)
{
	NiagaraGraph.Modify();
	UNiagaraNodeOutput* OutputNode = NiagaraGraph.FindOutputNode(ScriptUsage, ScriptUsageId);
	UEdGraphPin* OutputNodeInputPin = OutputNode != nullptr ? GetParameterMapInputPin(*OutputNode) : nullptr;
	if (OutputNode != nullptr && OutputNodeInputPin == nullptr)
	{
		NiagaraGraph.RemoveNode(OutputNode);
		OutputNode = nullptr;
	}

	if (OutputNode == nullptr)
	{
		FGraphNodeCreator<UNiagaraNodeOutput> OutputNodeCreator(NiagaraGraph);
		OutputNode = OutputNodeCreator.CreateNode();
		OutputNode->SetUsage(ScriptUsage);
		OutputNode->SetUsageId(ScriptUsageId);
		OutputNode->Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetParameterMapDef(), TEXT("Out")));
		OutputNodeCreator.Finalize();

		OutputNodeInputPin = GetParameterMapInputPin(*OutputNode);
	}
	else
	{
		OutputNode->Modify();
	}

	FNiagaraVariable InputVariable(FNiagaraTypeDefinition::GetParameterMapDef(), TEXT("InputMap"));
	FGraphNodeCreator<UNiagaraNodeInput> InputNodeCreator(NiagaraGraph);
	UNiagaraNodeInput* InputNode = InputNodeCreator.CreateNode();
	InputNode->Input = FNiagaraVariable(FNiagaraTypeDefinition::GetParameterMapDef(), TEXT("InputMap"));
	InputNode->Usage = ENiagaraInputNodeUsage::Parameter;
	InputNodeCreator.Finalize();

	UEdGraphPin* InputNodeOutputPin = GetParameterMapOutputPin(*InputNode);
	OutputNodeInputPin->BreakAllPinLinks();
	OutputNodeInputPin->MakeLinkTo(InputNodeOutputPin);

	if (ScriptUsage == ENiagaraScriptUsage::SystemSpawnScript || ScriptUsage == ENiagaraScriptUsage::SystemUpdateScript)
	{
		// TODO: Move the emitter node wrangling to a utility function instead of getting the typed outer here and creating a view model.
		UNiagaraSystem* System = NiagaraGraph.GetTypedOuter<UNiagaraSystem>();
		if (System != nullptr && System->GetEmitterHandles().Num() != 0)
		{
			TSharedRef<FNiagaraSystemScriptViewModel> SystemScriptViewModel = MakeShared<FNiagaraSystemScriptViewModel>(*System, nullptr);
			SystemScriptViewModel->RebuildEmitterNodes();
		}
	}

	return OutputNode;
}

const UNiagaraEmitter* FNiagaraStackGraphUtilities::GetBaseEmitter(UNiagaraEmitter& Emitter, UNiagaraSystem& OwningSystem)
{
	for(const FNiagaraEmitterHandle& Handle : OwningSystem.GetEmitterHandles())
	{ 
		if (Handle.GetInstance() == &Emitter)
		{
			if (Handle.GetSource() != nullptr && Handle.GetSource() != &Emitter)
			{
				return Handle.GetSource();
			}
			else
			{
				// If the source is null then it was deleted and if the source is the same as the emitter the owning
				// system is transient and the emitter doesn't have base.
				return nullptr;
			}
		}
	}
	return nullptr;
}

void GetFunctionNamesRecursive(UNiagaraNode* CurrentNode, TArray<UNiagaraNode*>& VisitedNodes, TArray<FString>& FunctionNames)
{
	if (VisitedNodes.Contains(CurrentNode) == false)
	{
		VisitedNodes.Add(CurrentNode);
		UNiagaraNodeFunctionCall* FunctionCall = Cast<UNiagaraNodeFunctionCall>(CurrentNode);
		if (FunctionCall != nullptr)
		{
			FunctionNames.Add(FunctionCall->GetFunctionName());
		}
		TArray<UEdGraphPin*> InputPins;
		CurrentNode->GetInputPins(InputPins);
		for (UEdGraphPin* InputPin : InputPins)
		{
			for (UEdGraphPin* LinkedPin : InputPin->LinkedTo)
			{
				UNiagaraNode* LinkedNode = Cast<UNiagaraNode>(LinkedPin->GetOwningNode());
				if (LinkedNode != nullptr)
				{
					GetFunctionNamesRecursive(LinkedNode, VisitedNodes, FunctionNames);
				}
			}
		}
	}
}

void GetFunctionNamesForOutputNode(UNiagaraNodeOutput& OutputNode, TArray<FString>& FunctionNames)
{
	TArray<UNiagaraNode*> VisitedNodes;
	GetFunctionNamesRecursive(&OutputNode, VisitedNodes, FunctionNames);
}

void FNiagaraStackGraphUtilities::CleanUpStaleRapidIterationParameters(UNiagaraScript& Script, UNiagaraEmitter& OwningEmitter)
{
	UNiagaraScriptSource* Source = CastChecked<UNiagaraScriptSource>(Script.GetSource());
	UNiagaraNodeOutput* OutputNode = Source->NodeGraph->FindOutputNode(Script.GetUsage(), Script.GetUsageId());
	if (OutputNode != nullptr)
	{
		TArray<FString> ValidFunctionCallNames;
		GetFunctionNamesForOutputNode(*OutputNode, ValidFunctionCallNames);
		TArray<FNiagaraVariable> RapidIterationParameters;
		Script.RapidIterationParameters.GetParameters(RapidIterationParameters);
		for (const FNiagaraVariable& RapidIterationParameter : RapidIterationParameters)
		{
			FString EmitterName;
			FString FunctionCallName;
			if (FNiagaraParameterMapHistory::TryGetEmitterAndFunctionCallNamesFromRapidIterationParameter(RapidIterationParameter, EmitterName, FunctionCallName))
			{
				if (EmitterName != OwningEmitter.GetUniqueEmitterName() || ValidFunctionCallNames.Contains(FunctionCallName) == false)
				{
					Script.RapidIterationParameters.RemoveParameter(RapidIterationParameter);
				}
			}
		}
	}
}

void FNiagaraStackGraphUtilities::CleanUpStaleRapidIterationParameters(UNiagaraEmitter& Emitter)
{
	TArray<UNiagaraScript*> Scripts;
	Emitter.GetScripts(Scripts, false);
	for (UNiagaraScript* Script : Scripts)
	{
		CleanUpStaleRapidIterationParameters(*Script, Emitter);
	}
}

#undef LOCTEXT_NAMESPACE