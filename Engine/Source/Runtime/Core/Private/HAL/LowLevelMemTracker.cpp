// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "HAL/LowLevelMemTracker.h"
#include "ScopeLock.h"

#include "LowLevelMemoryMap.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"

#if ENABLE_LOW_LEVEL_MEM_TRACKER

DECLARE_LLM_MEMORY_STAT(TEXT("LLM Overhead Total"), STAT_LLMOverheadTotal, STATGROUP_LLMOverhead);
DECLARE_LLM_MEMORY_STAT(TEXT("LLM Static Overhead"), STAT_LLMStaticOverhead, STATGROUP_LLMOverhead);
DECLARE_LLM_MEMORY_STAT(TEXT("LLM Pointer Tracking Overhead"), STAT_LLMPointerTrackingOverhead, STATGROUP_LLMOverhead);

DECLARE_LLM_MEMORY_STAT(TEXT("Small Binned OS Memory"), STAT_SmallBinnedMemory, STATGROUP_LLMPlatform);
DECLARE_LLM_MEMORY_STAT(TEXT("Large (Non) Binned OS Memory"), STAT_LargeBinnedMemory, STATGROUP_LLMPlatform);
DECLARE_LLM_MEMORY_STAT(TEXT("Malloc Pool"), STAT_MallocPool, STATGROUP_LLMPlatform);
DECLARE_LLM_MEMORY_STAT(TEXT("malloc"), STAT_Malloc, STATGROUP_LLM);
DECLARE_LLM_MEMORY_STAT(TEXT("Total"), STAT_LLMPlatformTotal, STATGROUP_LLMPlatform);
DECLARE_LLM_MEMORY_STAT(TEXT("Untracked"), STAT_LLMPlatformTotalUntracked, STATGROUP_LLMPlatform);
DECLARE_LLM_MEMORY_STAT(TEXT("Program Size"), STAT_ProgramSizePlatform, STATGROUP_LLMPlatform);
DECLARE_LLM_MEMORY_STAT(TEXT("Program Size"), STAT_ProgramSizeDefault, STATGROUP_LLM);
DECLARE_LLM_MEMORY_STAT(TEXT("Backup OOM Pool"), STAT_BackupOOMMemoryPoolPlatform, STATGROUP_LLMPlatform);
DECLARE_LLM_MEMORY_STAT(TEXT("Backup OOM Pool"), STAT_BackupOOMMemoryPoolDefault, STATGROUP_LLM);
DECLARE_LLM_MEMORY_STAT(TEXT("GenericPlatformMallocCrash"), STAT_GenericPlatformMallocCrash, STATGROUP_LLMPlatform);
DECLARE_LLM_MEMORY_STAT(TEXT("Audio Misc"), STAT_AudioMisc, STATGROUP_LLMPlatform);
DECLARE_LLM_MEMORY_STAT(TEXT("Game Misc"), STAT_GameMisc, STATGROUP_LLMPlatform);
DECLARE_LLM_MEMORY_STAT(TEXT("System Misc"), STAT_SystemMisc, STATGROUP_LLMPlatform);
DECLARE_LLM_MEMORY_STAT(TEXT("Thread Stacks"), STAT_ThreadStackMemory, STATGROUP_LLMPlatform);
DECLARE_LLM_MEMORY_STAT(TEXT("User Memory"), STAT_UserMemory, STATGROUP_LLMPlatform);

DECLARE_LLM_MEMORY_STAT(TEXT("Untagged"), STAT_UntaggedPlatformMemory, STATGROUP_LLMPlatform);
DECLARE_LLM_MEMORY_STAT(TEXT("Tracked Total"), STAT_TrackedPlatformMemory, STATGROUP_LLMPlatform);
DECLARE_LLM_MEMORY_STAT(TEXT("Untagged"), STAT_UntaggedDefaultMemory, STATGROUP_LLM);
DECLARE_LLM_MEMORY_STAT(TEXT("Tracked Total"), STAT_TrackedDefaultMemory, STATGROUP_LLM);

DEFINE_STAT(STAT_D3DPlatformLLM);

// pre-sized arrays for various resources so we don't need to allocate memory in the tracker
#define LLM_MAX_TRACKED_THREADS 64
#define LLM_MAX_THREAD_SCOPE_STACK_DEPTH 32
#if LLM_ALLOW_ASSETS_TAGS
	#if WITH_EDITOR
		#define LLM_MAX_THREAD_SCOPE_TAGS 8000
	#else
		#define LLM_MAX_THREAD_SCOPE_TAGS 4000
	#endif
#else
	#define LLM_MAX_THREAD_SCOPE_TAGS 64
#endif

/**
 * FLLMCsvWriter: class for writing out the LLM stats to a csv file every few seconds
 */
class FLLMCsvWriter
{
public:
	FLLMCsvWriter();

	~FLLMCsvWriter();

	void SetTracker(ELLMTracker InTracker) { Tracker = InTracker; }

	void AddStat(int64 Tag, int64 Value);

	void SetStat(int64 Tag, int64 Value);

	void Update();

private:
	void WriteGraph();

	void Write(const FString& Text);

	static FString GetTagName(int64 Tag);

	static const TCHAR* GetTrackerCsvName(ELLMTracker InTracker);

	struct StatValue
	{
		int64 Tag;
		int64 Value;
	};

	ELLMTracker Tracker;

	static const int32 MAxValueCount = 512;
	StatValue StatValues[MAxValueCount];
	StatValue StatValuesForWrite[MAxValueCount];

	int32 StatValueCount;

	FCriticalSection StatValuesLock;

	static const double WriteFrequency;	// write every n seconds
	double LastWriteTime;

	FArchive* Archive;

	int32 LastWriteStatValueCount;
};

class FLLMThreadStateManager
{
public:

	FLLMThreadStateManager();
	~FLLMThreadStateManager();

	void Initialise(ELLMTracker Tracker, LLMAllocFunction InAllocFunction, LLMFreeFunction InFreeFunction);

	void PushScopeTag(int64 Tag);
	void PopScopeTag();
#if LLM_ALLOW_ASSETS_TAGS
	void PushAssetTag(int64 Tag);
	void PopAssetTag();
#endif
	void TrackAllocation(const void* Ptr, uint64 Size, ELLMScopeTag DefaultTag);
	void TrackFree(const void* Ptr, uint64 CheckSize);
	void OnAllocMoved(const void* Dest, const void* Source);

