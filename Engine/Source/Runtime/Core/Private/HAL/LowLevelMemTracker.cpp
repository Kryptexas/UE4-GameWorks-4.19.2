// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "HAL/LowLevelMemTracker.h"
#include "ScopeLock.h"

// @todo xbox memory: Can we use anything else here?!?!?! this thing uses malloc, which we kinda depend on for FUseSystemMallocForNew.. but yeah
#include "LowLevelMemoryMap.h"

#if ENABLE_LOW_LEVEL_MEM_TRACKER

DECLARE_MEMORY_STAT(TEXT("Untracked Process Memory"), STAT_UntrackedProcessMemory, STATGROUP_LLM);
DECLARE_MEMORY_STAT(TEXT("Tracked Process Memory"), STAT_TrackedProcessMemory, STATGROUP_LLM);
DECLARE_MEMORY_STAT(TEXT("Untracked Process Memory [Overtracked Amount]"), STAT_UntrackedProcessMemoryOverage, STATGROUP_LLM);
DECLARE_MEMORY_STAT(TEXT("Total Used Process Memory"), STAT_UsedProcessMemory, STATGROUP_LLM);
DECLARE_MEMORY_STAT(TEXT("Pointer Tracking Overhead"), STAT_PointerTrackingOverhead, STATGROUP_LLM);
DECLARE_MEMORY_STAT(TEXT("Thread Tracking Overhead"), STAT_ThreadTrackingOverhead, STATGROUP_LLM);

DECLARE_MEMORY_STAT(TEXT("Program Size"), STAT_ProgramSize, STATGROUP_LLMPlatform);
DECLARE_MEMORY_STAT(TEXT("Small Binned OS Memory"), STAT_SmallBinnedMemory, STATGROUP_LLMPlatform);
DECLARE_MEMORY_STAT(TEXT("Large (Non) Binned OS Memory"), STAT_LargeBinnedMemory, STATGROUP_LLMPlatform);
DECLARE_MEMORY_STAT(TEXT("Malloc Pool"), STAT_MallocPool, STATGROUP_LLMPlatform);
DECLARE_MEMORY_STAT(TEXT("Thread Stacks"), STAT_ThreadStackMemory, STATGROUP_LLMPlatform);
DECLARE_MEMORY_STAT(TEXT("User Memory"), STAT_UserMemory, STATGROUP_LLMPlatform);

DECLARE_MEMORY_STAT(TEXT("Untagged Platform Memory"), STAT_UntaggedPlatformMemory, STATGROUP_LLMPlatform);
DECLARE_MEMORY_STAT(TEXT("Tracked Platform Memory"), STAT_TrackedPlatformMemory, STATGROUP_LLMPlatform);
DECLARE_MEMORY_STAT(TEXT("Untagged Malloc Memory"), STAT_UntaggedMallocMemory, STATGROUP_LLMMalloc);
DECLARE_MEMORY_STAT(TEXT("Tracked Malloc Memory"), STAT_TrackedMallocMemory, STATGROUP_LLMMalloc);
DECLARE_MEMORY_STAT(TEXT("Untagged RHI Memory"), STAT_UntaggedRHIMemory, STATGROUP_LLMRHI);
DECLARE_MEMORY_STAT(TEXT("Tracked RHI Memory"), STAT_TrackedRHIMemory, STATGROUP_LLMRHI);


// pre-sized arrays for various resources so we don't need to allocate memory in the tracker
#define LLM_MAX_TRACKED_THREADS 64
#define LLM_MAX_THREAD_SCOPE_STACK_DEPTH 32
#if LLM_ALLOW_ASSETS_TAGS
	#if WITH_EDITOR
		#define LLM_MAX_THREAD_SCOPE_TAGS 4000
	#else
		#define LLM_MAX_THREAD_SCOPE_TAGS 2000
	#endif
#else
	#define LLM_MAX_THREAD_SCOPE_TAGS 64
