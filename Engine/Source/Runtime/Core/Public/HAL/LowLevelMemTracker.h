// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"

// this is currently incompatible with PLATFORM_USES_FIXED_GMalloc_CLASS, because this ends up being included way too early
// and currently, PLATFORM_USES_FIXED_GMalloc_CLASS is only used in Test/Shipping builds, where we don't have STATS anyway,
// but we can't #include "Stats.h" to find out
#if PLATFORM_USES_FIXED_GMalloc_CLASS

#define ENABLE_LOW_LEVEL_MEM_TRACKER 0
#define LLM_ALLOW_ASSETS_TAGS 0

#else

#include "Stats/Stats.h"

// there's no need for this without stats
#define ENABLE_LOW_LEVEL_MEM_TRACKER (STATS && !PLATFORM_HTML5 && !PLATFORM_IOS)

// this controls if the commandline is used to enable tracking, or to disable it. If LLM_COMMANDLINE_ENABLES_FUNCTIONALITY is true, 
// then tracking will only happen through Engine::Init(), at which point it will be disabled unless the commandline tells 
// it to keep going (with -llm). If LLM_COMMANDLINE_ENABLES_FUNCTIONALITY is false, then tracking will be on unless the commandline
// disables it (with -nollm)
#define LLM_COMMANDLINE_ENABLES_FUNCTIONALITY 1

// using asset tagging requires a significantly higher number of per-thread tags, so make it optional
// even if this is on, we still need to run with -llmtagset=assets because of the shear number of stat ids it makes
#define LLM_ALLOW_ASSETS_TAGS 1



// for simplicity, we always declare the groups, so stat decls will work if LLM is of but STATS is on. Otherwise we'd need: LLM(DECLARE_MEMORY_STAT(Foo, STATGROUP_LLM))
DECLARE_STATS_GROUP(TEXT("LLM"), STATGROUP_LLM, STATCAT_Advanced);
DECLARE_STATS_GROUP(TEXT("LLMPlatform"), STATGROUP_LLMPlatform, STATCAT_Advanced);
DECLARE_STATS_GROUP(TEXT("LLMMalloc"), STATGROUP_LLMMalloc, STATCAT_Advanced);
DECLARE_STATS_GROUP(TEXT("LLMRHI"), STATGROUP_LLMRHI, STATCAT_Advanced);
DECLARE_STATS_GROUP(TEXT("LLMAssets"), STATGROUP_LLMAssets, STATCAT_Advanced);

#endif


#if ENABLE_LOW_LEVEL_MEM_TRACKER

enum class ELLMTracker : uint8
{
	Platform,
	Malloc,
	RHI,

	// see FLowLevelMemTracker::UpdateStatsPerFrame when adding!
	Max,
};

// optional tags that need to be enabled with -llmtagsets=x,y,z on the commandline
enum class ELLMTagSet : uint8
{
	None,
	Assets,
	AssetClasses,

	// note: check out FLowLevelMemTracker::ShouldReduceThreads and IsAssetTagForAssets if you add any asset-style tagsets

	Max,
};


///////////////////////////////////////////////////////////////////////////////////////
// The Premade IDs below are the values to use with LLM_SCOPED_TAG_WITH_ENUM
///////////////////////////////////////////////////////////////////////////////////////
enum class ELLMScopeTag : uint32
{
	// this indicates no tag, ie unscoped
	Untagged = 0,
	// special tag that indicates the thread is paused, and that the tracking code should ignore the alloc
	Paused,

	
	// some premade IDs (don't go past ELLMScopeTag::UserAllocationStart)
	SmallBinnedAllocation = 5,
	LargeBinnedAllocation,
	ThreadStack,
	MallocPool,


	// start any custom IDs here
	UserAllocationStart = 100,
	
	// anything above this value is treated as an FName for a stat section
	MaxUserAllocation=199,

};

#define SCOPE_NAME PREPROCESSOR_JOIN(LLMScope,__LINE__)