	void TrackMemory(int64 Tag, int64 Amount);

	// This will pause/unpause tracking, and also manually increment a given tag
	void PauseAndTrackMemory(int64 Tag, int64 Amount);
	void Pause();
	void Unpause();
	bool IsPaused();
	
	void Clear();

	void WriteCsv();

	struct FLowLevelAllocInfo
	{
		uint64 Size;
		int64 Tag;
#if LLM_ALLOW_ASSETS_TAGS
		int64 AssetTag;
#endif
	};
	LLMMap<PointerKey, FLowLevelAllocInfo>& GetAllocationMap()
	{
		return AllocationMap;
	}

	void SetMemoryStatFNames(FName UntaggedStatName, FName TrackedStatName);
	uint64 UpdateFrameAndReturnTotalTrackedMemory();

protected:

	// per thread state, uses system malloc function to be allocated (like FMalloc*)
	class FLLMThreadState
	{
	public:
		FLLMThreadState();

		void PushScopeTag(int64 Tag);
		void PopScopeTag();
		int64 GetTopTag();
#if LLM_ALLOW_ASSETS_TAGS
		void PushAssetTag(int64 Tag);
		void PopAssetTag();
		int64 GetTopAssetTag();
#endif
		void TrackAllocation(uint64 Size, ELLMScopeTag DefaultTag);
		void TrackFree(int64 Tag, uint64 Size, bool bTrackedUntagged);
		void IncrTag(int64 Tag, int64 Amount, bool bTrackUntagged);

		void UpdateFrame(FName UntaggedStatName, FLLMCsvWriter& CsvWriter);

		FCriticalSection TagSection;
		int64 TagStack[LLM_MAX_THREAD_SCOPE_STACK_DEPTH];
		int32 StackDepth;
#if LLM_ALLOW_ASSETS_TAGS
		int64 AssetTagStack[LLM_MAX_THREAD_SCOPE_STACK_DEPTH];
		int32 AssetStackDepth;
#endif
		int64 TaggedAllocs[LLM_MAX_THREAD_SCOPE_TAGS];
		int64 TaggedAllocTags[LLM_MAX_THREAD_SCOPE_TAGS];
		int32 TaggedAllocCount;
		int64 UntaggedAllocs;

		bool bPaused;
	};

	FLLMThreadState* GetOrCreateState();
	FLLMThreadState* GetState();

	uint32 TlsSlot;

	FCriticalSection ThreadArraySection;
	int32 NextThreadState;
	FLLMThreadState ThreadStates[LLM_MAX_TRACKED_THREADS];

	int64 TrackedMemoryOverFrames;

	LLMMap<PointerKey, FLowLevelAllocInfo> AllocationMap;

	FName UntaggedStatName;
	FName TrackedStatName;

	FLLMCsvWriter CsvWriter;
};



static int64 FNameToTag(FName Name)
{
	if (Name == NAME_None)
	{
		return (int64)ELLMScopeTag::Untagged;
	}

	// get the bits out of the FName we need
	int64 NameIndex = Name.GetComparisonIndex();
	int64 NameNumber = Name.GetNumber();
	int64 tag = (NameNumber << 32) | NameIndex;
	checkf(tag > (int64)ELLMScopeTag::MaxUserAllocation, TEXT("Passed with a name index [%d - %s] that was less than MemTracker_MaxUserAllocation"), NameIndex, *Name.ToString());

	// convert it to a tag, but you can actually convert this to an FMinimalName in the debugger to view it - *((FMinimalName*)&Tag)
	return tag;
}

static FName TagToFName(int64 Tag)
{
	// pull the bits back out of the tag
	int32 NameIndex = (int32)(Tag & 0xFFFFFFFF);
	int32 NameNumber = (int32)(Tag >> 32);
	return FName(NameIndex, NameIndex, NameNumber);

}

FLowLevelMemTracker& FLowLevelMemTracker::Get()
{
	static FLowLevelMemTracker Tracker;
	return Tracker;
}

bool FLowLevelMemTracker::IsEnabled()
{
	return !FLowLevelMemTracker::Get().bIsDisabled;
}

FLowLevelMemTracker::FLowLevelMemTracker()
	: bFirstTimeUpdating(true)
	, bIsDisabled(false)		// must start off enabled because alllocations happen before the command line enables/disables us
	, bCanEnable(true)
	, bCsvWriterEnabled(false)
{
	// set the LLMMap alloc functions
	LLMAllocFunction AllocFunction = NULL;
	LLMFreeFunction FreeFunction = NULL;
	if (!FPlatformMemory::GetLLMAllocFunctions(AllocFunction, FreeFunction))
	{
		bIsDisabled = true;
		bCanEnable = false;
	}

	for (int32 ManagerIndex = 0; ManagerIndex < (int32)ELLMTracker::Max; ++ManagerIndex)
	{
		GetThreadManager((ELLMTracker)ManagerIndex).Initialise((ELLMTracker)ManagerIndex, AllocFunction, FreeFunction);
	}

	// only None is on by default
	for (int32 Index = 0; Index < (int32)ELLMTagSet::Max; Index++)
	{
		ActiveSets[Index] = Index == (int32)ELLMTagSet::None;
	}

	// calculate program size early on... the platform can call update the program size later if it sees fit
	RefreshProgramSize();
}

