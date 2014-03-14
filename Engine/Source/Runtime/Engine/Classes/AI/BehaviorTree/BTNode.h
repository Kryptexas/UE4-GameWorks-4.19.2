// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BehaviorTreeComponent.h"
#include "BehaviorTreeTypes.h"
#include "BTNode.generated.h"

UCLASS(Abstract,config=Game)
class ENGINE_API UBTNode : public UObject
{
	GENERATED_UCLASS_BODY()

	virtual class UWorld* GetWorld() const OVERRIDE;

	/** fill in data about tree structure */
	void InitializeNode(class UBTCompositeNode* InParentNode, uint16 InExecutionIndex, uint16 InMemoryOffset, uint8 InTreeDepth);

	/** initialize any asset related data */
	virtual void InitializeFromAsset(class UBehaviorTree* Asset);
	
	/** initialize memory in instance */
	virtual void InitializeMemory(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const;

	/** gathers description of all runtime parameters */
	virtual void DescribeRuntimeValues(const class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const;

	/** size of instance memory */
	virtual uint16 GetInstanceMemorySize() const;

#if USE_BEHAVIORTREE_DEBUGGER
	/** fill in data about execution order */
	void InitializeExecutionOrder(class UBTNode* NextNode);

	/** @return next node in execution order */
	class UBTNode* GetNextNode() const;
#endif

	template<typename T>
	T* GetNodeMemory(struct FBehaviorTreeSearchData& SearchData) const;

	template<typename T>
	const T* GetNodeMemory(const struct FBehaviorTreeSearchData& SearchData) const;

	template<typename T>
	T* GetNodeMemory(struct FBehaviorTreeInstance& BTInstance) const;

	template<typename T>
	const T* GetNodeMemory(const struct FBehaviorTreeInstance& BTInstance) const;

	/** @return parent node */
	class UBTCompositeNode* GetParentNode() const;

	/** @return name of node */
	FString GetNodeName() const;

	/** @return execution index */
	uint16 GetExecutionIndex() const;

	/** @return mmeory offset */
	uint16 GetMemoryOffset() const;

	/** @return depth in tree */
	uint8 GetTreeDepth() const;

	/** get tree asset */
	class UBehaviorTree* GetTreeAsset() const;

	/** get blackboard asset */
	class UBlackboardData* GetBlackboardAsset() const;

	/** @return string containing description of this node with all setup values */
	virtual FString GetStaticDescription() const;

	/** @return string containing description of this node instance with all relevant runtime values */
	FString GetRuntimeDescription(const class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity) const;

	/** Used to retrieve "current blueprint call owner" in a single-threaded fashion */
	virtual class UBehaviorTreeComponent* GetCurrentCallOwner() const;

	/** hook for blueprint based nodes receiving external event */
	virtual void StartUsingExternalEvent(AActor* OwningActor);

	/** hook for blueprint based nodes receiving external event */
	virtual void StopUsingExternalEvent();

protected:

	/** node name */
	UPROPERTY(Category=Description, EditInstanceOnly)
	FString NodeName;

private:

	/** source asset */
	UPROPERTY()
	class UBehaviorTree* TreeAsset;

	/** parent node */
	UPROPERTY()
	class UBTCompositeNode* ParentNode;

#if USE_BEHAVIORTREE_DEBUGGER
	/** next node in execution order */
	class UBTNode* NextExecutionNode;
#endif

	/** depth first index (execution order) */
	uint16 ExecutionIndex;

	/** instance memory offset */
	uint16 MemoryOffset;

	/** depth in tree */
	uint8 TreeDepth;
};

//////////////////////////////////////////////////////////////////////////
// Inlines

FORCEINLINE class UBehaviorTree* UBTNode::GetTreeAsset() const
{
	return TreeAsset;
}

FORCEINLINE class UBTCompositeNode* UBTNode::GetParentNode() const
{
	return ParentNode;
}

#if USE_BEHAVIORTREE_DEBUGGER
FORCEINLINE class UBTNode* UBTNode::GetNextNode() const
{
	return NextExecutionNode;
}
#endif

FORCEINLINE FString UBTNode::GetNodeName() const
{
	return NodeName.Len() ? NodeName : GetClass()->GetName();
}

FORCEINLINE uint16 UBTNode::GetExecutionIndex() const
{
	return ExecutionIndex;
}

FORCEINLINE uint16 UBTNode::GetMemoryOffset() const
{
	return MemoryOffset;
}

FORCEINLINE uint8 UBTNode::GetTreeDepth() const
{
	return TreeDepth;
}

FORCEINLINE class UBehaviorTreeComponent* UBTNode::GetCurrentCallOwner() const
{
	return NULL;
};

template<typename T>
T* UBTNode::GetNodeMemory(struct FBehaviorTreeSearchData& SearchData) const
{
	return GetNodeMemory<T>(SearchData.OwnerComp->InstanceStack[SearchData.OwnerComp->GetActiveInstanceIdx()]);
}

template<typename T>
const T* UBTNode::GetNodeMemory(const struct FBehaviorTreeSearchData& SearchData) const
{
	return GetNodeMemory<T>(SearchData.OwnerComp->InstanceStack[SearchData.OwnerComp->GetActiveInstanceIdx()]);
}

template<typename T>
T* UBTNode::GetNodeMemory(struct FBehaviorTreeInstance& BTInstance) const
{
	return (T*)(BTInstance.InstanceMemory.GetTypedData() + MemoryOffset);
}

template<typename T>
const T* UBTNode::GetNodeMemory(const struct FBehaviorTreeInstance& BTInstance) const
{
	return (const T*)(BTInstance.InstanceMemory.GetTypedData() + MemoryOffset);
}

