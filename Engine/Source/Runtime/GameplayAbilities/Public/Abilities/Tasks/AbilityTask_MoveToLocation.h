// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "AbilityTask.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "AbilityTask_MoveToLocation.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMoveToLocationDelegate);

class AActor;
class UPrimitiveComponent;

/**
*		Move to a location, ignoring clipping, over a given length of time. Callback whenever we pass through an actor and when we reach our target location.
*/
UCLASS(MinimalAPI)
class UAbilityTask_MoveToLocation : public UAbilityTask
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(BlueprintAssignable)
	FMoveToLocationDelegate		OnTargetLocationReached;

	void InterpolatePosition();

	/** Move to the specified location, using the curve (range 0 - 1) or linearly if no curve is specified */
	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject", BlueprintInternalUseOnly = "TRUE"))
	static UAbilityTask_MoveToLocation* MoveToLocation(UObject* WorldContextObject, FVector Location, float Duration, UCurveFloat* OptionalInterpolationCurve);

	virtual void Activate() override;

protected:

	virtual void OnDestroy(bool AbilityEnded) override;

	FVector StartLocation;
	FVector TargetLocation;
	float DurationOfMovement;
	float TimeMoveStarted;
	float TimeMoveWillEnd;
	TWeakObjectPtr<UCurveFloat> LerpCurve;
};