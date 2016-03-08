// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "LeapMotionPublicPCH.h"
#include "LeapPointable.h"
#include "LeapFinger.generated.h"

//Api Reference: https://developer.leapmotion.com/documentation/cpp/api/Leap.Finger.html

UCLASS(BlueprintType)
class ULeapFinger : public ULeapPointable
{
	GENERATED_UCLASS_BODY()
public:
	~ULeapFinger();

	UFUNCTION(BlueprintCallable, meta = (Category = "Leap Finger"))
	class ULeapBone *Bone(enum LeapBoneType Type);

	//Convenience Properties
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Finger")
	class ULeapBone *Metacarpal;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Finger")
	class ULeapBone *Proximal;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Finger")
	class ULeapBone *Intermediate;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Finger")
	class ULeapBone *Distal;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Finger")
	TEnumAsByte<LeapFingerType> Type;

	void SetFinger(const class Leap::Finger &Pointable);

private:
	class FPrivateFinger* Private;
};