void FLowLevelMemTracker::UpdateStatsPerFrame(const TCHAR* LogName)
{
	// let some stats get through even if we've disabled LLM - this shows up some overhead that is always there even when disabled
	// (unless the #define completely removes support, of course)
	if (bIsDisabled && !bFirstTimeUpdating)
	{
		return;
	}

	// delay init
	if (bFirstTimeUpdating)
	{
		static_assert((uint8)ELLMTracker::Max == 2, "You added a tracker, without updating FLowLevelMemTracker::UpdateStatsPerFrame (and probably need to update macros)");

		GetThreadManager(ELLMTracker::Platform).SetMemoryStatFNames(GET_STATFNAME(STAT_UntaggedPlatformMemory), GET_STATFNAME(STAT_TrackedPlatformMemory));
		GetThreadManager(ELLMTracker::Default).SetMemoryStatFNames(GET_STATFNAME(STAT_UntaggedDefaultMemory), GET_STATFNAME(STAT_TrackedDefaultMemory));

		bFirstTimeUpdating = false;
	}

	int64 TrackedProcessMemory = 0;
	int64 PointerTrackingOverhead = 0;
	for (int32 ManagerIndex = 0; ManagerIndex < (int32)ELLMTracker::Max; ManagerIndex++)
	{
		FLLMThreadStateManager& Manager = GetThreadManager((ELLMTracker)ManagerIndex);

		// update stats and also get how much memory is now tracked
		int64 TrackedMemory = Manager.UpdateFrameAndReturnTotalTrackedMemory();

		// count all overhead
		PointerTrackingOverhead += Manager.GetAllocationMap().GetTotalMemoryUsed();

		// the Platform layer is special in that we use it to track untracked memory (it's expected that every other allocation
		// goes through this level, and if not, there's nothing better we can do)
		if (ManagerIndex == (int32)ELLMTracker::Platform)
		{
			TrackedProcessMemory = TrackedMemory;
		}
	}

	// set overhead stats
	int64 StaticOverhead = sizeof(FLLMThreadStateManager) + sizeof(FLLMThreadStateManager) * (int32)ELLMTracker::Max;
	int64 Overhead = StaticOverhead + PointerTrackingOverhead;
	SET_MEMORY_STAT(STAT_LLMOverheadTotal, Overhead);
	SET_MEMORY_STAT(STAT_LLMStaticOverhead, StaticOverhead);
	SET_MEMORY_STAT(STAT_LLMPointerTrackingOverhead, PointerTrackingOverhead);

	// calculate memory the platform thinks we have allocated, compared to what we have tracked, including the program memory
	FPlatformMemoryStats PlatformStats = FPlatformMemory::GetStats();
	uint64 PlatformProcessMemory = PlatformStats.TotalPhysical - PlatformStats.AvailablePhysical - Overhead;
	int64 PlatformTotalUntracked = PlatformProcessMemory - TrackedProcessMemory;
	SET_MEMORY_STAT(STAT_LLMPlatformTotal, PlatformProcessMemory);
	SET_MEMORY_STAT(STAT_LLMPlatformTotalUntracked, PlatformTotalUntracked);

	if (bCsvWriterEnabled)
	{
		for (int32 ManagerIndex = 0; ManagerIndex < (int32)ELLMTracker::Max; ++ManagerIndex)
		{
			GetThreadManager((ELLMTracker)ManagerIndex).WriteCsv();
		}
	}

	if (LogName != nullptr)
	{
		FPlatformMisc::LowLevelOutputDebugStringf(TEXT("---> Untracked memory at %s = %.2f mb\n"), LogName, (double)PlatformTotalUntracked / (1024.0 * 1024.0));
	}
}

void FLowLevelMemTracker::RefreshProgramSize()
{
	FPlatformMemoryStats Stats = FPlatformMemory::GetStats();
	int64 NewProgramSize = Stats.TotalPhysical - Stats.AvailablePhysical;

	int64 ProgramSizeDiff = NewProgramSize - ProgramSize;

	ProgramSize = NewProgramSize;

	GetThreadManager(ELLMTracker::Platform).TrackMemory((uint64)ELLMScopeTag::ProgramSizePlatform, ProgramSizeDiff);
	GetThreadManager(ELLMTracker::Default).TrackMemory((uint64)ELLMScopeTag::ProgramSizeDefault, ProgramSizeDiff);
}

void FLowLevelMemTracker::SetProgramSize(uint64 InProgramSize)
{
	int64 ProgramSizeDiff = InProgramSize - ProgramSize;

	ProgramSize = InProgramSize;

	GetThreadManager(ELLMTracker::Platform).TrackMemory((uint64)ELLMScopeTag::ProgramSizePlatform, ProgramSizeDiff);
	GetThreadManager(ELLMTracker::Default).TrackMemory((uint64)ELLMScopeTag::ProgramSizeDefault, ProgramSizeDiff);
}

void FLowLevelMemTracker::ProcessCommandLine(const TCHAR* CmdLine)
{
	if (bCanEnable)
	{
#if LLM_COMMANDLINE_ENABLES_FUNCTIONALITY
		// if we require commandline to enable it, then we are disabled if it's not there
		bIsDisabled = FParse::Param(CmdLine, TEXT("LLM")) == false;
#else
		// if we allow commandline to disable us, then we are disabled if it's there
		bIsDisabled = FParse::Param(CmdLine, TEXT("NOLLM")) == true;
#endif
	}

	bCsvWriterEnabled = FParse::Param(CmdLine, TEXT("LLMCSV"));

	// automatically enable LLM if only LLMCSV is there
	if (bCsvWriterEnabled && bIsDisabled && bCanEnable)
	{
		bIsDisabled = false;
	}

	if (bIsDisabled)
	{
		for (int32 ManagerIndex = 0; ManagerIndex < (int32)ELLMTracker::Max; ManagerIndex++)
		{
			GetThreadManager((ELLMTracker)ManagerIndex).Clear();
		}
	}

	// activate tag sets (we ignore None set, it's always on)
	FString SetList;
	static_assert((uint8)ELLMTagSet::Max == 3, "You added a tagset, without updating FLowLevelMemTracker::ProcessCommandLine");
	if (FParse::Value(CmdLine, TEXT("LLMTAGSETS="), SetList))
	{
		TArray<FString> Sets;
		SetList.ParseIntoArray(Sets, TEXT(","), true);
		for (FString& Set : Sets)
		{
			if (Set == TEXT("Assets"))
			{
#if LLM_ALLOW_ASSETS_TAGS // asset tracking has a per-thread memory overhead, so we have a #define to completely disable it - warn if we don't match
				ActiveSets[(int32)ELLMTagSet::Assets] = true;
#else
				UE_LOG(LogInit, Warning, TEXT("Attempted to use LLM to track assets, but LLM_ALLOW_ASSETS_TAGS is not defined to 1. You will need to enable the define"));
#endif
			}
			else if (Set == TEXT("AssetClasses"))
			{
				ActiveSets[(int32)ELLMTagSet::AssetClasses] = true;
			}
		}
	}
}

void FLowLevelMemTracker::OnLowLevelAlloc(ELLMTracker Tracker, const void* Ptr, uint64 Size, ELLMScopeTag DefaultTag)
{
	if (bIsDisabled)
	{
		return;
	}

	GetThreadManager(Tracker).TrackAllocation(Ptr, Size, DefaultTag);
}

