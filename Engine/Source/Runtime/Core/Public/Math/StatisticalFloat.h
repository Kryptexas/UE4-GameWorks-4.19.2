// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Misc/AssertionMacros.h"
#include "Math/UnrealMathUtility.h"

// Used to measure a distribution
struct FStatisticalFloat
{
public:
	FStatisticalFloat()
		: MinValue(0.0)
		, MaxValue(0.0)
		, Accumulator(0.0)
		, NumSamples(0)
	{
	}

	void AddSample(double Value)
	{
		if (NumSamples == 0)
		{
			MinValue = MaxValue = Value;
		}
		else
		{
			MinValue = FMath::Min(MinValue, Value);
			MaxValue = FMath::Max(MaxValue, Value);
		}
		Accumulator += Value;
		++NumSamples;
	}

	double GetMinValue() const
	{
		return MinValue;
	}

	double GetMaxValue() const
	{
		return MaxValue;
	}

	double GetAvgValue() const
	{
		return Accumulator / (double)NumSamples;
	}

	int32 GetCount() const
	{
		return NumSamples;
	}

private:
	double MinValue;
	double MaxValue;
	double Accumulator;
	int32 NumSamples;
};
