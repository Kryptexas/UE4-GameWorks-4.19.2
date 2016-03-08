// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "LeapMotionPublicPCH.h"
#include "LeapGesture.h"
#include "LeapScreenTapGesture.generated.h"

//API Reference: https://developer.leapmotion.com/documentation/cpp/api/Leap.ScreenTapGesture.html

UCLASS(BlueprintType)
class ULeapScreenTapGesture : public ULeapGesture
{
	GENERATED_UCLASS_BODY()
public:
	~ULeapScreenTapGesture();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Screen Tap  Gesture")
	TEnumAsByte<LeapBasicDirection> BasicDirection;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Screen Tap Gesture")
	FVector Direction;

	UFUNCTION(BlueprintCallable, Category = "Leap Screen Tap Gesture")
	class ULeapPointable* Pointable();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Screen Tap Gesture")
	FVector Position;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Screen Tap Gesture")
	float Progress;

	void SetGesture(const class Leap::ScreenTapGesture &Gesture);

private:
	class FPrivateScreenTapGesture* Private;

	UPROPERTY()
	ULeapPointable* PPointable = nullptr;
};