void FLowLevelMemTracker::OnLowLevelFree(ELLMTracker Tracker, const void* Ptr, uint64 CheckSize)
{
	if (bIsDisabled)
	{
		return;
	}

	if (Ptr != nullptr)
	{
		GetThreadManager(Tracker).TrackFree(Ptr, CheckSize);
	}
}

FLLMThreadStateManager& FLowLevelMemTracker::GetThreadManager(ELLMTracker Tracker)
{
	static FLLMThreadStateManager Managers[(int32)ELLMTracker::Max];
	return Managers[(int32)Tracker];
}

void FLowLevelMemTracker::OnLowLevelAllocMoved(ELLMTracker Tracker, const void* Dest, const void* Source)
{
	if (bIsDisabled)
	{
		return;
	}

	GetThreadManager(Tracker).OnAllocMoved(Dest, Source);
}

// void FLowLevelMemTracker::StartProcessMemoryScope(const TCHAR* Type)
// {
// 	FPlatformMemoryStats PlatformStats = FPlatformMemory::GetStats();
// 	uint64 PlatformProcessMemory = PlatformStats.TotalPhysical - PlatformStats.AvailablePhysical;
// 
// 	// take away now, and add end value in End
// 	FPlatformAtomics::InterlockedAdd(&PendingProcessMemoryAllocations, 0 - PlatformProcessMemory);
// 
// 	FPlatformMisc::LowLevelOutputDebugStringf(TEXT("----> Starting to track, via Process Memory, %s\n"), Type);
// }
// 
// void FLowLevelMemTracker::EndProcessMemoryScope()
// {
// 	FPlatformMemoryStats PlatformStats = FPlatformMemory::GetStats();
// 	uint64 PlatformProcessMemory = PlatformStats.TotalPhysical - PlatformStats.AvailablePhysical;
// 
// 	// take away now, and add end value in End
// 	FPlatformAtomics::InterlockedAdd(&PendingProcessMemoryAllocations, PlatformProcessMemory);
// 
// 	FPlatformMisc::LowLevelOutputDebugStringf(TEXT("----> Stopping tracking. PENDING Tracked Process Memory is now %lld\n"), PendingProcessMemoryAllocations);
// }

bool FLowLevelMemTracker::Exec(const TCHAR* Cmd, FOutputDevice& Ar)
{
	if (FParse::Command(&Cmd, TEXT("LLMEM")))
	{
		if (FParse::Command(&Cmd, TEXT("DUMP")))
		{

		}
		else if (FParse::Command(&Cmd, TEXT("SPAMALLOC")))
		{
			int32 NumAllocs = 128;
			int64 MaxSize = FCString::Atoi(Cmd);
			if (MaxSize == 0)
			{
				MaxSize = 128 * 1024;
			}

			UpdateStatsPerFrame(TEXT("Before spam"));
			FPlatformMisc::LowLevelOutputDebugStringf(TEXT("----> Spamming %d allocations, from %d..%d bytes\n"), NumAllocs, MaxSize/2, MaxSize);

			TArray<void*> Spam;
			Spam.Reserve(NumAllocs);
			uint32 TotalSize = 0;
			for (int32 Index = 0; Index < NumAllocs; Index++)
			{
				int32 Size = (FPlatformMath::Rand() % MaxSize / 2) + MaxSize / 2;
				TotalSize += Size;
				Spam.Add(FMemory::Malloc(Size));
			}
			FPlatformMisc::LowLevelOutputDebugStringf(TEXT("----> Allocated %d total bytes\n"), TotalSize);

			UpdateStatsPerFrame(TEXT("After spam"));

			for (int32 Index = 0; Index < Spam.Num(); Index++)
			{
				FMemory::Free(Spam[Index]);
			}

			UpdateStatsPerFrame(TEXT("After cleanup"));
		}
		return true;
	}

	return false;
}

bool FLowLevelMemTracker::IsTagSetActive(ELLMTagSet Set)
{
	return !bIsDisabled && ActiveSets[(int32)Set];
}

bool FLowLevelMemTracker::ShouldReduceThreads()
{
	return IsTagSetActive(ELLMTagSet::Assets) || IsTagSetActive(ELLMTagSet::AssetClasses);
}

static bool IsAssetTagForAssets(ELLMTagSet Set)
{
	return Set == ELLMTagSet::Assets || Set == ELLMTagSet::AssetClasses;
}






FLLMScopedTag::FLLMScopedTag(FName StatIDName, ELLMTagSet Set, ELLMTracker Tracker)
{
	Init(FNameToTag(StatIDName), Set, Tracker);
}

FLLMScopedTag::FLLMScopedTag(ELLMScopeTag ScopeTag, ELLMTagSet Set, ELLMTracker Tracker)
{
	Init((int64)ScopeTag, Set, Tracker);
}

void FLLMScopedTag::Init(int64 Tag, ELLMTagSet Set, ELLMTracker Tracker)
{
	TagSet = Set;
	TrackerSet = Tracker;
	Enabled = Tag != (int64)ELLMScopeTag::Untagged;

	// early out if tracking is disabled (don't do the singleton call, this is called a lot!)
	if (!Enabled)
	{
		return;
	}

	FLowLevelMemTracker& LLM = FLowLevelMemTracker::Get();
	if (!LLM.IsTagSetActive(TagSet))
	{
		return;
	}

#if LLM_ALLOW_ASSETS_TAGS
	if (IsAssetTagForAssets(TagSet))
	{
		LLM.GetThreadManager(Tracker).PushAssetTag(Tag);
	}
	else
#endif
	{
		LLM.GetThreadManager(Tracker).PushScopeTag(Tag);
	}
}

FLLMScopedTag::~FLLMScopedTag()
{
	// early out if tracking is disabled (don't do the singleton call, this is called a lot!)
	if (!Enabled)
	{
		return;
	}

	FLowLevelMemTracker& LLM = FLowLevelMemTracker::Get();
	if (!LLM.IsTagSetActive(TagSet))
	{
		return;
	}

#if LLM_ALLOW_ASSETS_TAGS
	if (IsAssetTagForAssets(TagSet))
	{
		LLM.GetThreadManager(TrackerSet).PopAssetTag();
	}
	else
#endif
	{
		LLM.GetThreadManager(TrackerSet).PopScopeTag();
	}
}


FLLMScopedPauseTrackingWithAmountToTrack::FLLMScopedPauseTrackingWithAmountToTrack(FName StatIDName, int64 Amount, ELLMTracker TrackerToPause)
{
	Init(FNameToTag(StatIDName), Amount, TrackerToPause);
}

