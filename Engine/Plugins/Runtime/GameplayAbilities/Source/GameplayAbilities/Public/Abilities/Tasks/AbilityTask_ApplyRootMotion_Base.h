// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "AbilityTask.h"
#include "AbilityTask_ApplyRootMotion_Base.generated.h"

class UCharacterMovementComponent;
enum class ERootMotionFinishVelocityMode : uint8;

UCLASS(MinimalAPI)
class UAbilityTask_ApplyRootMotion_Base : public UAbilityTask
{
	GENERATED_UCLASS_BODY()

	virtual void InitSimulatedTask(UGameplayTasksComponent& InGameplayTasksComponent) override;

protected:

	virtual void SharedInitAndApply() {};

	bool HasTimedOut() const;

protected:

	UPROPERTY(Replicated)
	FName ForceName;

	/** What to do with character's Velocity when root motion finishes */
	UPROPERTY(Replicated)
	ERootMotionFinishVelocityMode FinishVelocityMode;

	/** If FinishVelocityMode mode is "SetVelocity", character velocity is set to this value when root motion finishes */
	UPROPERTY(Replicated)
	FVector FinishSetVelocity;

	/** If FinishVelocityMode mode is "ClampVelocity", character velocity is clamped to this value when root motion finishes */
	UPROPERTY(Replicated)
	float FinishClampVelocity;

	UPROPERTY()
	UCharacterMovementComponent* MovementComponent; 
	
	uint16 RootMotionSourceID;

	bool bIsFinished;

	float StartTime;
	float EndTime;
};