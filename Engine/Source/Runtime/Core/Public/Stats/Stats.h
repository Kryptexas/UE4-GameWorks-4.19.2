// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	UnStats.h: Performance stats framework.
=============================================================================*/
#pragma once

#define FORCEINLINE_STATS FORCEINLINE
#define checkStats(x)

#if !defined(STATS)
#error "STATS must be defined as either zero or one."
#endif

#include "UMemoryDefines.h"
#include "StatsMisc.h"
CORE_API DECLARE_LOG_CATEGORY_EXTERN(LogStats, Warning, All);

// used by the profiler
enum EStatType
{
	STATTYPE_CycleCounter,
	STATTYPE_AccumulatorFLOAT,
	STATTYPE_AccumulatorDWORD,
	STATTYPE_CounterFLOAT,
	STATTYPE_CounterDWORD,
	STATTYPE_MemoryCounter,
	STATTYPE_Error
};

template<class T> inline const TCHAR* GetStatFormatString(void) { return TEXT(""); }
template<> inline const TCHAR* GetStatFormatString<uint32>(void) { return TEXT("%u"); }
template<> inline const TCHAR* GetStatFormatString<uint64>(void) { return TEXT("%llu"); }
template<> inline const TCHAR* GetStatFormatString<int64>(void) { return TEXT("%lld"); }
template<> inline const TCHAR* GetStatFormatString<float>(void) { return TEXT("%.1f"); }
template<> inline const TCHAR* GetStatFormatString<double>(void) { return TEXT("%.1f"); }

#include "stats2.h"

#if STATS

/**
 * This is a utility class for counting the number of cycles during the
 * lifetime of the object. It updates the per thread values for this
 * stat.
 */
class FScopeCycleCounter : public FCycleCounter
{
public:
	/**
	 * Pushes the specified stat onto the hierarchy for this thread. Starts
	 * the timing of the cycles used
	 */
	FORCEINLINE_STATS FScopeCycleCounter(TStatId StatId)
	{
		if (FThreadStats::IsCollectingData(StatId))
		{
			Start(*StatId);
		}
	}

	/**
	 * Updates the stat with the time spent
	 */
	FORCEINLINE_STATS ~FScopeCycleCounter()
	{
		Stop();
	}

};

FORCEINLINE void StatsMasterEnableAdd(int32 Value = 1)
{
	FThreadStats::MasterEnableAdd(Value);
}
FORCEINLINE void StatsMasterEnableSubtract(int32 Value = 1)
{
	FThreadStats::MasterEnableSubtract(Value);
}

#else

struct FNothing{};

typedef FNothing TStatId;

class FScopeCycleCounter
{
public:
	FORCEINLINE_STATS FScopeCycleCounter(TStatId)
	{
	}
};

FORCEINLINE void StatsMasterEnableAdd(int32 Value = 1)
{
}
FORCEINLINE void StatsMasterEnableSubtract(int32 Value = 1)
{
}

// Remove all the macros
#define STAT_IS_COLLECTING(Stat) (false)

#define REGISTER_DYNAMIC_CYCLE_STAT(Stat, Group) (TStatId())


#define DEFINE_STAT(Stat)
#define SCOPE_CYCLE_COUNTER(Stat)
#define QUICK_SCOPE_CYCLE_COUNTER(Stat)
#define DECLARE_SCOPE_CYCLE_COUNTER(CounterName,StatId,GroupId)
#define CONDITIONAL_SCOPE_CYCLE_COUNTER(Stat,bCondition)
#define RETURN_QUICK_DECLARE_CYCLE_STAT(StatId,GroupId) return TStatId();
#define DECLARE_CYCLE_STAT(CounterName,StatId,GroupId)
#define DECLARE_FLOAT_COUNTER_STAT(CounterName,StatId,GroupId)
#define DECLARE_DWORD_COUNTER_STAT(CounterName,StatId,GroupId)
#define DECLARE_FLOAT_ACCUMULATOR_STAT(CounterName,StatId,GroupId)
#define DECLARE_DWORD_ACCUMULATOR_STAT(CounterName,StatId,GroupId)
#define DECLARE_MEMORY_STAT(CounterName,StatId,GroupId)
#define DECLARE_MEMORY_STAT_POOL(CounterName,StatId,GroupId,Pool)
#define DECLARE_CYCLE_STAT_EXTERN(CounterName,StatId,GroupId, API)
#define DECLARE_FLOAT_COUNTER_STAT_EXTERN(CounterName,StatId,GroupId, API)
#define DECLARE_DWORD_COUNTER_STAT_EXTERN(CounterName,StatId,GroupId, API)
#define DECLARE_FLOAT_ACCUMULATOR_STAT_EXTERN(CounterName,StatId,GroupId, API)
#define DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(CounterName,StatId,GroupId, API)
#define DECLARE_MEMORY_STAT_EXTERN(CounterName,StatId,GroupId, API)
#define DECLARE_MEMORY_STAT_POOL_EXTERN(CounterName,StatId,GroupId,Pool, API)

#define DECLARE_STATS_GROUP(GroupDesc,GroupId)
#define DECLARE_STATS_GROUP_VERBOSE(GroupDesc,GroupId)
#define DECLARE_STATS_GROUP_COMPILED_OUT(GroupDesc,GroupId)

#define SET_CYCLE_COUNTER(Stat,Cycles)
#define INC_DWORD_STAT(StatId)
#define INC_FLOAT_STAT_BY(StatId,Amount)
#define INC_DWORD_STAT_BY(StatId,Amount)
#define INC_DWORD_STAT_FNAME_BY(StatId,Amount)
#define INC_MEMORY_STAT_BY(StatId,Amount)
#define DEC_DWORD_STAT(StatId)
#define DEC_FLOAT_STAT_BY(StatId,Amount)
#define DEC_DWORD_STAT_BY(StatId,Amount)
#define DEC_DWORD_STAT_FNAME_BY(StatId,Amount)
#define DEC_MEMORY_STAT_BY(StatId,Amount)
#define SET_MEMORY_STAT(StatId,Value)
#define SET_DWORD_STAT(StatId,Value)
#define SET_FLOAT_STAT(StatId,Value)

#define SET_CYCLE_COUNTER_FName(Stat,Cycles)
#define INC_DWORD_STAT_FName(Stat)
#define INC_FLOAT_STAT_BY_FName(Stat, Amount)
#define INC_DWORD_STAT_BY_FName(Stat, Amount)
#define INC_MEMORY_STAT_BY_FName(Stat, Amount)
#define DEC_DWORD_STAT_FName(Stat)
#define DEC_FLOAT_STAT_BY_FName(Stat,Amount)
#define DEC_DWORD_STAT_BY_FName(Stat,Amount)
#define DEC_MEMORY_STAT_BY_FName(Stat,Amount)
#define SET_MEMORY_STAT_FName(Stat,Value)
#define SET_DWORD_STAT_FName(Stat,Value)
#define SET_FLOAT_STAT_FName(Stat,Value)

#define GET_STATID(Stat) (TStatId())
#define GET_STATFNAME(Stat) (FName())

#endif