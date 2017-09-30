// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/**
*
* A lightweight single-threaded CSV profiler which can be used for profiling in Test/Shipping builds 
*/

#pragma once

#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "CoreTypes.h"
#include "CoreMinimal.h"

#define CSV_PROFILER (WITH_ENGINE && !UE_BUILD_SHIPPING)

#if CSV_PROFILER
  #define CSV_SCOPED_STAT(StatName)							FScopedCsvStat ScopedStat_ ## StatName (#StatName);
  #define CSV_CUSTOM_STAT_SET(StatName,Value) 				FCsvProfiler::Get()->RecordCustomStat(#StatName, Value, ECsvCustomStatType::Set )
  #define CSV_CUSTOM_STAT_MIN(StatName,Value) 				FCsvProfiler::Get()->RecordCustomStat(#StatName, Value, ECsvCustomStatType::Min )
  #define CSV_CUSTOM_STAT_MAX(StatName,Value) 				FCsvProfiler::Get()->RecordCustomStat(#StatName, Value, ECsvCustomStatType::Max )
  #define CSV_CUSTOM_STAT_ACC(StatName,Value) 				FCsvProfiler::Get()->RecordCustomStat(#StatName, Value, ECsvCustomStatType::Accumulate )
  #define CSV_CUSTOM_STAT_ACC_MAX(StatName,Value) 			FCsvProfiler::Get()->RecordCustomStat(#StatName, Value, ECsvCustomStatType::AccumulateMax )
#else
  #define CSV_SCOPED_STAT(StatName) 
  #define CSV_CUSTOM_STAT_SET(StatName,Value)							
  #define CSV_CUSTOM_STAT_MIN(StatName,Value) 				
  #define CSV_CUSTOM_STAT_MAX(StatName,Value) 				
  #define CSV_CUSTOM_STAT_ACC(StatName,Value)							
  #define CSV_CUSTOM_STAT_ACC_MAX(StatName,Value) 			
#endif

#if CSV_PROFILER
class FCsvProfilerFrame;
class FCsvProfilerThread;

enum class ECsvCustomStatType : uint8
{
	Set,
	Min,
	Max,
	Accumulate,
	AccumulateMax,
};

/**
* FCsvProfiler class. This manages recording and reporting all for CSV stats
*/
class FCsvProfiler
{
private:
	static FCsvProfiler* Instance;
	FCsvProfiler();
public:
	static ENGINE_API FCsvProfiler* Get();

	ENGINE_API void Init();

	/** Push/pop events */
	ENGINE_API void BeginStat(const char * StatName );
	ENGINE_API void EndStat(const char * StatName );

	ENGINE_API void RecordCustomStat(const char * StatName, float Value, ECsvCustomStatType CustomStatType);

	inline bool IsCapturing() { return bRequestStartCapture || bCapturing; }

	/** Per-frame update */
	void BeginFrame();
	void EndFrame();

	/** Begin/End Capture */
	ENGINE_API void BeginCapture(int InNumFramesToCapture = -1);
	ENGINE_API void EndCapture();

	/** Final cleanup */
	void Release();

private:
	/** Renderthread begin/end frame */
	void BeginFrameRT();
	void EndFrameRT();

	void WriteCaptureToFile();
	FCsvProfilerThread* GetProfilerThread();

	int32 NumFramesToCapture;
	int32 CaptureFrameNumber;
	bool bRequestStartCapture;
	bool bRequestStopCapture;
	volatile bool bCapturing;
	uint64 LastEndFrameTimestamp;

	FCriticalSection GetResultsLock;
	TArray<FCsvProfilerThread*> ProfilerThreads;
	TArray<uint64> FrameBeginTimestamps;
	TArray<uint64> FrameBeginTimestampsRT;

};

class FScopedCsvStat
{
public:
	FScopedCsvStat(const char * InStatName)
		: StatName(InStatName)
	{
		FCsvProfiler::Get()->BeginStat(StatName);
	}

	~FScopedCsvStat()
	{
		FCsvProfiler::Get()->EndStat(StatName);
	}
	const char * StatName;
};
#endif //CSV_PROFILER