// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "LeapMotionPublicPCH.h"
#include "LeapGesture.h"
#include "LeapSwipeGesture.generated.h"

//API Reference: https://developer.leapmotion.com/documentation/cpp/api/Leap.SwipeGesture.html

UCLASS(BlueprintType)
class ULeapSwipeGesture : public ULeapGesture
{
	GENERATED_UCLASS_BODY()
public:
	~ULeapSwipeGesture();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Swipe Gesture")
	TEnumAsByte<LeapBasicDirection> BasicDirection;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Swipe Gesture")
	FVector Direction;

	UFUNCTION(BlueprintCallable, Category = "Leap Swipe Gesture")
	class ULeapPointable* Pointable();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Swipe Gesture")
	FVector Position;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Swipe Gesture")
	float Speed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Swipe Gesture")
	FVector StartPosition;
	
	void SetGesture(const class Leap::SwipeGesture &Gesture);

private:
	class FPrivateSwipeGesture* Private;

	UPROPERTY()
	ULeapPointable* PPointable = nullptr;
};