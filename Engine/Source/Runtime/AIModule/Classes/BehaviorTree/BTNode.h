// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BehaviorTreeComponent.h"
#include "BehaviorTreeTypes.h"
#include "BTNode.generated.h"

struct FBTInstancedNodeMemory
{
	int32 NodeIdx;
};

UCLASS(Abstract,config=Game)
class AIMODULE_API UBTNode : public UObject
{
	GENERATED_UCLASS_BODY()

	virtual class UWorld* GetWorld() const override;

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

	/** called when node instance is added to tree */
	virtual void OnInstanceCreated(class UBehaviorTreeComponent* OwnerComp);

	/** called when node instance is removed from tree */
	virtual void OnInstanceDestroyed(class UBehaviorTreeComponent* OwnerComp);

	/** initialize memory and instancing */
	void InitializeForInstance(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, int32& NextInstancedIndex) const;

	/** size of special, hidden memory block for internal mechanics */
	virtual uint16 GetSpecialMemorySize() const;

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

	/** get special memory block used for hidden shared data (e.g. node instancing) */
	template<typename T>
	T* GetSpecialNodeMemory(uint8* NodeMemory) const;

	/** @return parent node */
	class UBTCompositeNode* GetParentNode() const;

	/** @return name of node */
	FString GetNodeName() const;

	/** @return execution index */
	uint16 GetExecutionIndex() const;

	/** @return memory offset */
	uint16 GetMemoryOffset() const;

	/** @return depth in tree */
	uint8 GetTreeDepth() const;

	/** sets bIsInjected flag, do NOT call this function unless you really know what you are doing! */
	void MarkInjectedNode();

	/** @return true if node was injected by subtree */
	bool IsInjected() const;

	/** sets bCreateNodeInstance flag, do NOT call this function on already pushed tree instance! */
	void ForceInstancing(bool bEnable);

	/** @return true if node wants to be instanced */
	bool HasInstance() const;

	/** @return true if this object is instanced node */
	bool IsInstanced() const;

	/** @return tree asset */
	class UBehaviorTree* GetTreeAsset() const;

	/** @return blackboard asset */
	class UBlackboardData* GetBlackboardAsset() const;

	/** @return node instance if bCreateNodeInstance was set */
	class UBTNode* GetNodeInstance(const class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const;
	class UBTNode* GetNodeInstance(struct FBehaviorTreeSearchData& SearchData) const;

	/** @return string containing description of this node instance with all relevant runtime values */
	FString GetRuntimeDescription(const class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity) const;

	/** @return string containing description of this node with all setup values */
	virtual FString GetStaticDescription() const;

#if WITH_EDITOR
	/** Get the name of the icon used to display this node in the editor */
	virtual FName GetNodeIconName() const;

	/** Get whether this node is using a blueprint for its logic */
	virtual bool UsesBlueprint() const;
#endif

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

protected:

	/** if set, node will be instanced instead of using memory block and template shared with all other BT components */
	uint8 bCreateNodeInstance : 1;

	/** set automatically for node instances */
	uint8 bIsInstanced : 1;

	/** if set, node is injected by subtree */
	uint8 bIsInjected : 1;
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

FORCEINLINE void UBTNode::MarkInjectedNode()
{
	bIsInjected = true;
}

FORCEINLINE bool UBTNode::IsInjected() const
{
	return bIsInjected;
}

FORCEINLINE void UBTNode::ForceInstancing(bool bEnable)
{
	// allow only in not initialized trees, side effect: root node always blocked
	check(ParentNode == NULL);

	bCreateNodeInstance = bEnable;
}

FORCEINLINE bool UBTNode::HasInstance() const
{
	return bCreateNodeInstance;
}

FORCEINLINE bool UBTNode::IsInstanced() const
{
	return bIsInstanced;
}

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

template<typename T>
T* UBTNode::GetSpecialNodeMemory(uint8* NodeMemory) const
{
	return (T*)(NodeMemory - ((GetSpecialMemorySize() + 3) & ~3));
}
