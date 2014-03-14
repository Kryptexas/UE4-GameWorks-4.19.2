// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


/*=============================================================================================
	GenericPlatformSurvey.h: Generic platform hardware-survey classes
==============================================================================================*/

#pragma once

struct FSynthBenchmarkStat
{
	FSynthBenchmarkStat()
		: Desc(0)
		, MeasuredTime(-1)
		, IndexTime(-1)
		, ValueType(0)
		, Confidence(0)
	{
	}

	// @param InDesc descriptions
	FSynthBenchmarkStat(const TCHAR* InDesc, float InIndexTime, const TCHAR* InValueType)
		: Desc(InDesc)
		, MeasuredTime(-1)
		, IndexTime(InIndexTime)
		, ValueType(InValueType)
		, Confidence(0)
	{
//		check(InDesc);
//		check(InValueType);
	}

	float ComputePerfIndex() const
	{
		return 100.0f * IndexTime / MeasuredTime;
	}

	// @param InConfidence 0..100
	void SetMeasuredTime(float InMeasuredValue, float InConfidence = 90)
	{
//		check(InConfidence >= 0.0f && InConfidence <= 100.0f);
		MeasuredTime = InMeasuredValue;
		Confidence = InConfidence;
	}

	// @return can be 0 
	const TCHAR* GetDesc() const
	{
		return Desc;
	}

	// @return can be 0 
	const TCHAR* GetValueType() const
	{
		return ValueType;
	}

	float GetMeasuredTime() const
	{
		return MeasuredTime;
	}

	// @param 0..100
	float GetConfidence() const
	{
		return Confidence;
	}

private:
	// 0 if not valid
	const TCHAR *Desc;
	// -1 if not defined
	float MeasuredTime;
	// -1 if not defined, timing value expected on a norm GPU (index value 100, here NVidia 670)
	float IndexTime;
	// 0 if not valid
	const TCHAR *ValueType;
	// 0..100, 100: fully confident
	float Confidence;
};

struct FSynthBenchmarkResults 
{
	FSynthBenchmarkStat CPUStats[2];
	FSynthBenchmarkStat GPUStats[5];

	// 100: avg good CPU, <100:slower, >100:faster
	float ComputeCPUPerfIndex() const
	{
		// if needed we can weight them differently
		return (
			CPUStats[0].ComputePerfIndex() +
			CPUStats[1].ComputePerfIndex() ) / 2;
	}

	// 100: avg good GPU, <100:slower, >100:faster
	float ComputeGPUPerfIndex() const
	{
		// if needed we can weight them differently
		return (
			GPUStats[0].ComputePerfIndex() +
			GPUStats[1].ComputePerfIndex() +
			GPUStats[2].ComputePerfIndex() +
			GPUStats[3].ComputePerfIndex() +
			GPUStats[4].ComputePerfIndex() ) / 5;
	}
};

struct FHardwareDisplay	
{
	static const uint32 MaxStringLength = 260;

	uint32 CurrentModeWidth;
	uint32 CurrentModeHeight;

	TCHAR GPUCardName[MaxStringLength];
	uint32 GPUDedicatedMemoryMB;
	TCHAR GPUDriverVersion[MaxStringLength];
};

struct FHardwareSurveyResults
{
	static const int32 MaxDisplayCount = 8; 
	static const int32 MaxStringLength = FHardwareDisplay::MaxStringLength; 

	TCHAR Platform[MaxStringLength];

	TCHAR OSVersion[MaxStringLength];
	TCHAR OSSubVersion[MaxStringLength];
	uint32 OSBits;
	TCHAR OSLanguage[MaxStringLength];

	TCHAR MultimediaAPI[MaxStringLength];

	uint32 HardDriveGB;
	uint32 MemoryMB;

	float CPUPerformanceIndex;
	float GPUPerformanceIndex;
	float RAMPerformanceIndex;

	uint32 bIsLaptopComputer:1;
	uint32 bIsRemoteSession:1;

	uint32 CPUCount;
	float CPUClockGHz;
	TCHAR CPUBrand[MaxStringLength];

	uint32 DisplayCount;
	FHardwareDisplay Displays[MaxDisplayCount];

	uint32 ErrorCount;
	TCHAR LastSurveyError[MaxStringLength];
	TCHAR LastSurveyErrorDetail[MaxStringLength];
	TCHAR LastPerformanceIndexError[MaxStringLength];
	TCHAR LastPerformanceIndexErrorDetail[MaxStringLength];

	FSynthBenchmarkResults SynthBenchmark;
};

/**
* Generic implementation for most platforms, these tend to be unused and unimplemented
**/
struct CORE_API FGenericPlatformSurvey
{
	/**
	 * Attempt to get hardware survey results now.
	 * If they aren't available the survey will be started/ticked until complete.
	 *
	 * @param OutResults	The struct that receives the results if available.
	 * @param bWait			If true, the function won't return until the results are available or the survey fails. Defaults to false.
	 *
	 * @return				True if the results were available, false otherwise.
	 **/
	static bool GetSurveyResults( FHardwareSurveyResults& OutResults, bool bWait = false )
	{
		return false;
	}
};