FLLMScopedPauseTrackingWithAmountToTrack::FLLMScopedPauseTrackingWithAmountToTrack(ELLMScopeTag ScopeTag, int64 Amount, ELLMTracker TrackerToPause)
{
	Init((uint64)ScopeTag, Amount, TrackerToPause);
}

void FLLMScopedPauseTrackingWithAmountToTrack::Init(int64 Tag, int64 Amount, ELLMTracker TrackerToPause)
{
	FLowLevelMemTracker& LLM = FLowLevelMemTracker::Get();
	if (!LLM.IsTagSetActive(ELLMTagSet::None))
	{
		return;
	}

	for (int32 ManagerIndex = 0; ManagerIndex < (int32)ELLMTracker::Max; ManagerIndex++)
	{
		ELLMTracker Tracker = (ELLMTracker)ManagerIndex;

		if (TrackerToPause == ELLMTracker::Max || TrackerToPause == Tracker)
		{
			if (Amount == 0)
			{
				LLM.GetThreadManager((ELLMTracker)ManagerIndex).Pause();
			}
			else
			{
				LLM.GetThreadManager((ELLMTracker)ManagerIndex).PauseAndTrackMemory(Tag, Amount);
			}
		}
	}
}

FLLMScopedPauseTrackingWithAmountToTrack::~FLLMScopedPauseTrackingWithAmountToTrack()
{
	FLowLevelMemTracker& LLM = FLowLevelMemTracker::Get();
	if (!LLM.IsTagSetActive(ELLMTagSet::None))
	{
		return;
	}

	for (int32 ManagerIndex = 0; ManagerIndex < (int32)ELLMTracker::Max; ManagerIndex++)
	{
		LLM.GetThreadManager((ELLMTracker)ManagerIndex).Unpause();
	}
}





FLLMThreadStateManager::FLLMThreadStateManager()
	: NextThreadState(0)
	, TrackedMemoryOverFrames(0)
{
	TlsSlot = FPlatformTLS::AllocTlsSlot();
}

FLLMThreadStateManager::~FLLMThreadStateManager()
{
	FPlatformTLS::FreeTlsSlot(TlsSlot);
}

void FLLMThreadStateManager::Initialise(
	ELLMTracker Tracker,
	LLMAllocFunction InAllocFunction,
	LLMFreeFunction InFreeFunction)
{
	CsvWriter.SetTracker(Tracker);
	AllocationMap.SetAllocationFunctions(InAllocFunction, InFreeFunction);
}

FLLMThreadStateManager::FLLMThreadState* FLLMThreadStateManager::GetOrCreateState()
{
	// look for already allocated thread state
	FLLMThreadState* State = (FLLMThreadState*)FPlatformTLS::GetTlsValue(TlsSlot);
	// get one if needed
	if (State == nullptr)
	{
		// protect any accesses to the ThreadStates array
		FScopeLock Lock(&ThreadArraySection);

		checkf(NextThreadState < LLM_MAX_TRACKED_THREADS, TEXT("Trying to track too many threads; increase LLM_MAX_TRACKED_THREADS"));
		// get next one in line
		State = &ThreadStates[NextThreadState++];

		// push to Tls
		FPlatformTLS::SetTlsValue(TlsSlot, State);
	}
	return State;
}

FLLMThreadStateManager::FLLMThreadState* FLLMThreadStateManager::GetState()
{
	return (FLLMThreadState*)FPlatformTLS::GetTlsValue(TlsSlot);
}

void FLLMThreadStateManager::PushScopeTag(int64 Tag)
{
	// pass along to the state object
	GetOrCreateState()->PushScopeTag(Tag);
}

void FLLMThreadStateManager::PopScopeTag()
{
	// look for already allocated thread state
	FLLMThreadState* State = GetState();

	checkf(State != nullptr, TEXT("Called PopScopeTag but PushScopeTag was never called!"));

	State->PopScopeTag();
}

#if LLM_ALLOW_ASSETS_TAGS
void FLLMThreadStateManager::PushAssetTag(int64 Tag)
{
	// pass along to the state object
	GetOrCreateState()->PushAssetTag(Tag);
}

void FLLMThreadStateManager::PopAssetTag()
{
	// look for already allocated thread state
	FLLMThreadState* State = GetState();

	checkf(State != nullptr, TEXT("Called PopScopeTag but PushScopeTag was never called!"));

	State->PopAssetTag();
}
#endif

void FLLMThreadStateManager::TrackAllocation(const void* Ptr, uint64 Size, ELLMScopeTag DefaultTag)
{
	if (IsPaused())
	{
		return;
	}

	// track the total quickly
	FPlatformAtomics::InterlockedAdd(&TrackedMemoryOverFrames, (int64)Size);
	
	FLLMThreadState* State = GetOrCreateState();
	
	// track on the thread state, and get the tag
	State->TrackAllocation(Size, DefaultTag);

	// tracking a nullptr with a Size is allowed, but we don't need to remember it, since we can't free it ever
	if (Ptr != nullptr)
	{
		// remember the size and scope info
		int64 tag = State->GetTopTag();
		if (tag == (int64)ELLMScopeTag::Untagged)
			tag = (int64)DefaultTag;
		FLLMThreadStateManager::FLowLevelAllocInfo AllocInfo = { Size, tag
#if LLM_ALLOW_ASSETS_TAGS
			, State->GetTopAssetTag()
#endif
		};

		GetAllocationMap().Add(Ptr, AllocInfo);
	}
}

void FLLMThreadStateManager::TrackFree(const void* Ptr, uint64 CheckSize)
{
	if (IsPaused())
	{
		return;
	}

	// @todo llm: This is needed only for an Xbox thing that is under discussion - very temporary need
#if PLATFORM_XBOXONE
	if (!GetAllocationMap().HasKey(Ptr))
	{
		FPlatformMisc::LowLevelOutputDebugStringf(TEXT("Attempted to free a pointer that wasn't tracked!! [%p]\n"), Ptr);
		return;
	}
#endif

	// look up the pointer in the tracking map
	FLLMThreadStateManager::FLowLevelAllocInfo AllocInfo = GetAllocationMap().Remove(Ptr);

	// track the total quickly
	FPlatformAtomics::InterlockedAdd(&TrackedMemoryOverFrames, 0 - AllocInfo.Size);

	FLLMThreadState* State = GetOrCreateState();

	State->TrackFree(AllocInfo.Tag, AllocInfo.Size, true);
#if LLM_ALLOW_ASSETS_TAGS
	State->TrackFree(AllocInfo.AssetTag, AllocInfo.Size, false);
#endif

	// CheckSize is just used to verify (at least for now)
	checkf(CheckSize == 0 || CheckSize == AllocInfo.Size, TEXT("Called LLM::OnFree with a Size, but it didn't match what was allocated? [allocated = %lld, passed in = %lld]"), AllocInfo.Size, CheckSize);
}

