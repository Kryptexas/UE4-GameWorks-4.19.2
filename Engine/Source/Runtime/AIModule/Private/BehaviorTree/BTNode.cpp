// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BTNode.h"
#include "BehaviorTree/BTCompositeNode.h"

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
	bCreateNodeInstance = false;
	bIsInstanced = false;
	bIsInjected = false;

#if USE_BEHAVIORTREE_DEBUGGER
	NextExecutionNode = NULL;
#endif
}

UWorld* UBTNode::GetWorld() const
{
	// instanced nodes are created for behavior tree component owning that instance
	// template nodes are created for behavior tree manager, which is located directly in UWorld

	return GetOuter() == NULL ? NULL :
		IsInstanced() ? (Cast<UBehaviorTreeComponent>(GetOuter()))->GetWorld() :
		Cast<UWorld>(GetOuter()->GetOuter());
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

void UBTNode::OnInstanceCreated(class UBehaviorTreeComponent* OwnerComp)
{
	// empty in base class
}

void UBTNode::OnInstanceDestroyed(class UBehaviorTreeComponent* OwnerComp)
{
	// empty in base class
}

void UBTNode::InitializeForInstance(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, int32& NextInstancedIndex) const
{
	if (bCreateNodeInstance)
	{
		// composite nodes can't be instanced!
		check(IsA(UBTCompositeNode::StaticClass()) == false);

		UBTNode* NodeInstance = OwnerComp->NodeInstances.IsValidIndex(NextInstancedIndex) ? OwnerComp->NodeInstances[NextInstancedIndex] : NULL;
		if (NodeInstance == NULL)
		{
			NodeInstance = DuplicateObject<UBTNode>(this, OwnerComp);
			NodeInstance->InitializeNode(GetParentNode(), GetExecutionIndex(), GetMemoryOffset(), GetTreeDepth());
			NodeInstance->bIsInstanced = true;

			OwnerComp->NodeInstances.Add(NodeInstance);
		}

		FBTInstancedNodeMemory* MyMemory = GetSpecialNodeMemory<FBTInstancedNodeMemory>(NodeMemory);
		MyMemory->NodeIdx = NextInstancedIndex;

		NodeInstance->OnInstanceCreated(OwnerComp);
		NextInstancedIndex++;
	}
	else
	{
		InitializeMemory(OwnerComp, NodeMemory);
	}
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

uint16 UBTNode::GetSpecialMemorySize() const
{
	return bCreateNodeInstance ? sizeof(FBTInstancedNodeMemory) : 0;
}

class UBTNode* UBTNode::GetNodeInstance(const class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	FBTInstancedNodeMemory* MyMemory = GetSpecialNodeMemory<FBTInstancedNodeMemory>(NodeMemory);
	return OwnerComp && MyMemory && OwnerComp->NodeInstances.IsValidIndex(MyMemory->NodeIdx) ?
		OwnerComp->NodeInstances[MyMemory->NodeIdx] : NULL;
}

class UBTNode* UBTNode::GetNodeInstance(struct FBehaviorTreeSearchData& SearchData) const
{
	return GetNodeInstance(SearchData.OwnerComp, GetNodeMemory<uint8>(SearchData));
}

FString UBTNode::GetRuntimeDescription(const class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity) const
{
	FString Description = NodeName.Len() ? FString::Printf(TEXT("%s [%s]"), *NodeName, *GetStaticDescription()) : GetStaticDescription();
	TArray<FString> RuntimeValues;

	const UBTNode* NodeOb = bCreateNodeInstance ? GetNodeInstance(OwnerComp, NodeMemory) : this;
	if (NodeOb)
	{
		NodeOb->DescribeRuntimeValues(OwnerComp, NodeMemory, Verbosity, RuntimeValues);
	}

	for (int32 ValueIndex = 0; ValueIndex < RuntimeValues.Num(); ValueIndex++)
	{
		Description += TEXT(", ");
		Description += RuntimeValues[ValueIndex];
	}

	return Description;
}

FString UBTNode::GetStaticDescription() const
{
	// short type name
	return UBehaviorTreeTypes::GetShortTypeName(this);
}

void UBTNode::DescribeRuntimeValues(const class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const
{
	// nothing stored in memory for base class
}

#if WITH_EDITOR

FName UBTNode::GetNodeIconName() const
{
	return NAME_None;
}

bool UBTNode::UsesBlueprint() const
{
	return false;
}

#endif