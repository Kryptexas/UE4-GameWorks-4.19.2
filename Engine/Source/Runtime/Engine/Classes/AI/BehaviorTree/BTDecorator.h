// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "BehaviorTreeTypes.h"
#include "BTDecorator.generated.h"

UCLASS(Abstract)
class ENGINE_API UBTDecorator : public UBTAuxiliaryNode
{
	GENERATED_UCLASS_BODY()

	/** fill in data about tree structure */
	void InitializeDecorator(uint8 InChildIndex);

	/** execution check wrapper, including inversed condition check */
	bool CanExecute(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const;

	/** called when underlying node is activated  */
	void ConditionalOnNodeActivation(struct FBehaviorTreeSearchData& SearchData) const;
	
	/** called when underlying node has finished */
	void ConditionalOnNodeDeactivation(struct FBehaviorTreeSearchData& SearchData, EBTNodeResult::Type NodeResult) const;

	/** called when underlying node was processed (before deactivation) */
	void ConditionalOnNodeProcessed(struct FBehaviorTreeSearchData& SearchData, EBTNodeResult::Type& NodeResult) const;

	/** @return decorated child */
	const class UBTNode* GetMyNode() const;

	/** @return index of child in parent's array */
	uint8 GetChildIndex() const;

	/** @return flow controller's abort mode */
	EBTFlowAbortMode::Type GetFlowAbortMode() const;

	/** @return true if restart self can result in EBTRestartRange::OnlyChildNodes range, false means that execution have to leave the node */
	bool IsRestartChildOnlyAllowed() const;

	/** @return true if condition should be inversed */
	bool IsInversed() const;

	virtual FString GetStaticDescription() const OVERRIDE;

	/** make sure that current flow abort mode can be used with parent composite */
	void ValidateFlowAbortMode();

protected:

	/** if set, FlowAbortMode can be set to None */
	uint32 bAllowAbortNone : 1;

	/** if set, FlowAbortMode can be set to LowerPriority and Both */
	uint32 bAllowAbortLowerPri : 1;

	/** if set, FlowAbortMode can be set to Self and Both */
	uint32 bAllowAbortChildNodes : 1;

	/** if set, OnNodeActivation will be used */
	uint32 bNotifyActivation : 1;

	/** if set, OnNodeDeactivation will be used */
	uint32 bNotifyDeactivation : 1;

	/** if set, OnNodeProcessed will be used */
	uint32 bNotifyProcessed : 1;

	/** if set, static description will include default description of inversed condition */
	uint32 bShowInverseConditionDesc : 1;

private:
	UPROPERTY(Category=Condition, EditAnywhere)
	uint32 bInverseCondition : 1;

protected:
	/** flow controller settings */
	UPROPERTY(Category=FlowControl, EditAnywhere)
	TEnumAsByte<EBTFlowAbortMode::Type> FlowAbortMode;

	/** child index in parent node */
	uint8 ChildIndex;

	void SetIsInversed(bool bShouldBeInversed);

	/** called when underlying node is activated  */
	virtual void OnNodeActivation(struct FBehaviorTreeSearchData& SearchData) const;

	/** called when underlying node has finished */
	virtual void OnNodeDeactivation(struct FBehaviorTreeSearchData& SearchData, EBTNodeResult::Type NodeResult) const;

	/** called when underlying node was processed (deactivated or failed to activate) */
	virtual void OnNodeProcessed(struct FBehaviorTreeSearchData& SearchData, EBTNodeResult::Type& NodeResult) const;

	/** calculates raw, core value of decorator's condition. Should not include calling IsInversed */
	virtual bool CalculateRawConditionValue(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const;

	friend class FBehaviorDecoratorDetails;
};


//////////////////////////////////////////////////////////////////////////
// Inlines

FORCEINLINE EBTFlowAbortMode::Type UBTDecorator::GetFlowAbortMode() const
{
	return FlowAbortMode;
}

FORCEINLINE bool UBTDecorator::IsRestartChildOnlyAllowed() const
{
	/** execution have to leave node only when decorator always work in abort self mode (e.g. TimeLimit) */
	return bAllowAbortNone || bAllowAbortLowerPri;
}

FORCEINLINE bool UBTDecorator::IsInversed() const
{
	return bInverseCondition;
}

FORCEINLINE uint8 UBTDecorator::GetChildIndex() const
{
	return ChildIndex;
}

FORCEINLINE bool UBTDecorator::CanExecute(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	// same as IsInversed ? !CheckRawCondition : CheckRawCondition
	return IsInversed() != CalculateRawConditionValue(OwnerComp, NodeMemory);
}
