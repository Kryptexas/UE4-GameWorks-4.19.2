// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/**
*
* A lightweight multi-threaded CSV profiler which can be used for profiling in Test/Shipping builds
*/

#pragma once

#include "CoreTypes.h"
#include "CoreMinimal.h"
#include "Containers/Queue.h"

#if WITH_SERVER_CODE
  #define CSV_PROFILER (WITH_ENGINE && 1)
#else
  #define CSV_PROFILER (WITH_ENGINE && !UE_BUILD_SHIPPING)
#endif

#if CSV_PROFILER

// Helpers
#define CSV_CATEGORY_INDEX(CategoryName)						(_GCsvCategory_##CategoryName.Index)
#define CSV_CATEGORY_INDEX_GLOBAL								(0)
#define CSV_STAT_FNAME(StatName)								(_GCsvStat_##StatName.Name)

// Inline stats (no up front definition)
#define CSV_SCOPED_TIMING_STAT(Category,StatName)				FScopedCsvStat ScopedStat_ ## StatName (#StatName, CSV_CATEGORY_INDEX(Category));
#define CSV_SCOPED_TIMING_STAT_GLOBAL(StatName)					FScopedCsvStat ScopedStat_ ## StatName (#StatName, CSV_CATEGORY_INDEX_GLOBAL);

#define CSV_CUSTOM_STAT(Category,StatName,Value,Op)				FCsvProfiler::Get()->RecordCustomStat(#StatName, CSV_CATEGORY_INDEX(Category), Value, Op)
#define CSV_CUSTOM_STAT_GLOBAL(StatName,Value,Op) 				FCsvProfiler::Get()->RecordCustomStat(#StatName, CSV_CATEGORY_INDEX_GLOBAL, Value, Op)

// Stats declared up front
#define CSV_DEFINE_STAT(Category,StatName)						FCsvDeclaredStat _GCsvStat_##StatName((TCHAR*)TEXT(#StatName), CSV_CATEGORY_INDEX(Category));
#define CSV_DEFINE_STAT_GLOBAL(StatName)						FCsvDeclaredStat _GCsvStat_##StatName((TCHAR*)TEXT(#StatName), CSV_CATEGORY_INDEX_GLOBAL);
#define CSV_DECLARE_STAT_EXTERN(Category,StatName)				extern FCsvDeclaredStat _GCsvStat_##StatName
#define CSV_CUSTOM_STAT_DEFINED(StatName,Value,Op)				FCsvProfiler::Get()->RecordCustomStat(_GCsvStat_##StatName.Name, _GCsvStat_##StatName.CategoryIndex, Value, Op);

// Categories
#define CSV_DEFINE_CATEGORY(CategoryName,bDefaultValue)			FCsvCategory _GCsvCategory_##CategoryName(TEXT(#CategoryName),bDefaultValue)
#define CSV_DECLARE_CATEGORY_EXTERN(CategoryName)				extern FCsvCategory _GCsvCategory_##CategoryName

#define CSV_DEFINE_CATEGORY_MODULE(Module_API,CategoryName,bDefaultValue)	FCsvCategory Module_API _GCsvCategory_##CategoryName(TEXT(#CategoryName),bDefaultValue)
#define CSV_DECLARE_CATEGORY_MODULE_EXTERN(Module_API,CategoryName)			extern Module_API FCsvCategory _GCsvCategory_##CategoryName

// Events
#define CSV_EVENT(Category, Format, ...) 						FCsvProfiler::Get()->RecordEventf( CSV_CATEGORY_INDEX(Category), Format, __VA_ARGS__ )
#define CSV_EVENT_GLOBAL(Format, ...) 							FCsvProfiler::Get()->RecordEventf( CSV_CATEGORY_INDEX_GLOBAL, Format, __VA_ARGS__ )

#else
  #define CSV_CATEGORY_INDEX(CategoryName)						
  #define CSV_CATEGORY_INDEX_GLOBAL								
  #define CSV_STAT_FNAME(StatName)								
  #define CSV_SCOPED_TIMING_STAT(Category,StatName)				
  #define CSV_SCOPED_TIMING_STAT_GLOBAL(StatName)					
  #define CSV_CUSTOM_STAT(Category,StatName,Value,Op)				
  #define CSV_CUSTOM_STAT_GLOBAL(StatName,Value,Op) 				
  #define CSV_DEFINE_STAT(Category,StatName)						
  #define CSV_DEFINE_STAT_GLOBAL(StatName)						
  #define CSV_DECLARE_STAT_EXTERN(Category,StatName)				
  #define CSV_CUSTOM_STAT_DEFINED(StatName,Value,Op)				
  #define CSV_DEFINE_CATEGORY(CategoryName,bDefaultValue)			
  #define CSV_DECLARE_CATEGORY_EXTERN(CategoryName)				
  #define CSV_DEFINE_CATEGORY_MODULE(Module_API,CategoryName,bDefaultValue)	
  #define CSV_DECLARE_CATEGORY_MODULE_EXTERN(Module_API,CategoryName)			
  #define CSV_EVENT(Category, Format, ...) 						
  #define CSV_EVENT_GLOBAL(Format, ...) 							
#endif

#if CSV_PROFILER
class FCsvProfilerFrame;
class FCsvProfilerThreadData;
class FCsvProfilerProcessingThread;
class FName;

enum class ECsvCustomStatOp : uint8
{
	Set,
	Min,
	Max,
	Accumulate,
};

enum class ECsvCommandType : uint8
{
	Start,
	Stop,
	Count
};

struct FCsvCategory;

#define CSV_STAT_NAME_PREFIX TEXT("__CSVSTAT__")