#endif

class FLLMThreadStateManager
{
public:

	FLLMThreadStateManager();
	~FLLMThreadStateManager();

	void PushScopeTag(int64 Tag);
	void PopScopeTag();
#if LLM_ALLOW_ASSETS_TAGS
	void PushAssetTag(int64 Tag);
	void PopAssetTag();
#endif
	void TrackAllocation(const void* Ptr, uint64 Size);
	void TrackFree(const void* Ptr, uint64 CheckSize);

	// This will pause/unpause tracking, and also manually increment a given tag
	void PauseAndTrackMemory(int64 Tag, int64 Amount);
	void Pause();
	void Unpause();
	bool IsPaused();
	

	struct FLowLevelAllocInfo
	{
		uint64 Size;
		int64 Tag;
#if LLM_ALLOW_ASSETS_TAGS
		int64 AssetTag;
#endif
	};
	VirtualMap<PointerKey, FLowLevelAllocInfo>& GetAllocationMap()
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
		void TrackAllocation(uint64 Size);
		void TrackFree(int64 Tag, uint64 Size, bool bTrackedUntagged);
		void IncrTag(int64 Tag, int64 Amount, bool bTrackUntagged);

		void UpdateFrame(FName UntaggedStatName);

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

	VirtualMap<PointerKey, FLowLevelAllocInfo> AllocationMap;

	FName UntaggedStatName;
	FName TrackedStatName;
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
	checkf(NameIndex > (int64)ELLMScopeTag::MaxUserAllocation, TEXT("Passed with a name index [%d - %s] that was less than MemTracker_MaxUserAllocation"), NameIndex, *Name.ToString());

	// convert it to a tag, but you can actually convert this to an FMinimalName in the debugger to view it - *((FMinimalName*)&Tag)
	return (NameNumber << 32) | NameIndex;
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

FLowLevelMemTracker::FLowLevelMemTracker()
	: bFirstTimeUpdating(true)
	, bIsDisabled(false)
{
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
		static_assert((uint8)ELLMTracker::Max == 3, "You added a tracker, without updating FLowLevelMemTracker::UpdateStatsPerFrame (and probably need to update macros)");

		GetThreadManager(ELLMTracker::Platform).SetMemoryStatFNames(GET_STATFNAME(STAT_UntaggedPlatformMemory), GET_STATFNAME(STAT_TrackedPlatformMemory));
		GetThreadManager(ELLMTracker::Malloc).SetMemoryStatFNames(GET_STATFNAME(STAT_UntaggedMallocMemory), GET_STATFNAME(STAT_TrackedMallocMemory));
		GetThreadManager(ELLMTracker::RHI).SetMemoryStatFNames(GET_STATFNAME(STAT_UntaggedRHIMemory), GET_STATFNAME(STAT_TrackedRHIMemory));

		bFirstTimeUpdating = false;
	}

 	// calculate memory the platform thinks we have allocated, compared to what we have tracked, including the program memory
	FPlatformMemoryStats PlatformStats = FPlatformMemory::GetStats();
	uint64 PlatformProcessMemory = PlatformStats.TotalPhysical - PlatformStats.AvailablePhysical;

	int64 TrackedProcessMemory = 0;
	int64 Overhead = 0;
	for (int32 ManagerIndex = 0; ManagerIndex < (int32)ELLMTracker::Max; ManagerIndex++)
	{
		FLLMThreadStateManager& Manager = GetThreadManager((ELLMTracker)ManagerIndex);

		// update stats and also get how much memory is now tracked
		int64 TrackedMemory = Manager.UpdateFrameAndReturnTotalTrackedMemory();

		// count all overhead
		Overhead += Manager.GetAllocationMap().GetTotalMemoryUsed();

		// the Platform layer is special in that we use it to track untracked memory (it's expected that every other allocation
		// goes through this level, and if not, there's nothing better we can do)
		if (ManagerIndex == (int32)ELLMTracker::Platform)
		{
			TrackedProcessMemory = TrackedMemory;
		}
	}

