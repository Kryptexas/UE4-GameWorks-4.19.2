// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "LeapMotionPublicPCH.h"
#include "LeapPointableList.generated.h"

//API Reference: https://developer.leapmotion.com/documentation/cpp/api/Leap.PointableList.html

UCLASS(BlueprintType)
class ULeapPointableList : public UObject
{
	GENERATED_UCLASS_BODY()
public:
	~ULeapPointableList();
	
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "append", CompactNodeTitle = "", Keywords = "append"), Category = "Leap Pointable List")
	ULeapPointableList* Append(class ULeapPointableList *List);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "appendFingers", CompactNodeTitle = "", Keywords = "append"), Category = "Leap Pointable List")
	ULeapPointableList* AppendFingers(class ULeapFingerList *List);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "appendTools", CompactNodeTitle = "", Keywords = "append"), Category = "Leap Pointable List")
	ULeapPointableList* AppendTools(class ULeapToolList *List);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Pointable List")
	int32 Count;

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "extended", CompactNodeTitle = "", Keywords = "extended"), Category = "Leap Pointable List")
	ULeapPointableList *Extended();

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "frontmost", CompactNodeTitle = "", Keywords = "frontmost"), Category = "Leap Pointable List")
	class ULeapPointable *Frontmost();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Pointable List")
	bool IsEmpty;

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "leftmost", CompactNodeTitle = "", Keywords = "leftmost"), Category = "Leap Pointable List")
	class ULeapPointable *Leftmost();

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "getPointableById", CompactNodeTitle = "[]", Keywords = "get pointable by id"), Category = "Leap Pointable List")
	class ULeapPointable *GetPointableById(int32 Id);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "rightmost", CompactNodeTitle = "", Keywords = "rightmost"), Category = "Leap Pointable List")
	class ULeapPointable *Rightmost();

	void SetPointableList(const class Leap::PointableList &Pointables);

private:
	class FPrivatePointableList* Private;

	UPROPERTY()
	ULeapPointable* PLeftmost = nullptr;
	UPROPERTY()
	ULeapPointable* PRightmost = nullptr;
	UPROPERTY()
	ULeapPointable* PFrontmost = nullptr;
	UPROPERTY()
	ULeapPointable* PPointableById = nullptr;
	UPROPERTY()
	ULeapPointableList* PAppendedList = nullptr;
	UPROPERTY()
	ULeapPointableList* PExtendedList = nullptr;
};