// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "BehaviorTree/BTDecorator.h"
#include "GameplayTagContainer.h"
#include "BTDecorator_TagCooldown.generated.h"

struct FBTTagCooldownDecoratorMemory
{	
	uint8 bRequestedRestart : 1;
};

/**
 * Cooldown decorator node.
 * A decorator node that bases its condition on whether a cooldown timer based on a gameplay tag has expired.
 */
UCLASS(HideCategories=(Condition))
class AIMODULE_API UBTDecorator_TagCooldown : public UBTDecorator
{
	GENERATED_UCLASS_BODY()

	/** gameplay tag that will be used for the cooldown */
	UPROPERTY(Category = Decorator, EditAnywhere)
	FGameplayTag CooldownTag;

	/** how long the cooldown lasts */
	UPROPERTY(Category = Decorator, EditAnywhere, meta = (EditCondition = "bActivatesCooldown"))
	float CooldownDuration;

	/** true if we are adding to any existing duration, false if we are setting the duration (potentially invalidating an existing end time) */
	UPROPERTY(Category = Decorator, EditAnywhere, meta = (EditCondition = "bActivatesCooldown"))
	bool bAddToExistingDuration;

	/** whether or not we are setting the tag's value when we deactivate */
	UPROPERTY(Category = Decorator, EditAnywhere)
	bool bActivatesCooldown;

	// Begin UObject Interface
	virtual void PostLoad() override;
	// End UObject Interface

	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual void InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const override;
	virtual uint16 GetInstanceMemorySize() const override;
	virtual void DescribeRuntimeValues(const UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const override;
	virtual FString GetStaticDescription() const override;

#if WITH_EDITOR
	virtual FName GetNodeIconName() const override;
#endif // WITH_EDITOR

protected:

	virtual void OnNodeDeactivation(FBehaviorTreeSearchData& SearchData, EBTNodeResult::Type NodeResult) override;
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

private:
	bool HasCooldownFinished(const UBehaviorTreeComponent& OwnerComp) const;
};
