// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BehaviorTreeEditorPrivatePCH.h"

UBehaviorTreeDecoratorGraphNode_Decorator::UBehaviorTreeDecoratorGraphNode_Decorator(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	NodeInstance = NULL;
}

void UBehaviorTreeDecoratorGraphNode_Decorator::AllocateDefaultPins()
{
	CreatePin(EGPD_Output, TEXT("Transition"), TEXT(""), NULL, false, false, TEXT("Out"));
}

void UBehaviorTreeDecoratorGraphNode_Decorator::PostPlacedNewNode()
{
	UClass* NodeClass = ClassData.GetClass();
	if (NodeClass != NULL)
	{
		UBehaviorTreeGraphNode_CompositeDecorator* OwningNode = Cast<UBehaviorTreeGraphNode_CompositeDecorator>(GetDecoratorGraph()->GetOuter());
		UBehaviorTree* BT = OwningNode ? Cast<UBehaviorTree>(OwningNode->GetOuter()->GetOuter()) : NULL;
		
		UBTDecorator* MyDecorator = ConstructObject<UBTDecorator>(NodeClass, BT);
		MyDecorator->InitializeFromAsset(BT);
		OwningNode->InitializeDecorator(MyDecorator);

		NodeInstance = MyDecorator;
	}
}

FString UBehaviorTreeDecoratorGraphNode_Decorator::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	const UBTNode* MyNode = Cast<UBTNode>(NodeInstance);
	if (MyNode != NULL)
	{
		return MyNode->GetStaticDescription();
	}

	return Super::GetNodeTitle(TitleType);
}

EBTDecoratorLogic::Type UBehaviorTreeDecoratorGraphNode_Decorator::GetOperationType() const
{
	return EBTDecoratorLogic::Test;
}

void UBehaviorTreeDecoratorGraphNode_Decorator::PrepareForCopying()
{
	if (NodeInstance)
	{
		// Temporarily take ownership of the node instance, so that it is not deleted when cutting
		NodeInstance->Rename(NULL, this, REN_DontCreateRedirectors | REN_DoNotDirty );
	}
}

void UBehaviorTreeDecoratorGraphNode_Decorator::PostEditImport()
{
	ResetNodeOwner();

	if (NodeInstance)
	{
		UBehaviorTreeGraphNode_CompositeDecorator* OwningNode = Cast<UBehaviorTreeGraphNode_CompositeDecorator>(GetDecoratorGraph()->GetOuter());
		UBehaviorTree* BT = OwningNode ? Cast<UBehaviorTree>(OwningNode->GetOuter()->GetOuter()) : NULL;

		UBTDecorator* MyDecorator = (UBTDecorator*)NodeInstance;
		MyDecorator->InitializeFromAsset(BT);
		MyDecorator->InitializeNode(NULL, MAX_uint16, 0, 0);

		OwningNode->InitializeDecorator(MyDecorator);
	}
}

void UBehaviorTreeDecoratorGraphNode_Decorator::PostCopyNode()
{
	ResetNodeOwner();
}

void UBehaviorTreeDecoratorGraphNode_Decorator::ResetNodeOwner()
{
	if (NodeInstance)
	{
		UBehaviorTreeGraphNode_CompositeDecorator* OwningNode = Cast<UBehaviorTreeGraphNode_CompositeDecorator>(GetDecoratorGraph()->GetOuter());
		UBehaviorTree* BT = OwningNode ? Cast<UBehaviorTree>(OwningNode->GetOuter()->GetOuter()) : NULL;

		NodeInstance->Rename(NULL, BT, REN_DontCreateRedirectors | REN_DoNotDirty);
	}
}