	// update overhead stat
	SET_MEMORY_STAT(STAT_PointerTrackingOverhead, Overhead);
	SET_MEMORY_STAT(STAT_ThreadTrackingOverhead, sizeof(FLLMThreadStateManager) * (int32)ELLMTracker::Max);

	// untracked is physical - tracked 
	int64 UntrackedMemory = PlatformProcessMemory - (TrackedProcessMemory + Overhead);

	// update higher level stats
	SET_MEMORY_STAT(STAT_UntrackedProcessMemory, UntrackedMemory);
	SET_MEMORY_STAT(STAT_TrackedProcessMemory, TrackedProcessMemory + Overhead);
	
	if (UntrackedMemory < 0)
	{
		SET_MEMORY_STAT(STAT_UntrackedProcessMemoryOverage, -UntrackedMemory);
	}
	SET_MEMORY_STAT(STAT_UsedProcessMemory, PlatformProcessMemory);

	SET_MEMORY_STAT(STAT_ProgramSize, ProgramSize);

	if (LogName != nullptr)
	{
		FPlatformMisc::LowLevelOutputDebugStringf(TEXT("---> Untracked memory at %s = %.2f mb\n"), LogName, (double)UntrackedMemory / (1024.0 * 1024.0));
	}
}

void FLowLevelMemTracker::RefreshProgramSize()
{
	FPlatformMemoryStats Stats = FPlatformMemory::GetStats();
	ProgramSize = Stats.TotalPhysical - Stats.AvailablePhysical;
}

void FLowLevelMemTracker::SetProgramSize(uint64 InProgramSize)
{
	ProgramSize = InProgramSize;
}