void FLLMThreadStateManager::OnAllocMoved(const void* Dest, const void* Source)
{
	FLLMThreadStateManager::FLowLevelAllocInfo AllocInfo = GetAllocationMap().Remove(Source);
	GetAllocationMap().Add(Dest, AllocInfo);
}

void FLLMThreadStateManager::TrackMemory(int64 Tag, int64 Amount)
{
	FLLMThreadStateManager::FLLMThreadState* State = GetOrCreateState();
	State->IncrTag(Tag, Amount, true);
	FPlatformAtomics::InterlockedAdd(&TrackedMemoryOverFrames, Amount);
}

// This will pause/unpause tracking, and also manually increment a given tag
void FLLMThreadStateManager::PauseAndTrackMemory(int64 Tag, int64 Amount)
{
	FLLMThreadStateManager::FLLMThreadState* State = GetOrCreateState();
	State->bPaused = true;
	State->IncrTag(Tag, Amount, true);
	FPlatformAtomics::InterlockedAdd(&TrackedMemoryOverFrames, Amount);
}

void FLLMThreadStateManager::Pause()
{
	FLLMThreadStateManager::FLLMThreadState* State = GetOrCreateState();
	State->bPaused = true;
}

void FLLMThreadStateManager::Unpause()
{
	GetOrCreateState()->bPaused = false;
}

bool FLLMThreadStateManager::IsPaused()
{
	FLLMThreadStateManager::FLLMThreadState* State = GetState();
	// pause during shutdown, as the massive number of frees is likely to overflow some of the buffers
	return GIsRequestingExit || (State == nullptr ? false : State->bPaused);
}

void FLLMThreadStateManager::Clear()
{
	AllocationMap.Clear();
}

void FLLMThreadStateManager::SetMemoryStatFNames(FName InUntaggedStatName, FName InTrackedStatName)
{
	UntaggedStatName = InUntaggedStatName;
	TrackedStatName = InTrackedStatName;
}

uint64 FLLMThreadStateManager::UpdateFrameAndReturnTotalTrackedMemory()
{
	// protect any accesses to the ThreadStates array
	FScopeLock Lock(&ThreadArraySection);

	for (int32 ThreadIndex = 0; ThreadIndex < NextThreadState; ThreadIndex++)
	{
		// update stat per thread states
		ThreadStates[ThreadIndex].UpdateFrame(UntaggedStatName, CsvWriter);
	}

	SET_MEMORY_STAT_FName(TrackedStatName, TrackedMemoryOverFrames);

	CsvWriter.SetStat(FNameToTag(TrackedStatName), TrackedMemoryOverFrames);
	CsvWriter.SetStat(FNameToTag(TEXT("AVAILABLE_PHYSICAL")), FPlatformMemory::GetStats().AvailablePhysical);

	return (uint64)TrackedMemoryOverFrames;
}

void FLLMThreadStateManager::WriteCsv()
{
	CsvWriter.Update();
}


FLLMThreadStateManager::FLLMThreadState::FLLMThreadState()
	: StackDepth(0)
#if LLM_ALLOW_ASSETS_TAGS
	, AssetStackDepth(0)
#endif
	, TaggedAllocCount(0)
	, UntaggedAllocs(0)
	, bPaused(false)
{
}

void FLLMThreadStateManager::FLLMThreadState::PushScopeTag(int64 Tag)
{
	FScopeLock Lock(&TagSection);

	checkf(StackDepth < LLM_MAX_THREAD_SCOPE_STACK_DEPTH, TEXT("LLM scoped tag depth exceeded LLM_MAX_THREAD_SCOPE_STACK_DEPTH; increase this value"));
	// push a tag
	TagStack[StackDepth++] = Tag;
}

void FLLMThreadStateManager::FLLMThreadState::PopScopeTag()
{
	FScopeLock Lock(&TagSection);

	checkf(StackDepth > 0, TEXT("Called FLLMThreadStateManager::FLLMThreadState::PopScopeTag without a matching Push (stack was empty on pop)"));
	StackDepth--;
}

int64 FLLMThreadStateManager::FLLMThreadState::GetTopTag()
{
	// make sure we have some pushed
	if (StackDepth == 0)
	{
		return (int64)ELLMScopeTag::Untagged;
	}

	// return the top tag
	return TagStack[StackDepth - 1];
}

#if LLM_ALLOW_ASSETS_TAGS
void FLLMThreadStateManager::FLLMThreadState::PushAssetTag(int64 Tag)
{
	FScopeLock Lock(&TagSection);

	checkf(AssetStackDepth < LLM_MAX_THREAD_SCOPE_STACK_DEPTH, TEXT("LLM scoped tag depth exceeded LLM_MAX_THREAD_SCOPE_STACK_DEPTH; increase this value"));
	// push a tag
	AssetTagStack[AssetStackDepth++] = Tag;
}

void FLLMThreadStateManager::FLLMThreadState::PopAssetTag()
{
	FScopeLock Lock(&TagSection);

	checkf(AssetStackDepth > 0, TEXT("Called FLLMThreadStateManager::FLLMThreadState::PopScopeTag without a matching Push (stack was empty on pop)"));
	AssetStackDepth--;
}

int64 FLLMThreadStateManager::FLLMThreadState::GetTopAssetTag()
{
	// make sure we have some pushed
	if (AssetStackDepth == 0)
	{
		return (int64)ELLMScopeTag::Untagged;
	}

	// return the top tag
	return AssetTagStack[AssetStackDepth - 1];
}
#endif

