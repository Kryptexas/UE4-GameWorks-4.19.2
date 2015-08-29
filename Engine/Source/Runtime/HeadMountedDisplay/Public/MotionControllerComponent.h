// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "IMotionController.h"
#include "MotionControllerComponent.generated.h"

UCLASS(MinimalAPI, meta = (BlueprintSpawnableComponent), ClassGroup = MotionController)
class UMotionControllerComponent : public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()

	/** Which player index this motion controller should automatically follow */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MotionController")
	int32 PlayerIndex;

	/** Which hand this component should automatically follow */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MotionController")
	TEnumAsByte<EControllerHand> Hand;

	void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

	/** Whether or not this component had a valid tracked device this frame */
	UFUNCTION(BlueprintPure, Category = "MotionController")
	bool IsTracked() const
	{
		return bTracked;
	}

private:
	/** Whether or not this component had a valid tracked controller associated with it this frame*/
	bool bTracked;
};
