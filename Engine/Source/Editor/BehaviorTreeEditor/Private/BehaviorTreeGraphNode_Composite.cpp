// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BehaviorTreeEditorPrivatePCH.h"

#define LOCTEXT_NAMESPACE "BehaviorTreeGraphNode_Composite"

UBehaviorTreeGraphNode_Composite::UBehaviorTreeGraphNode_Composite(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

FString UBehaviorTreeGraphNode_Composite::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	const UBTNode* MyNode = Cast<UBTNode>(NodeInstance);
	if (MyNode != NULL)
	{
		return  MyNode->GetNodeName();
	}
	return Super::GetNodeTitle(TitleType);
}

FName UBehaviorTreeGraphNode_Composite::GetNameIcon() const
{
	return FName("BTEditor.Graph.BTNode.Composite.Icon");
}


void UBehaviorTreeGraphNode_Composite::GetContextMenuActions(const FGraphNodeContextMenuBuilder& Context) const
{
	AddContextMenuActionsDecorators(Context);
	AddContextMenuActionsServices(Context);
}

#undef LOCTEXT_NAMESPACE
