// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "LeapMotionPublicPCH.h"
#include "LeapGesture.generated.h"

//Api Reference: https://developer.leapmotion.com/documentation/cpp/api/Leap.Gesture.html

UCLASS(BlueprintType)
class ULeapGesture : public UObject
{
	GENERATED_UCLASS_BODY()
public:
	~ULeapGesture();
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Gesture")
	float Duration;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Gesture")
	float DurationSeconds;

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "frame", CompactNodeTitle = "", Keywords = "frame"), Category = "Leap Gesture")
	class ULeapFrame* Frame();

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "hands", CompactNodeTitle = "", Keywords = "hands"), Category = "Leap Gesture")
	class ULeapHandList* Hands();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Gesture")
	int32 Id;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Gesture")
	bool IsValid;

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "pointables", CompactNodeTitle = "", Keywords = "pointables"), Category = "Leap Gesture")
	class ULeapPointableList* Pointables();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Gesture")
	TEnumAsByte<LeapGestureState> State;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Gesture")
	TEnumAsByte<LeapGestureType> Type;

	bool operator!=(const ULeapGesture &rhs) const;
	bool operator==(const ULeapGesture &rhs) const;

	void SetGesture(const class Leap::Gesture &Gesture);

private:
	class FPrivateGesture* Private;

	UPROPERTY()
	ULeapFrame* PFrame = nullptr;
	UPROPERTY()
	ULeapHandList* PHands = nullptr;
	UPROPERTY()
	ULeapPointableList* PPointables = nullptr;
};