void FLLMThreadStateManager::FLLMThreadState::IncrTag(int64 Tag, int64 Amount, bool bTrackUntagged)
{
	// track the untagged allocations
	if (Tag == (int64)ELLMScopeTag::Untagged)
	{
		if (bTrackUntagged)
		{
			UntaggedAllocs += Amount;
		}
	}
	else
	{
		// look over existing tags on this thread for already tracking this tag
		for (int32 TagSearch = 0; TagSearch < TaggedAllocCount; TagSearch++)
		{
			if (TaggedAllocTags[TagSearch] == Tag)
			{
				// update it if we found it, and break out
				TaggedAllocs[TagSearch] += Amount;
				return;
			}
		}

		checkf(TaggedAllocCount < LLM_MAX_THREAD_SCOPE_TAGS, TEXT("Tried to use more tags on a thread than LLM_MAX_THREAD_SCOPE_TAGS; increase that value"));

		// if we get here, then we need to add a new tracked tag
		TaggedAllocTags[TaggedAllocCount] = Tag;
		TaggedAllocs[TaggedAllocCount] = Amount;
		TaggedAllocCount++;
	}
}

void FLLMThreadStateManager::FLLMThreadState::TrackAllocation(uint64 Size, ELLMScopeTag DefaultTag)
{
	FScopeLock Lock(&TagSection);

	int64 Tag = GetTopTag();
	if (Tag == (int64)ELLMScopeTag::Untagged)
		Tag = (int64)DefaultTag;
	IncrTag(Tag, Size, true);
#if LLM_ALLOW_ASSETS_TAGS
	int64 AssetTag = GetTopAssetTag();
	IncrTag(AssetTag, Size, false);
#endif
}

void FLLMThreadStateManager::FLLMThreadState::TrackFree(int64 Tag, uint64 Size, bool bTrackedUntagged)
{
	FScopeLock Lock(&TagSection);

	IncrTag(Tag, 0 - Size, bTrackedUntagged);
}

void FLLMThreadStateManager::FLLMThreadState::UpdateFrame(FName InUntaggedStatName, FLLMCsvWriter& InCsvWriter)
{
	// we can't call Malloc while TagSection is locked, as it could deadlock, and stats will Malloc
	FLLMThreadState LocalState;

	{
		FScopeLock Lock(&TagSection);

		LocalState.UntaggedAllocs = UntaggedAllocs;
		LocalState.TaggedAllocCount = TaggedAllocCount;
		FMemory::Memcpy(LocalState.TaggedAllocTags, TaggedAllocTags, sizeof(int64) * TaggedAllocCount);
		FMemory::Memcpy(LocalState.TaggedAllocs, TaggedAllocs, sizeof(int64) * TaggedAllocCount);

		// restart the tracking now that we've copied out, safely
		UntaggedAllocs = 0;
		TaggedAllocCount = 0;
	}

	INC_MEMORY_STAT_BY_FName(InUntaggedStatName, LocalState.UntaggedAllocs);

	InCsvWriter.AddStat(FNameToTag(InUntaggedStatName), LocalState.UntaggedAllocs);

	// walk over the tags for this level
	for (int32 TagIndex = 0; TagIndex < LocalState.TaggedAllocCount; TagIndex++)
	{
		int64 Tag = LocalState.TaggedAllocTags[TagIndex];
		int64 Amount = LocalState.TaggedAllocs[TagIndex];

		InCsvWriter.AddStat(Tag, Amount);

		if (Tag > (int64)ELLMScopeTag::MaxUserAllocation)
		{
			INC_MEMORY_STAT_BY_FName(TagToFName(Tag), Amount);
		}
		else
		{
			switch ((ELLMScopeTag)Tag)
			{
				case ELLMScopeTag::SmallBinnedAllocation:
					INC_MEMORY_STAT_BY(STAT_SmallBinnedMemory, Amount);
					break;
				case ELLMScopeTag::LargeBinnedAllocation:
					INC_MEMORY_STAT_BY(STAT_LargeBinnedMemory, Amount);
					break;
				case ELLMScopeTag::ThreadStack:
					INC_MEMORY_STAT_BY(STAT_ThreadStackMemory, Amount);
					break;
				case ELLMScopeTag::MallocPool:
					INC_MEMORY_STAT_BY(STAT_MallocPool, Amount);
					break;
				case ELLMScopeTag::Malloc:
					INC_MEMORY_STAT_BY(STAT_Malloc, Amount);
					break;
				case ELLMScopeTag::ProgramSizePlatform:
					INC_MEMORY_STAT_BY(STAT_ProgramSizePlatform, Amount);
					break;
				case ELLMScopeTag::ProgramSizeDefault:
					INC_MEMORY_STAT_BY(STAT_ProgramSizeDefault, Amount);
					break;
				case ELLMScopeTag::BackupOOMMemoryPoolPlatform:
					INC_MEMORY_STAT_BY(STAT_BackupOOMMemoryPoolPlatform, Amount);
					break;
				case ELLMScopeTag::BackupOOMMemoryPoolDefault:
					INC_MEMORY_STAT_BY(STAT_BackupOOMMemoryPoolDefault, Amount);
					break;
				case ELLMScopeTag::GenericPlatformMallocCrash:
					INC_MEMORY_STAT_BY(STAT_GenericPlatformMallocCrash, Amount);
					break;
				case ELLMScopeTag::GraphicsMisc:
					INC_MEMORY_STAT_BY(STAT_D3DPlatformLLM, Amount);
					break;
				case ELLMScopeTag::AudioMisc:
					INC_MEMORY_STAT_BY(STAT_AudioMisc, Amount);
					break;
				case ELLMScopeTag::SystemMisc:
					INC_MEMORY_STAT_BY(STAT_SystemMisc, Amount);
					break;
				case ELLMScopeTag::GameMisc:
					INC_MEMORY_STAT_BY(STAT_GameMisc, Amount);
					break;

				default:
					INC_MEMORY_STAT_BY(STAT_UserMemory, Amount);
			}
		}
	}

}

/*
 * FLLMCsvWriter implementation
*/

const double FLLMCsvWriter::WriteFrequency = 5.0;	// seconds (0 for every frame)

/*
* don't allocate memory in the constructor because it is called before allocators are setup
*/
FLLMCsvWriter::FLLMCsvWriter()
	: StatValueCount(0)
	, LastWriteTime(FPlatformTime::Seconds())
	, Archive(NULL)
	, LastWriteStatValueCount(0)
{
}

FLLMCsvWriter::~FLLMCsvWriter()
{
	delete Archive;
}

