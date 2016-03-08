// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "LeapMotionPublicPCH.h"
#include "LeapFingerList.generated.h"

//Api Reference: https://developer.leapmotion.com/documentation/cpp/api/Leap.FingerList.html

UCLASS(BlueprintType)
class ULeapFingerList : public UObject
{
	GENERATED_UCLASS_BODY()
public:
	~ULeapFingerList();
	
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "append", CompactNodeTitle = "", Keywords = "append"), Category = "Leap Finger List")
	ULeapFingerList *Append(const ULeapFingerList *List);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Finger List")
	int32 Count;

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "extended", CompactNodeTitle = "", Keywords = "extended"), Category = "Leap Finger List")
	ULeapFingerList *Extended();

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "frontmost", CompactNodeTitle = "", Keywords = "frontmost"), Category = "Leap Finger List")
	class ULeapFinger *Frontmost();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Finger List")
	bool IsEmpty;

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "leftmost", CompactNodeTitle = "", Keywords = "leftmost"), Category = "Leap Finger List")
	class ULeapFinger *Leftmost();

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "getPointableById", CompactNodeTitle = "[]", Keywords = "get pointable by id"), Category = "Leap Finger List")
	class ULeapFinger *GetPointableById(int32 Id);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "rightmost", CompactNodeTitle = "", Keywords = "rightmost"), Category = "Leap Finger List")
	class ULeapFinger *Rightmost();

	void SetFingerList(const class Leap::FingerList &Pointables);
	Leap::FingerList* FingerList();

private:
	class FPrivateFingerList* Private;

	UPROPERTY()
	class ULeapFinger* PFrontmost = nullptr;
	UPROPERTY()
	ULeapFinger* PLeftmost = nullptr;
	UPROPERTY()
	ULeapFinger* PRightmost = nullptr;
	UPROPERTY()
	ULeapFinger* PPointableById = nullptr;
	UPROPERTY()
	class ULeapFingerList* PAppendedList = nullptr;
	UPROPERTY()
	ULeapFingerList* PExtendedList = nullptr;
};