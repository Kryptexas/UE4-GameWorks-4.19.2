// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "BehaviorTree/BTDecorator.h"
#include "GameplayTagContainer.h"
#include "BTDecorator_SetTagCooldown.generated.h"

/**
 * Set tag cooldown decorator node.
 * A decorator node that sets a gameplay tag cooldown.
 */
UCLASS(HideCategories=(Condition))
class AIMODULE_API UBTDecorator_SetTagCooldown : public UBTDecorator
{
	GENERATED_UCLASS_BODY()

	/** gameplay tag that will be used for the set cooldown */
	UPROPERTY(Category = Decorator, EditAnywhere)
	FGameplayTag CooldownTag;

	/** how long the cooldown lasts */
	UPROPERTY(Category = Decorator, EditAnywhere)
	float CooldownDuration;

	/** true if we are adding to any existing duration, false if we are setting the duration (potentially invalidating an existing end time) */
	UPROPERTY(Category = Decorator, EditAnywhere)
	bool bAddToExistingDuration;

	virtual FString GetStaticDescription() const override;

#if WITH_EDITOR
	virtual FName GetNodeIconName() const override;
#endif // WITH_EDITOR

protected:
	virtual void OnNodeDeactivation(FBehaviorTreeSearchData& SearchData, EBTNodeResult::Type NodeResult) override;
};
