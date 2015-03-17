// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "BTTask_SetTagCooldown.generated.h"

/**
 * Cooldown task node.
 * Sets a cooldown tag value.  Use with cooldown tag decorators to prevent behavior tree execution.
 */
UCLASS()
class AIMODULE_API UBTTask_SetTagCooldown : public UBTTaskNode
{
	GENERATED_UCLASS_BODY()

	/** gameplay tag that will be used for the cooldown */
	UPROPERTY(Category = Cooldown, EditAnywhere)
	FGameplayTag CooldownTag;

	/** true if we are adding to any existing duration, false if we are setting the duration (potentially invalidating an existing end time) */
	UPROPERTY(Category = Decorator, EditAnywhere)
	bool bAddToExistingDuration;

	/** how long the cooldown lasts */
	UPROPERTY(Category = Decorator, EditAnywhere)
	float CooldownDuration;

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

#if WITH_EDITOR
	virtual FName GetNodeIconName() const override;
#endif // WITH_EDITOR
};
