// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "LeapMotionPublicPCH.h"
#include "LeapPointable.generated.h"

//API Reference: https://developer.leapmotion.com/documentation/cpp/api/Leap.Pointable.html

UCLASS(BlueprintType)
class ULeapPointable : public UObject
{
	GENERATED_UCLASS_BODY()
public:
	~ULeapPointable();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Pointable")
	FVector Direction;

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "frame", CompactNodeTitle="", Keywords = "frame"), Category = "Leap Pointable")
	virtual class ULeapFrame *Frame();

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "hand", CompactNodeTitle="", Keywords = "hand"), Category = "Leap Pointable")
	virtual class ULeapHand *Hand();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Pointable")
	int32 Id;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Pointable")
	bool IsExtended;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Pointable")
	bool IsFinger;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Pointable")
	bool IsTool;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Pointable")
	bool IsValid;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Pointable")
	float Length;

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "equal", CompactNodeTitle="==", Keywords = "equal"), Category = "Leap Pointable")
	virtual bool Equal(const ULeapPointable *Other);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "different", CompactNodeTitle="!=", Keywords = "different"), Category = "Leap Pointable")
	virtual bool Different(const ULeapPointable *Other);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Pointable")
	FVector StabilizedTipPosition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Pointable")
	float TimeVisible;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Pointable")
	FVector TipPosition;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Pointable")
	FVector TipVelocity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Pointable")
	float TouchDistance;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Pointable")
	TEnumAsByte<LeapZone> TouchZone;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Pointable")
	float Width;

	void SetPointable(const class Leap::Pointable &Pointable);
	const Leap::Pointable &GetPointable() const;

private:
	class FPrivatePointable* Private;

	UPROPERTY()
	ULeapFrame* PFrame = nullptr;
	UPROPERTY()
	ULeapHand* PHand = nullptr;
};