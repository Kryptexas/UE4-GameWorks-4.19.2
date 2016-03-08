// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "LeapMotionPublicPCH.h"
#include "LeapPointable.h"
#include "LeapTool.generated.h"

//API Reference: https://developer.leapmotion.com/documentation/cpp/api/Leap.Tool.html

UCLASS(BlueprintType)
class ULeapTool : public ULeapPointable
{
	GENERATED_UCLASS_BODY()
public:
	~ULeapTool();

	void SetTool(const class Leap::Tool &Tool);

private:
	class FPrivateTool* Private;
};