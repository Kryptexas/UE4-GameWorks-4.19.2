// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BehaviorTree/BTNode.h"
#include "BTCompositeNode.generated.h"

DECLARE_DELEGATE_RetVal_ThreeParams(int32, FGetNextChildDelegate, struct FBehaviorTreeSearchData& /*search data*/, int32 /*last child index*/, EBTNodeResult::Type /*last result*/);

struct FBTCompositeMemory
{
	/** index of currently active child node */
	int8 CurrentChild;

	/** child override for next selection */
	int8 OverrideChild;
};

UENUM()
enum class EBTChildIndex
{
	FirstNode,
	TaskNode,
};

UENUM()
namespace EBTDecoratorLogic
{
	// keep in sync with DescribeLogicOp() in BTCompositeNode.cpp
	enum Type
	{
		Invalid,
		Test,		// test decorator conditions
		And,		// logic op: AND
		Or,			// logic op: OR
		Not,		// logic op: NOT
	};
}

USTRUCT()
struct FBTDecoratorLogic
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TEnumAsByte<EBTDecoratorLogic::Type> Operation;

	UPROPERTY()
	uint16 Number;

	FBTDecoratorLogic() : Operation(EBTDecoratorLogic::Invalid) {}
	FBTDecoratorLogic(uint8 InOperation, uint16 InNumber) : Operation(InOperation), Number(InNumber) {}
};

USTRUCT()
struct FBTCompositeChild
{
	GENERATED_USTRUCT_BODY()

	/** child node */
	UPROPERTY()
	class UBTCompositeNode* ChildComposite;

	UPROPERTY()
	class UBTTaskNode* ChildTask;

	/** execution decorators */
	UPROPERTY()
	TArray<class UBTDecorator*> Decorators;

	/** logic operations for decorators */
	UPROPERTY()
	TArray<FBTDecoratorLogic> DecoratorOps;
};

UCLASS(Abstract)
class AIMODULE_API UBTCompositeNode : public UBTNode
{
	GENERATED_UCLASS_BODY()

	/** child nodes */
	UPROPERTY()
	TArray<FBTCompositeChild> Children;

	/** service nodes */
	UPROPERTY()
	TArray<class UBTService*> Services;

	/** delegate for finding next child to execute */
	FGetNextChildDelegate OnNextChild;

	/** fill in data about tree structure */
	void InitializeComposite(uint16 InLastExecutionIndex);

	/** find next child branch to execute */
	int32 FindChildToExecute(struct FBehaviorTreeSearchData& SearchData, EBTNodeResult::Type& LastResult) const;

	/** get index of child node (handle subtrees) */
	int32 GetChildIndex(struct FBehaviorTreeSearchData& SearchData, const class UBTNode* ChildNode) const;
	/** get index of child node */
	int32 GetChildIndex(const class UBTNode* ChildNode) const;

	/** called before passing search to child node */
	void OnChildActivation(struct FBehaviorTreeSearchData& SearchData, const class UBTNode* ChildNode) const;
	void OnChildActivation(struct FBehaviorTreeSearchData& SearchData, int32 ChildIndex) const;

	/** called after child has finished search */
	void OnChildDeactivation(struct FBehaviorTreeSearchData& SearchData, const class UBTNode* ChildNode, EBTNodeResult::Type& NodeResult) const;
	void OnChildDeactivation(struct FBehaviorTreeSearchData& SearchData, int32 ChildIndex, EBTNodeResult::Type& NodeResult) const;

	/** called when start enters this node */
	void OnNodeActivation(struct FBehaviorTreeSearchData& SearchData) const;

	/** called when search leaves this node */
	void OnNodeDeactivation(struct FBehaviorTreeSearchData& SearchData, EBTNodeResult::Type& NodeResult) const;

	/** called when search needs to reactivate this node */
	void OnNodeRestart(struct FBehaviorTreeSearchData& SearchData) const;

	/** notify about task execution start */
	void ConditionalNotifyChildExecution(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, const class UBTNode* ChildNode, EBTNodeResult::Type& NodeResult) const;

