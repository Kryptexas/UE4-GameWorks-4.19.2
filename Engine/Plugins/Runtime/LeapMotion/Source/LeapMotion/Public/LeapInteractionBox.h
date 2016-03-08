// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "LeapMotionPublicPCH.h"
#include "LeapInteractionBox.generated.h"

//API Reference: https://developer.leapmotion.com/documentation/cpp/api/Leap.InteractionBox.html

UCLASS(BlueprintType)
class ULeapInteractionBox : public UObject
{
	GENERATED_UCLASS_BODY()
public:
	~ULeapInteractionBox();
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Interaction Box")
	FVector Center;

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "DenormalizePoint", CompactNodeTitle = "", Keywords = "normalize point"), Category = "Leap Interaction Box")
	FVector DenormalizePoint(FVector Position) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Interaction Box")
	float Depth;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Interaction Box")
	float Height;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Interaction Box")
	bool IsValid;

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "normalizePoint", CompactNodeTitle = "", Keywords = "normalize point"), Category = "Leap Interaction Box")
	FVector NormalizePoint(FVector Position, bool Clamp=true) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Interaction Box")
	float Width;

	void SetInteractionBox(const class Leap::InteractionBox &InteractionBox);

private:
	class FPrivateInteractionBox* Private;
};