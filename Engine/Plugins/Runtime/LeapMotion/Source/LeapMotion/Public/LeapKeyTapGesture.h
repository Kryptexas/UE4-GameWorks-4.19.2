// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "LeapMotionPublicPCH.h"
#include "LeapGesture.h"
#include "LeapKeyTapGesture.generated.h"

//API Reference: https://developer.leapmotion.com/documentation/cpp/api/Leap.KeyTapGesture.html

UCLASS(BlueprintType)
class ULeapKeyTapGesture : public ULeapGesture
{
	GENERATED_UCLASS_BODY()
public:
	~ULeapKeyTapGesture();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Key Tap  Gesture")
	TEnumAsByte<LeapBasicDirection> BasicDirection;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Key Tap Gesture")
	FVector Direction;

	UFUNCTION(BlueprintCallable, Category = "Leap Key Tap Gesture")
	class ULeapPointable* Pointable();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Key Tap Gesture")
	FVector Position;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Key Tap Gesture")
	float Progress;

	void SetGesture(const class Leap::KeyTapGesture &Gesture);

private:
	class FPrivateKeyTapGesture* Private;

	UPROPERTY()
	ULeapPointable* PPointable = nullptr;
};