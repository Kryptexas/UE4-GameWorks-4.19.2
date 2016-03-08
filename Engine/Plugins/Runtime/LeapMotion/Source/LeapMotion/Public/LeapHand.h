// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "LeapEnums.h"
#include "LeapMotionPublicPCH.h"
#include "LeapHand.generated.h"

//Api Reference: https://developer.leapmotion.com/documentation/cpp/api/Leap.Hand.html

UCLASS(BlueprintType)
class ULeapHand : public UObject
{
	GENERATED_UCLASS_BODY()
public:
	~ULeapHand();
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Hand")
	class ULeapArm* Arm;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Hand")
	bool IsLeft;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Hand")
	bool IsRight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Hand")
	bool IsValid;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Hand")
	TEnumAsByte<LeapHandType> HandType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Hand")
	FVector PalmNormal;

	//Custom API, Origin is a flat palm facing down.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Hand")
	FRotator PalmOrientation;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Hand")
	FVector PalmPosition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Hand")
	FVector PalmVelocity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Hand")
	FMatrix Basis;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Hand")
	float Confidence;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Hand")
	FVector Direction;

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Frame"), Category = "Leap Hand")
	class ULeapFrame* Frame();

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Fingers"), Category = "Leap Hand")
	class ULeapFingerList* Fingers();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Hand")
	float GrabStrength;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Hand")
	float PalmWidth;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Hand")
	float PinchStrength;

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "RotationAngle"), Category = "Leap Hand")
	float RotationAngle(class ULeapFrame *OtherFrame);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "RotationAngleWithAxis"), Category = "Leap Hand")
	float RotationAngleWithAxis(class ULeapFrame *OtherFrame, const FVector &Axis);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "RotationMatrix"), Category = "Leap Hand")
	FMatrix RotationMatrix(const class ULeapFrame *OtherFrame);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "RotationAxis"), Category = "Leap Hand")
	FVector RotationAxis(const class ULeapFrame *OtherFrame);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "RotationProbability"), Category = "Leap Hand")
	float RotationProbability(const class ULeapFrame *OtherFrame);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "ScaleFactor"), Category = "Leap Hand")
	float ScaleFactor(const class ULeapFrame *OtherFrame);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "ScaleProbability"), Category = "Leap Hand")
	float ScaleProbability(const class ULeapFrame *OtherFrame);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Hand")
	FVector SphereCenter;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Hand")
	float SphereRadius;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Hand")
	FVector StabilizedPalmPosition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Hand")
	float TimeVisible;

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Translation"), Category = "Leap Hand")
	FVector Translation(const class ULeapFrame *OtherFrame);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "TranslationProbability"), Category = "Leap Hand")
	float TranslationProbability(const class ULeapFrame *OtherFrame);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Hand")
	int32 Id;


	bool operator!=(const ULeapHand &) const;

	bool operator==(const ULeapHand &) const;

	void SetHand(const class Leap::Hand &Hand);

private:
	class FPrivateHand* Private;

	UPROPERTY()
	ULeapFrame* PFrame = nullptr;
	UPROPERTY()
	ULeapFingerList* PFingers = nullptr;
};