	/** size of instance memory */
	virtual uint16 GetInstanceMemorySize() const override;

	/** @return child node at given index */
	class UBTNode* GetChildNode(int32 Index) const;

	/** @return children count */
	int32 GetChildrenNum() const;

	/** @return execution index of child node */
	uint16 GetChildExecutionIndex(int32 Index, EBTChildIndex ChildMode = EBTChildIndex::TaskNode) const;

	/** @return execution index of last node in child branches */
	uint16 GetLastExecutionIndex() const;

	/** set override for next child index */
	void SetChildOverride(FBehaviorTreeSearchData& SearchData, int8 Index) const;

	/** gathers description of all runtime parameters */
	virtual void DescribeRuntimeValues(const class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const override;

	/** check if child node can execute new subtree */
	virtual bool CanPushSubtree(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, int32 ChildIdx) const;

#if WITH_EDITOR
	/** @return allowed flow abort modes for decorators */
	virtual bool CanAbortLowerPriority() const;
	virtual bool CanAbortSelf() const;
#endif // WITH_EDITOR

	/** find branch containing specified node index */
	int32 GetMatchingChildIndex(int32 ActiveInstanceIdx, struct FBTNodeIndex& NodeIdx) const;

	/** is child execution allowed by decorators? */
	bool DoDecoratorsAllowExecution(class UBehaviorTreeComponent* OwnerComp, int32 InstanceIdx, int32 ChildIdx) const;

protected:

	/** execution index of last node in child branches */
	uint16 LastExecutionIndex;

	/** if set, NotifyChildExecution will be called */
	uint8 bUseChildExecutionNotify : 1;

	/** if set, NotifyNodeActivation will be called */
	uint8 bUseNodeActivationNotify : 1;

	/** if set, NotifyNodeDeactivation will be called */
	uint8 bUseNodeDeactivationNotify : 1;

	/** called just after child execution, allows to modify result */
	virtual void NotifyChildExecution(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, int32 ChildIdx, EBTNodeResult::Type& NodeResult) const;

	/** called when start enters this node */
	virtual void NotifyNodeActivation(struct FBehaviorTreeSearchData& SearchData) const;

	/** called when start leaves this node */
	virtual void NotifyNodeDeactivation(struct FBehaviorTreeSearchData& SearchData, EBTNodeResult::Type& NodeResult) const;

	/** runs through decorators on given child node and notify them about activation */
	void NotifyDecoratorsOnActivation(struct FBehaviorTreeSearchData& SearchData, int32 ChildIdx) const;

	/** runs through decorators on given child node and notify them about deactivation */
	void NotifyDecoratorsOnDeactivation(struct FBehaviorTreeSearchData& SearchData, int32 ChildIdx, EBTNodeResult::Type& NodeResult) const;

	/** runs through decorators on given child node and notify them about failed activation */
	void NotifyDecoratorsOnFailedActivation(struct FBehaviorTreeSearchData& SearchData, int32 ChildIdx, EBTNodeResult::Type& NodeResult) const;

	/** get next child to process and store it in CurrentChild */
	int32 GetNextChild(struct FBehaviorTreeSearchData& SearchData, int32 LastChildIdx, EBTNodeResult::Type LastResult) const;

	/** store delayed execution request */
	void RequestDelayedExecution(class UBehaviorTreeComponent* OwnerComp, EBTNodeResult::Type LastResult) const;
};


//////////////////////////////////////////////////////////////////////////
// Inlines

FORCEINLINE class UBTNode* UBTCompositeNode::GetChildNode(int32 Index) const
{
	return Children.IsValidIndex(Index) ?
		(Children[Index].ChildComposite ?
			(UBTNode*)Children[Index].ChildComposite :
			(UBTNode*)Children[Index].ChildTask) :
		NULL;
}

FORCEINLINE int32 UBTCompositeNode::GetChildrenNum() const
{
	return Children.Num();
}

FORCEINLINE uint16 UBTCompositeNode::GetLastExecutionIndex() const
{
	return LastExecutionIndex;
}
