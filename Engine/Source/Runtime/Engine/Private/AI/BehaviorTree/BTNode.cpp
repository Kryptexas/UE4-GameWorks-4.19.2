// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

//----------------------------------------------------------------------//
// UBTNode
//----------------------------------------------------------------------//

UBTNode::UBTNode(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	NodeName = "";
	ParentNode = NULL;
	TreeAsset = NULL;
	ExecutionIndex = 0;
	MemoryOffset = 0;
	TreeDepth = 0;

#if USE_BEHAVIORTREE_DEBUGGER
	NextExecutionNode = NULL;
#endif
}

UWorld* UBTNode::GetWorld() const
{
	return Cast<UWorld>(GetOuter()->GetOuter());
}

void UBTNode::InitializeNode(class UBTCompositeNode* InParentNode, uint16 InExecutionIndex, uint16 InMemoryOffset, uint8 InTreeDepth)
{
	ParentNode = InParentNode;
	ExecutionIndex = InExecutionIndex;
	MemoryOffset = InMemoryOffset;
	TreeDepth = InTreeDepth;
}

void UBTNode::InitializeMemory(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	// empty in base class 
}

#if USE_BEHAVIORTREE_DEBUGGER
void UBTNode::InitializeExecutionOrder(class UBTNode* NextNode)
{
	NextExecutionNode = NextNode;
}
#endif

void UBTNode::InitializeFromAsset(class UBehaviorTree* Asset)
{
	TreeAsset = Asset;
}

UBlackboardData* UBTNode::GetBlackboardAsset() const
{
	return TreeAsset ? TreeAsset->BlackboardAsset : NULL;
}

uint16 UBTNode::GetInstanceMemorySize() const
{
	return 0;
}

FString UBTNode::GetStaticDescription() const
{
	// short type name
	return UBehaviorTreeTypes::GetShortTypeName(this);
}

FString UBTNode::GetRuntimeDescription(const class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity) const
{
	FString Description = NodeName.Len() ? FString::Printf(TEXT("%s [%s]"), *NodeName, *GetStaticDescription()) : GetStaticDescription();

	TArray<FString> RuntimeValues;
	DescribeRuntimeValues(OwnerComp, NodeMemory, Verbosity, RuntimeValues);
	for (int32 i = 0; i < RuntimeValues.Num(); i++)
	{
		Description += TEXT(", ");
		Description += RuntimeValues[i];
	}

	return Description;
}

void UBTNode::DescribeRuntimeValues(const class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const
{
	// nothing stored in memory for base class
}

void UBTNode::StartUsingExternalEvent(AActor* OwningActor)
{
	// will be implemented by blueprint based nodes
}

void UBTNode::StopUsingExternalEvent()
{
	// will be implemented by blueprint based nodes
}
