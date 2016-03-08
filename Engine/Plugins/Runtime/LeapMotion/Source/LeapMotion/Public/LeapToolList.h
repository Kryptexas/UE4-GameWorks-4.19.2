// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "LeapMotionPublicPCH.h"
#include "LeapToolList.generated.h"

//API Reference: https://developer.leapmotion.com/documentation/cpp/api/Leap.ToolList.html

UCLASS(BlueprintType)
class ULeapToolList : public UObject
{
	GENERATED_UCLASS_BODY()
public:
	~ULeapToolList();
	
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "append", CompactNodeTitle = "", Keywords = "append"), Category = "Leap Tool List")
	ULeapToolList *Append(const ULeapToolList *List);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "count", CompactNodeTitle = "", Keywords = "count"), Category = "Leap Tool List")
	int32 Count() const;

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "frontmost", CompactNodeTitle = "", Keywords = "frontmost"), Category = "Leap Tool List")
	class ULeapTool *Frontmost();

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "isEmpty", CompactNodeTitle = "", Keywords = "is empty"), Category = "Leap Tool List")
	bool IsEmpty() const;

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "leftmost", CompactNodeTitle = "", Keywords = "leftmost"), Category = "Leap Tool List")
	class ULeapTool *Leftmost();

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "getPointableById", CompactNodeTitle = "[]", Keywords = "get pointable by id"), Category = "Leap Tool List")
	class ULeapPointable *GetPointableById(int32 Id);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "rightmost", CompactNodeTitle = "", Keywords = "rightmost"), Category = "Leap Tool List")
	class ULeapTool *Rightmost();

	void SetToolList(const class Leap::ToolList &Tools);
	const Leap::ToolList* ToolList();

private:
	class FPrivateToolList* Private;

	UPROPERTY()
	ULeapTool* PLeftmost = nullptr;
	UPROPERTY()
	ULeapTool* PRightmost = nullptr;
	UPROPERTY()
	ULeapTool* PFrontmost = nullptr;
	UPROPERTY()
	ULeapTool* PPointableById = nullptr;
	UPROPERTY()
	ULeapToolList* PAppendedList = nullptr;
};