///////////////////////////////////////////////////////////////////////////////////////
// These are the main macros to use externally when tracking memory
///////////////////////////////////////////////////////////////////////////////////////
#define LLM(x) x
#define LLM_SCOPED_TAG_WITH_ENUM(ID) FLLMScopedTag SCOPE_NAME(ID, ELLMTagSet::None);
#define LLM_SCOPED_TAG_WITH_STAT(Stat) FLLMScopedTag SCOPE_NAME(GET_STATFNAME(Stat), ELLMTagSet::None);
#define LLM_SCOPED_TAG_WITH_STAT_IN_SET(Stat, Set) FLLMScopedTag SCOPE_NAME(GET_STATFNAME(Stat), Set);
#define LLM_SCOPED_TAG_WITH_STAT_NAME(StatName) FLLMScopedTag SCOPE_NAME(StatName, ELLMTagSet::None);
#define LLM_SCOPED_TAG_WITH_STAT_NAME_IN_SET(StatName, Set) FLLMScopedTag SCOPE_NAME(StatName, Set);
#define LLM_SCOPED_SINGLE_PLATFORM_STAT_TAG(Stat) DECLARE_MEMORY_STAT(TEXT(#Stat), Stat, STATGROUP_LLMPlatform); LLM_SCOPED_TAG_WITH_STAT(Stat);
#define LLM_SCOPED_SINGLE_PLATFORM_STAT_TAG_IN_SET(Stat, Set) DECLARE_MEMORY_STAT(TEXT(#Stat), Stat, STATGROUP_LLMPlatform); LLM_SCOPED_TAG_WITH_STAT_IN_SET(Stat, Set);
#define LLM_SCOPED_SINGLE_MALLOC_STAT_TAG(Stat) DECLARE_MEMORY_STAT(TEXT(#Stat), Stat, STATGROUP_LLMMalloc); LLM_SCOPED_TAG_WITH_STAT(Stat);
#define LLM_SCOPED_SINGLE_MALLOC_STAT_TAG_IN_SET(Stat, Set) DECLARE_MEMORY_STAT(TEXT(#Stat), Stat, STATGROUP_LLMMalloc); LLM_SCOPED_TAG_WITH_STAT_IN_SET(Stat, Set);
#define LLM_SCOPED_SINGLE_RHI_STAT_TAG(Stat) DECLARE_MEMORY_STAT(TEXT(#Stat), Stat, STATGROUP_LLMRHI); LLM_SCOPED_TAG_WITH_STAT(Stat);
#define LLM_SCOPED_SINGLE_RHI_STAT_TAG_IN_SET(Stat, Set) DECLARE_MEMORY_STAT(TEXT(#Stat), Stat, STATGROUP_LLMRHI); LLM_SCOPED_TAG_WITH_STAT_IN_SET(Stat, Set);

// because there's so much overhead in creating the stat id, that we skip over it if the set isn't active
#define LLM_SCOPED_TAG_WITH_OBJECT_IN_SET(Object, Set) LLM_SCOPED_TAG_WITH_STAT_NAME_IN_SET(FLowLevelMemTracker::Get().IsTagSetActive(Set) ? (FDynamicStats::CreateMemoryStatId<FStatGroup_STATGROUP_LLMAssets>(FName(*(Object)->GetFullName())).GetName()) : NAME_None, Set);

#define LLM_SCOPED_PAUSE_TRACKING() FLLMScopedPauseTrackingWithAmountToTrack SCOPE_NAME(NAME_None, 0, ELLMTracker::Max);
#define LLM_SCOPED_PAUSE_TRACKING_WITH_ENUM_AND_AMOUNT(ID, Amount, Tracker) FLLMScopedPauseTrackingWithAmountToTrack SCOPE_NAME(ID, Amount, Tracker);
#define LLM_SCOPED_PAUSE_TRACKING_WITH_STAT_AND_AMOUNT(Stat, Amount, Tracker) FLLMScopedPauseTrackingWithAmountToTrack SCOPE_NAME(GET_STATFNAME(Stat), Amount, Tracker);

// special stat pushing for when asset tracking is on, which abuses the poor thread tracking
#define LLM_PUSH_STATS_FOR_ASSET_TAGS() if (FLowLevelMemTracker::Get().IsTagSetActive(ELLMTagSet::Assets)) FLowLevelMemTracker::Get().UpdateStatsPerFrame();





class CORE_API FLowLevelMemTracker
{
public:

	// get the singleton, which makes sure that we always have a valid object
	static FLowLevelMemTracker& Get();

