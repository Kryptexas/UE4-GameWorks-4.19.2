// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BehaviorTreeEditorPrivatePCH.h"

UBehaviorTreeGraphNode_Decorator::UBehaviorTreeGraphNode_Decorator(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UBehaviorTreeGraphNode_Decorator::AllocateDefaultPins()
{
	//No Pins for decorators
}

FString UBehaviorTreeGraphNode_Decorator::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	const UBTDecorator* Decorator = Cast<UBTDecorator>(NodeInstance);
	if (Decorator != NULL)
	{
		return Decorator->GetNodeName();
	}

	return Super::GetNodeTitle(TitleType);
}

FName UBehaviorTreeGraphNode_Decorator::GetNameIcon() const
{
	return FName("BTEditor.Graph.BTNode.Decorator.Icon");
}

bool UBehaviorTreeGraphNode_Decorator::IsSubNode() const
{
	return true;
}

void UBehaviorTreeGraphNode_Decorator::CollectDecoratorData(TArray<UBTDecorator*>& NodeInstances, TArray<FBTDecoratorLogic>& Operations) const
{
	if (NodeInstance)
	{
		UBTDecorator* DecoratorNode = (UBTDecorator*)NodeInstance;
		const int32 InstanceIdx = NodeInstances.Add(DecoratorNode);
		Operations.Add(FBTDecoratorLogic(EBTDecoratorLogic::Test, InstanceIdx));
	}
}
