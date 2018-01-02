// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

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
  // These stats require literals, but no up-front declaration
  #define CSV_SCOPED_STAT(StatName)							FScopedCsvStat ScopedStat_ ## StatName (#StatName);
  #define CSV_CUSTOM_STAT_SET(StatName,Value) 				FCsvProfiler::Get()->RecordCustomStat(#StatName, Value, ECsvCustomStatType::Set )
  #define CSV_CUSTOM_STAT_MIN(StatName,Value) 				FCsvProfiler::Get()->RecordCustomStat(#StatName, Value, ECsvCustomStatType::Min )
  #define CSV_CUSTOM_STAT_MAX(StatName,Value) 				FCsvProfiler::Get()->RecordCustomStat(#StatName, Value, ECsvCustomStatType::Max )
  #define CSV_CUSTOM_STAT_ACC(StatName,Value) 				FCsvProfiler::Get()->RecordCustomStat(#StatName, Value, ECsvCustomStatType::Accumulate )
  #define CSV_CUSTOM_STAT_ACC_MAX(StatName,Value) 			FCsvProfiler::Get()->RecordCustomStat(#StatName, Value, ECsvCustomStatType::AccumulateMax )

  // Stats declared up front
  #define CSV_DECLARE_STAT(StatName)						static FCsvProfiler::DeclaredStat CSVStat_##StatName((TCHAR*)TEXT(#StatName));
  #define CSV_DECLARED_STAT_NAME(StatName)					CSVStat_##StatName.Name 
  #define CSV_DECLARED_CUSTOM_STAT_SET(StatName,Value)		FCsvProfiler::Get()->RecordCustomStat( CSVStat_##StatName.Name,Value, ECsvCustomStatType::Set );
#else
  #define CSV_SCOPED_STAT(StatName) 
  #define CSV_CUSTOM_STAT_SET(StatName,Value)							
  #define CSV_CUSTOM_STAT_MIN(StatName,Value) 				
  #define CSV_CUSTOM_STAT_MAX(StatName,Value) 				
  #define CSV_CUSTOM_STAT_ACC(StatName,Value)							
  #define CSV_CUSTOM_STAT_ACC_MAX(StatName,Value) 			
  #define CSV_DECLARE_STAT(StatName)						
  #define CSV_DECLARED_STAT_NAME(StatName)					FName()
  #define CSV_DECLARED_CUSTOM_STAT_SET(StatName,Value)	
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

#define CSV_STAT_NAME_PREFIX TEXT("__CSVSTAT__")
/**
* FCsvProfiler class. This manages recording and reporting all for CSV stats
*/
class FCsvProfiler
{
private:
	static FCsvProfiler* Instance;
	FCsvProfiler();
public:
	struct DeclaredStat
	{
		DeclaredStat(TCHAR* NameString) : Name(*(FString(CSV_STAT_NAME_PREFIX) + NameString)) {}
		FName Name;
	};
	static ENGINE_API FCsvProfiler* Get();

	ENGINE_API void Init();

	/** Push/pop events */
	ENGINE_API void BeginStat(const char * StatName );
	ENGINE_API void EndStat(const char * StatName );

	ENGINE_API void RecordCustomStat(const char * StatName, float Value, const ECsvCustomStatType CustomStatType);
	ENGINE_API void RecordCustomStat(const FName& StatName, float Value, const ECsvCustomStatType CustomStatType);

	ENGINE_API bool IsCapturing();
	ENGINE_API bool IsCapturing_Renderthread();

	ENGINE_API int32 GetCaptureFrameNumber();

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
	volatile bool bCapturingRT; // Renderthread version of the above
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