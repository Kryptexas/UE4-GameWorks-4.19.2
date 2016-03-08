// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "LeapMotionPublicPCH.h"
#include "LeapGesture.h"
#include "LeapCircleGesture.generated.h"

//Api Reference: https://developer.leapmotion.com/documentation/cpp/api/Leap.CircleGesture.html

UCLASS(BlueprintType)
class ULeapCircleGesture : public ULeapGesture
{
	GENERATED_UCLASS_BODY()
public:
	~ULeapCircleGesture();
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Circle Gesture")
	FVector Center;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Circle Gesture")
	FVector Normal;

	UFUNCTION(BlueprintCallable, Category = "Leap Circle Gesture")
	class ULeapPointable* Pointable();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Circle Gesture")
	float Progress;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Circle Gesture")
	float Radius;

	void SetGesture(const class Leap::CircleGesture &Gesture);

private:
	class FPrivateCircleGesture* Private;

	UPROPERTY()
	class ULeapPointable* PPointable = nullptr;
};