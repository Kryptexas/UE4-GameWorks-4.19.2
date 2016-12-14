// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ScopeLock.h"

// Macros which can be enabled to cause DSP sample checking
#define CHECK_SAMPLE(VALUE) 
#define CHECK_SAMPLE2(VALUE)

//#define CHECK_SAMPLE(VALUE)  Audio::CheckSample(VALUE)
//#define CHECK_SAMPLE2(VALUE)  Audio::CheckSample(VALUE)

namespace Audio
{
	// Utility to check for sample clipping. Put breakpoint in conditional to find 
	// DSP code that's not behaving correctly
	static void CheckSample(float InSample)
	{
		if (InSample > 1.0f || InSample < -1.0f)
		{
			UE_LOG(LogTemp, Log, TEXT("SampleValue Was %.2f"), InSample);
		}
	}

	// Clamps floats to 0 if they are in sub-normal range
	FORCEINLINE float UnderflowClamp(const float InValue)
	{
		if (InValue > 0.0f && InValue < FLT_MIN)
		{
			return 0.0f;
		}
		if (InValue < 0.0f && InValue > -FLT_MIN)
		{
			return 0.0f;
		}
		return InValue;
	}

	FORCEINLINE float ConvertToDecibels(const float InLinear)
	{
		return 20.0f * FMath::LogX(10.0f, FMath::Max(InLinear, SMALL_NUMBER));
	}

	// Simple parameter object which uses critical section to write to and read from data
	template<typename T>
	class TParams
	{
	public:
		TParams()
			: bChanged(false)
		{}

		// Sets the params
		void SetParams(const T& InParams)
		{
			FScopeLock Lock(&CritSect);
			bChanged = true;
			CurrentParams = InParams;
		}

		// Returns a copy of the params safely if they've changed since last time this was called
		bool GetParams(T* OutParamsCopy)
		{
			FScopeLock Lock(&CritSect);
			if (bChanged)
			{
				bChanged = false;
				*OutParamsCopy = CurrentParams;
				return true;
			}
			return false;
		}

		bool bChanged;
		T CurrentParams;
		FCriticalSection CritSect;
	};

}

