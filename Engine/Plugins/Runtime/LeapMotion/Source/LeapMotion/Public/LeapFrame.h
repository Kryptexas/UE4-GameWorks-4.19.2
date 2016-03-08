// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "LeapMotionPublicPCH.h"
#include "LeapFrame.generated.h"

//Api Reference: https://developer.leapmotion.com/documentation/cpp/api/Leap.Frame.html

UCLASS(BlueprintType)
class ULeapFrame : public UObject
{
	GENERATED_UCLASS_BODY()
public:
	~ULeapFrame();
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Frame")
	float CurrentFPS;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Frame")
	bool IsValid;

	UFUNCTION(BlueprintCallable, Category = "Leap Frame")
	class ULeapFinger* Finger(int32 Id);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "fingers", CompactNodeTitle = "", Keywords = "get fingers"), Category = "Leap Frame")
	class ULeapFingerList* Fingers();

	UFUNCTION(BlueprintCallable, Category = "Leap Frame")
	class ULeapGesture* Gesture(int32 Id);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "gestures", CompactNodeTitle = "", Keywords = "get gestures"), Category = "Leap Frame")
	class ULeapGestureList* Gestures();

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "gestures", CompactNodeTitle = "", Keywords = "get gestures"), Category = "Leap Frame")
	class ULeapGestureList* GesturesSinceFrame(class ULeapFrame* frame);

	UFUNCTION(BlueprintCallable, Category = "Leap Frame")
	class ULeapHand* Hand(int32 Id);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "getHands", CompactNodeTitle = "", Keywords = "get hands"), Category = "Leap Frame")
	class ULeapHandList* Hands();

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "images", CompactNodeTitle = "", Keywords = "get images"), Category = "Leap Frame")
	class ULeapImageList* Images();

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "interactionBox", CompactNodeTitle = "", Keywords = "get interaction box"), Category = "Leap Frame")
	class ULeapInteractionBox* InteractionBox();

	UFUNCTION(BlueprintCallable, Category = "Leap Frame")
	class ULeapPointable* Pointable(int32 Id);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "pointables", CompactNodeTitle = "", Keywords = "get pointables"), Category = "Leap Frame")
	class ULeapPointableList* Pointables();

	UFUNCTION(BlueprintCallable, Category = "Leap Frame")
	float RotationAngle(class ULeapFrame* Frame);

	UFUNCTION(BlueprintCallable, Category = "Leap Frame")
	float RotationAngleAroundAxis(class ULeapFrame* Frame, FVector Axis);

	UFUNCTION(BlueprintCallable, Category = "Leap Frame")
	FVector RotationAxis(class ULeapFrame* Frame);

	UFUNCTION(BlueprintCallable, Category = "Leap Frame")
	float RotationProbability(class ULeapFrame* Frame);

	UFUNCTION(BlueprintCallable, Category = "Leap Frame")
	float ScaleFactor(class ULeapFrame* Frame);

	UFUNCTION(BlueprintCallable, Category = "Leap Frame")
	float ScaleProbability(class ULeapFrame* Frame);

	UFUNCTION(BlueprintCallable, Category = "Leap Frame")
	class ULeapTool* Tool(int32 Id);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "pointables", CompactNodeTitle = "", Keywords = "get pointables"), Category = "Leap Frame")
	class ULeapToolList* Tools();

	UFUNCTION(BlueprintCallable, Category = "Leap Frame")
	FVector Translation(class ULeapFrame* Frame);

	UFUNCTION(BlueprintCallable, Category = "Leap Frame")
	float TranslationProbability(class ULeapFrame* Frame);

	void SetFrame(Leap::Controller &Leap, int32 History = 0);
	void SetFrame(const class Leap::Frame &Frame);
	const Leap::Frame &GetFrame() const;

private:
	class FPrivateLeapFrame* Private;

	UPROPERTY()
	ULeapFinger* PFinger = nullptr;
	UPROPERTY()
	ULeapFingerList* PFingers = nullptr;
	UPROPERTY()
	ULeapGesture* PGesture = nullptr;
	UPROPERTY()
	ULeapGestureList* PGestures = nullptr;
	UPROPERTY()
	ULeapHand* PHand = nullptr;
	UPROPERTY()
	ULeapHandList* PHands = nullptr;
	UPROPERTY()
	ULeapImageList* PImages = nullptr;
	UPROPERTY()
	ULeapInteractionBox* PInteractionBox = nullptr;
	UPROPERTY()
	ULeapPointable* PPointable = nullptr;
	UPROPERTY()
	ULeapPointableList* PPointables = nullptr;
	UPROPERTY()
	ULeapTool* PTool = nullptr;
	UPROPERTY()
	ULeapToolList* PTools = nullptr;
};