void FLowLevelMemTracker::ProcessCommandLine(const TCHAR* CmdLine)
{
#if LLM_COMMANDLINE_ENABLES_FUNCTIONALITY
	// if we require commandline to enable it, then we are disabled if it's not there
	bIsDisabled = FParse::Param(CmdLine, TEXT("LLM")) == false;
#else
	// if we allow commandline to disable us, then we are disabled if it's there
	bIsDisabled = FParse::Param(CmdLine, TEXT("NOLLM")) == true;
#endif



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

void FLowLevelMemTracker::OnLowLevelAlloc(ELLMTracker Tracker, const void* Ptr, uint64 Size)
{
	if (bIsDisabled)
	{
		return;
	}

	GetThreadManager(Tracker).TrackAllocation(Ptr, Size);
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






FLLMScopedTag::FLLMScopedTag(FName StatIDName, ELLMTagSet Set)
{
	Init(FNameToTag(StatIDName), Set);
}

FLLMScopedTag::FLLMScopedTag(ELLMScopeTag ScopeTag, ELLMTagSet Set)
{
	Init((int64)ScopeTag, Set);
}

void FLLMScopedTag::Init(int64 Tag, ELLMTagSet Set)
{
	TagSet = Set;

	FLowLevelMemTracker& LLM = FLowLevelMemTracker::Get();
	if (!LLM.IsTagSetActive(TagSet))
	{
		return;
	}

	for (int32 ManagerIndex = 0; ManagerIndex < (int32)ELLMTracker::Max; ManagerIndex++)
	{
#if LLM_ALLOW_ASSETS_TAGS
		if (IsAssetTagForAssets(TagSet))
		{
			LLM.GetThreadManager((ELLMTracker)ManagerIndex).PushAssetTag(Tag);
		}
		else
#endif
		{
			LLM.GetThreadManager((ELLMTracker)ManagerIndex).PushScopeTag(Tag);
		}
	}
}

FLLMScopedTag::~FLLMScopedTag()
{
	FLowLevelMemTracker& LLM = FLowLevelMemTracker::Get();
	if (!LLM.IsTagSetActive(TagSet))
	{
		return;
	}

	for (int32 ManagerIndex = 0; ManagerIndex < (int32)ELLMTracker::Max; ManagerIndex++)
	{
#if LLM_ALLOW_ASSETS_TAGS
		if (IsAssetTagForAssets(TagSet))
		{
			LLM.GetThreadManager((ELLMTracker)ManagerIndex).PopAssetTag();
		}
		else
#endif
		{
			LLM.GetThreadManager((ELLMTracker)ManagerIndex).PopScopeTag();
		}
	}
}


FLLMScopedPauseTrackingWithAmountToTrack::FLLMScopedPauseTrackingWithAmountToTrack(FName StatIDName, int64 Amount, ELLMTracker TrackerForAmount)
{
	Init(FNameToTag(StatIDName), Amount, TrackerForAmount);
}

FLLMScopedPauseTrackingWithAmountToTrack::FLLMScopedPauseTrackingWithAmountToTrack(ELLMScopeTag ScopeTag, int64 Amount, ELLMTracker TrackerForAmount)
{
	Init((uint64)ScopeTag, Amount, TrackerForAmount);
}

void FLLMScopedPauseTrackingWithAmountToTrack::Init(int64 Tag, int64 Amount, ELLMTracker TrackerForAmount)
{
	FLowLevelMemTracker& LLM = FLowLevelMemTracker::Get();
	if (!LLM.IsTagSetActive(ELLMTagSet::None))
	{
		return;
	}

	for (int32 ManagerIndex = 0; ManagerIndex < (int32)ELLMTracker::Max; ManagerIndex++)
	{
		ELLMTracker Tracker = (ELLMTracker)ManagerIndex;

		// no amount means we can just to pause!
		if (Amount == 0 || TrackerForAmount != Tracker)
		{
			LLM.GetThreadManager((ELLMTracker)ManagerIndex).Pause();
		}
		else
		{
			LLM.GetThreadManager((ELLMTracker)ManagerIndex).PauseAndTrackMemory(Tag, Amount);
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

void FLLMThreadStateManager::TrackAllocation(const void* Ptr, uint64 Size)
{
	if (IsPaused())
	{
		return;
	}

	// track the total quickly
	FPlatformAtomics::InterlockedAdd(&TrackedMemoryOverFrames, (int64)Size);
	
	FLLMThreadState* State = GetOrCreateState();
	
	// track on the thread state, and get the tag
	State->TrackAllocation(Size);

	// tracking a nullptr with a Size is allowed, but we don't need to remember it, since we can't free it ever
	if (Ptr != nullptr)
	{
		// remember the size and scope info
		FLLMThreadStateManager::FLowLevelAllocInfo AllocInfo = { Size, State->GetTopTag()
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
		ThreadStates[ThreadIndex].UpdateFrame(UntaggedStatName);
	}

	SET_MEMORY_STAT_FName(TrackedStatName, TrackedMemoryOverFrames);
	return (uint64)TrackedMemoryOverFrames;
}






FLLMThreadStateManager::FLLMThreadState::FLLMThreadState()
	: StackDepth(0)
	, AssetStackDepth(0)
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

void FLLMThreadStateManager::FLLMThreadState::TrackAllocation(uint64 Size)
{
	FScopeLock Lock(&TagSection);

	int64 Tag = GetTopTag();
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

void FLLMThreadStateManager::FLLMThreadState::UpdateFrame(FName InUntaggedStatName)
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

	// walk over the tags for this level
	for (int32 TagIndex = 0; TagIndex < LocalState.TaggedAllocCount; TagIndex++)
	{
		int64 Tag = LocalState.TaggedAllocTags[TagIndex];
		int64 Amount = LocalState.TaggedAllocs[TagIndex];
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

				default:
					INC_MEMORY_STAT_BY(STAT_UserMemory, Amount);
			}
		}
	}

}


#endif



