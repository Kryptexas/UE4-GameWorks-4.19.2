// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class UEdGraphPin;

/////////////////////////////////////////////////////
// FGraphSplineOverlapResult

struct FGraphSplineOverlapResult
{
	UEdGraphPin* BestPin;
	UEdGraphPin* Pin1;
	UEdGraphPin* Pin2;
	float DistanceSquared;
	float DistanceSquaredToPin1;
	float DistanceSquaredToPin2;

	FGraphSplineOverlapResult()
		: BestPin(nullptr)
		, Pin1(nullptr)
		, Pin2(nullptr)
		, DistanceSquared(FLT_MAX)
		, DistanceSquaredToPin1(FLT_MAX)
		, DistanceSquaredToPin2(FLT_MAX)
	{
	}

	FGraphSplineOverlapResult(UEdGraphPin* InPin1, UEdGraphPin* InPin2, float InDistanceSquared, float InDistanceSquaredToPin1, float InDistanceSquaredToPin2)
		: BestPin(nullptr)
		, Pin1(InPin1)
		, Pin2(InPin2)
		, DistanceSquared(InDistanceSquared)
		, DistanceSquaredToPin1(InDistanceSquaredToPin1)
		, DistanceSquaredToPin2(InDistanceSquaredToPin2)
	{
	}

	bool IsValid() const
	{
		return DistanceSquared < FLT_MAX;
	}

	GRAPHEDITOR_API void ComputeBestPin();

	UEdGraphPin* GetBestPin() const
	{
		return IsValid() ? BestPin : nullptr;
	}
};