	// we always start up running, but if the commandline disables us, we will do it later after main
	// (can't get the commandline early enough in a cross-platform way)
	void ProcessCommandLine(const TCHAR* CmdLine);

	// this is the main entry point for the class - used to track any pointer that was allocated or freed 
	void OnLowLevelAlloc(ELLMTracker Tracker, const void* Ptr, uint64 Size);
	void OnLowLevelFree(ELLMTracker Tracker, const void* Ptr, uint64 CheckSize);

	// expected to be called once a frame, from game thread or similar - updates memory stats 
	void UpdateStatsPerFrame(const TCHAR* LogName=nullptr);

	// Optionally set the amount of memory taken up before the game starts for executable and data segments
	void RefreshProgramSize();
	void SetProgramSize(uint64 InProgramSize);

	// console command handler
	bool Exec(const TCHAR* Cmd, FOutputDevice& Ar);

	// are we in the more intensive asset tracking mode, and is it active
	bool IsTagSetActive(ELLMTagSet Set);

	// for some tag sets, it's really useful to reduce threads, to attribute allocations to assets, for instance
	bool ShouldReduceThreads();

protected:

	// constructor
	FLowLevelMemTracker();

	// thread state manager object
	class FLLMThreadStateManager& GetThreadManager(ELLMTracker Tracker);
	friend class FLLMScopedTag;
	friend class FLLMScopedPauseTrackingWithAmountToTrack;

	bool bFirstTimeUpdating;
	uint64 ProgramSize;

	bool bIsDisabled;
	bool ActiveSets[(int32)ELLMTagSet::Max];
};


class CORE_API FLLMScopedTag
{
public:
	FLLMScopedTag(FName StatIDName, ELLMTagSet Set);
	FLLMScopedTag(ELLMScopeTag ScopeTag, ELLMTagSet Set);
	~FLLMScopedTag();
protected:
	void Init(int64 Tag, ELLMTagSet Set);
	ELLMTagSet TagSet;
};

class CORE_API FLLMScopedPauseTrackingWithAmountToTrack
{
public:
	FLLMScopedPauseTrackingWithAmountToTrack(FName StatIDName, int64 Amount, ELLMTracker TrackerForAmount);
	FLLMScopedPauseTrackingWithAmountToTrack(ELLMScopeTag ScopeTag, int64 Amount, ELLMTracker TrackerForAmount);
	~FLLMScopedPauseTrackingWithAmountToTrack();
protected:
	void Init(int64 Tag, int64 Amount, ELLMTracker TrackerForAmount);
};




// class FScopedProcessMemoryAllocation
// {
// public:
// 	FScopedProcessMemoryAllocation(const TCHAR* InType);
// 	~FScopedProcessMemoryAllocation();
// };




#else

#define LLM(...)
#define LLM_SCOPED_TAG_WITH_ENUM(...)
#define LLM_SCOPED_TAG_WITH_STAT(...)
#define LLM_SCOPED_TAG_WITH_STAT_IN_SET(...)
#define LLM_SCOPED_TAG_WITH_STAT_NAME(...)
#define LLM_SCOPED_TAG_WITH_STAT_NAME_IN_SET(...)
#define LLM_SCOPED_SINGLE_PLATFORM_STAT_TAG(...)
#define LLM_SCOPED_SINGLE_PLATFORM_STAT_TAG_IN_SET(...)
#define LLM_SCOPED_SINGLE_MALLOC_STAT_TAG(...)
#define LLM_SCOPED_SINGLE_MALLOC_STAT_TAG_IN_SET(...)
#define LLM_SCOPED_SINGLE_RHI_STAT_TAG(...)
#define LLM_SCOPED_SINGLE_RHI_STAT_TAG_IN_SET(...)
#define LLM_SCOPED_TAG_WITH_OBJECT_IN_SET(...)
#define LLM_SCOPED_PAUSE_TRACKING()
#define LLM_SCOPED_PAUSE_TRACKING_WITH_ENUM_AND_AMOUNT(...)
#define LLM_SCOPED_PAUSE_TRACKING_WITH_STAT_AND_AMOUNT(...)
#define LLM_PUSH_STATS_FOR_ASSET_TAGS()

#endif