/*
* don't allocate memory in this function because it is called by the allocator
*/
void FLLMCsvWriter::AddStat(int64 Tag, int64 Value)
{
	FScopeLock lock(&StatValuesLock);

	for (int32 i = 0; i < StatValueCount; ++i)
	{
		if (StatValues[i].Tag == Tag)
		{
			StatValues[i].Value += Value;
			return;
		}
	}

	check(StatValueCount < MAxValueCount);		// increase FLLMCsvWriter::MAxValueCount
	StatValues[StatValueCount].Tag = Tag;
	StatValues[StatValueCount].Value = Value;
	++StatValueCount;
}

/*
* don't allocate memory in this function because it is called by the allocator
*/
void FLLMCsvWriter::SetStat(int64 Tag, int64 Value)
{
	FScopeLock lock(&StatValuesLock);

	for (int32 i = 0; i < StatValueCount; ++i)
	{
		if (StatValues[i].Tag == Tag)
		{
			StatValues[i].Value = Value;
			return;
		}
	}

	check(StatValueCount < MAxValueCount);		// increase FLLMCsvWriter::MAxValueCount
	StatValues[StatValueCount].Tag = Tag;
	StatValues[StatValueCount].Value = Value;
	++StatValueCount;
}

/*
* memory can be allocated in this function
*/
void FLLMCsvWriter::Update()
{
	double Now = FPlatformTime::Seconds();
	if (Now - LastWriteTime >= WriteFrequency)
	{
		WriteGraph();

		LastWriteTime = Now;
	}
}

const TCHAR* FLLMCsvWriter::GetTrackerCsvName(ELLMTracker InTracker)
{
	switch (InTracker)
	{
		case ELLMTracker::Default: return TEXT("LLM");
		case ELLMTracker::Platform: return TEXT("LLMPlatform");
		default: check(false); return TEXT("");
	}
}

/*
 * Archive is a binary stream, so we can't just serialise an FString using <<
*/
void FLLMCsvWriter::Write(const FString& Text)
{
	Archive->Serialize((void*)*Text, Text.Len() * sizeof(TCHAR));
}

/*
 * create the csv file on the first call. When it finds a new stat name it seeks
 * back to the start of the file and re-writes the column names.
*/
void FLLMCsvWriter::WriteGraph()
{
	// create the csv file
	if (!Archive)
	{
		FString Directory = FPaths::ProfilingDir() + "LLM/";
		IFileManager::Get().MakeDirectory(*Directory, true);
		
		const TCHAR* TrackerName = GetTrackerCsvName(Tracker);
		FString Filename = FString::Printf(TEXT("%s/%s.csv"), *Directory, TrackerName);
		Archive = IFileManager::Get().CreateFileWriter(*Filename);
		check(Archive);

		// create space for column titles that are filled in as we get them
		for (int32 i = 0; i < 500; ++i)
		{
			Write(TEXT("          "));
		}
		Write(TEXT("\n"));
	}

	// grab the stats (make sure that no allocations happen in this scope)
	int32 StatValueCountLocal = 0;
	{
		FScopeLock lock(&StatValuesLock);
		memcpy(StatValuesForWrite, StatValues, StatValueCount * sizeof(StatValue));
		StatValueCountLocal = StatValueCount;
	}

	// re-write the column names if we have found a new one
	if (StatValueCountLocal != LastWriteStatValueCount)
	{
		int64 OriginalOffset = Archive->Tell();
		Archive->Seek(0);

		for (int32 i = 0; i < StatValueCountLocal; ++i)
		{
			FString StatName = GetTagName(StatValuesForWrite[i].Tag);
			FString Text = FString::Printf(TEXT("%s,"), *StatName);
			Write(Text);
		}

		Archive->Seek(OriginalOffset);

		LastWriteStatValueCount = StatValueCountLocal;
	}

	// write the actual stats
	for (int32 i = 0; i < StatValueCountLocal; ++i)
	{
		FString Text = FString::Printf(TEXT("%0.2f,"), StatValues[i].Value / 1024.0f / 1024.0f);
		Write(Text);
	}
	Write(TEXT("\n"));

	Archive->Flush();
}

/*
 * convert a Tag to a string. If the Tag is actually a Stat then extract the name of the stat.
*/
FString FLLMCsvWriter::GetTagName(int64 Tag)
{
	if (Tag >= (int64)ELLMScopeTag::UserAllocationStart)
	{
		FString Name = TagToFName(Tag).ToString();

		// if it has a trible slash assume it is a Stat string and extract the descriptive name
		int32 StartIndex = Name.Find(TEXT("///"));
		if (StartIndex != -1)
		{
			StartIndex += 3;
			int32 EndIndex = Name.Find(TEXT("///"), ESearchCase::IgnoreCase, ESearchDir::FromStart, StartIndex);
			if (EndIndex != -1)
			{
				Name = Name.Mid(StartIndex, EndIndex - StartIndex);
			}
		}

		return Name;
	}
	else
	{
		switch ((ELLMScopeTag)Tag)
		{
			case ELLMScopeTag::SmallBinnedAllocation: return TEXT("SmallBinnedAllocation"); break;
			case ELLMScopeTag::LargeBinnedAllocation: return TEXT("LargeBinnedAllocation"); break;
			case ELLMScopeTag::ThreadStack: return TEXT("ThreadStack"); break;
			case ELLMScopeTag::MallocPool: return TEXT("MallocPool"); break;
			case ELLMScopeTag::Malloc: return TEXT("Malloc"); break;
			case ELLMScopeTag::ProgramSizePlatform: return TEXT("ProgramSize"); break;
			case ELLMScopeTag::ProgramSizeDefault: return TEXT("ProgramSize"); break;
			case ELLMScopeTag::BackupOOMMemoryPoolPlatform: return TEXT("BackupOOMMemoryPool"); break;
			case ELLMScopeTag::BackupOOMMemoryPoolDefault: return TEXT("BackupOOMMemoryPool"); break;
			case ELLMScopeTag::GenericPlatformMallocCrash: return TEXT("GenericPlatformMallocCrash"); break;
			case ELLMScopeTag::GraphicsMisc: return TEXT("Graphics Misc"); break;
			case ELLMScopeTag::AudioMisc: return TEXT("Audio Misc"); break;
			case ELLMScopeTag::SystemMisc: return TEXT("System Misc"); break;
			default: return TEXT("Missing Stat"); break;
		}
	}
}

#endif		// #if ENABLE_LOW_LEVEL_MEM_TRACKER