struct FCsvDeclaredStat
{
	FCsvDeclaredStat(TCHAR* NameString, uint32 InCategoryIndex) : Name(*(FString(CSV_STAT_NAME_PREFIX) + NameString)), CategoryIndex(InCategoryIndex) {}
	FName Name;
	uint32 CategoryIndex;
};

struct FCsvCaptureCommand
{
	FCsvCaptureCommand()
		: CommandType(ECsvCommandType::Count)
		, FrameRequested(-1)
		, Value(-1)
	{}

	FCsvCaptureCommand(ECsvCommandType InCommandType, uint32 InFrameRequested, uint32 InValue = -1, FString InFilenameOverride = FString())
		: CommandType(InCommandType)
		, FrameRequested(InFrameRequested)
		, Value(InValue)
		, FilenameOverride(InFilenameOverride)
	{}

	ECsvCommandType CommandType;
	uint32 FrameRequested;
	uint32 Value;
	FString FilenameOverride;
};

/**
* FCsvProfiler class. This manages recording and reporting all for CSV stats
*/
class FCsvProfiler
{
	friend class FCsvProfilerProcessingThread;
	friend class FCsvProfilerThreadData;
	friend struct FCsvCategory;
private:
	static FCsvProfiler* Instance;
	FCsvProfiler();
public:
	static CORE_API FCsvProfiler* Get();

	CORE_API void Init();

	/** Push/pop events */
	CORE_API void BeginStat(const char * StatName, uint32 CategoryIndex);
	CORE_API void EndStat(const char * StatName, uint32 CategoryIndex);

	CORE_API void RecordCustomStat(const char * StatName, uint32 CategoryIndex, float Value, const ECsvCustomStatOp CustomStatOp);
	CORE_API void RecordCustomStat(const FName& StatName, uint32 CategoryIndex, float Value, const ECsvCustomStatOp CustomStatOp);
	CORE_API void RecordCustomStat(const char * StatName, uint32 CategoryIndex, int32 Value, const ECsvCustomStatOp CustomStatOp);
	CORE_API void RecordCustomStat(const FName& StatName, uint32 CategoryIndex, int32 Value, const ECsvCustomStatOp CustomStatOp);

	CORE_API void RecordEvent(int32 CategoryIndex, const FString& EventText);

	template <typename FmtType, typename... Types>
	inline void RecordEventf(int32 CategoryIndex, const FmtType& Fmt, Types... Args)
	{
		static_assert(TIsArrayOrRefOfType<FmtType, TCHAR>::Value, "Formatting string must be a TCHAR array.");
		static_assert(TAnd<TIsValidVariadicFunctionArg<Types>...>::Value, "Invalid argument(s) passed to FCsvProfiler::RecordEventf");

		if (!bCapturing)
		{
			return;
		}
		RecordEventfInternal(CategoryIndex, Fmt, Args...);
	}

	CORE_API bool IsCapturing();
	CORE_API bool IsCapturing_Renderthread();

	CORE_API int32 GetCaptureFrameNumber();

	/** Per-frame update */
	CORE_API void BeginFrame();
	CORE_API void EndFrame();

	/** Begin/End Capture */
	CORE_API void BeginCapture(int InNumFramesToCapture = -1, const FString& InDestinationFilenameOverride = FString());

	CORE_API void EndCapture();

	/** Final cleanup */
	void Release();

	/** Renderthread begin/end frame */
	CORE_API void BeginFrameRT();
	CORE_API void EndFrameRT();

private:
	CORE_API void VARARGS RecordEventfInternal(int32 CategoryIndex, const TCHAR* Fmt, ...);

	static CORE_API int32 RegisterCategory(const FString& Name, bool bEnableByDefault, bool bIsGlobal);
	static int32 GetCategoryIndex(const FString& Name);

	void GetProfilerThreadDataArray(TArray<FCsvProfilerThreadData*>& OutProfilerThreadDataArray);
	void WriteCaptureToFile();
	float ProcessStatData();

	const TArray<uint64>& GetTimestampsForThread(uint32 ThreadId) const;

	FCsvProfilerThreadData* GetTlsProfilerThreadData();

	int32 NumFramesToCapture;
	int32 CaptureFrameNumber;
	volatile bool bCapturing;
	volatile bool bCapturingRT; // Renderthread version of the above

	bool bInsertEndFrameAtFrameStart;

	uint64 LastEndFrameTimestamp;
	uint32 CaptureEndFrameCount;

	// Can be written from any thread - protected by ProfilerThreadDataArrayLock
	TArray<FCsvProfilerThreadData*> ProfilerThreadDataArray;
	FCriticalSection ProfilerThreadDataArrayLock;

	FString DestinationFilenameOverride;
	TQueue<FCsvCaptureCommand> CommandQueue;
	FCsvProfilerProcessingThread* ProcessingThread;
};

class FScopedCsvStat
{
public:
	FScopedCsvStat(const char * InStatName, uint32 InCategoryIndex)
		: StatName(InStatName)
		, CategoryIndex(InCategoryIndex)
	{
		FCsvProfiler::Get()->BeginStat(StatName, CategoryIndex);
	}

	~FScopedCsvStat()
	{
		FCsvProfiler::Get()->EndStat(StatName, CategoryIndex);
	}
	const char * StatName;
	uint32 CategoryIndex;
};

struct FCsvCategory
{
	FCsvCategory() : Index(-1) {}
	FCsvCategory(const TCHAR* CategoryString, bool bDefaultValue, bool bIsGlobal = false)
	{
		Name = CategoryString;
		Index = FCsvProfiler::RegisterCategory(Name, bDefaultValue, bIsGlobal);
	}

	uint32 Index;
	FString Name;
};

#endif //CSV_PROFILER