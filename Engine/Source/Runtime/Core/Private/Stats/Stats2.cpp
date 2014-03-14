// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	UnStats.cpp: Performance stats framework.
=============================================================================*/


#include "CorePrivate.h"
#include "Stats2.h"

template<> uint32 FThreadSingleton<FThreadIdleStats>::TlsSlot = 0;

static struct FForceInitAtBootFThreadIdleStats : public TForceInitAtBoot<FThreadIdleStats>
{} FForceInitAtBootFThreadIdleStats;



/* Todo

FStatsThreadState::FStatsThreadState(FString const& Filename) - needs to be more careful about files that are "cut off". Should look for < 16 bytes left OR next 8 bytes are zero. Bad files will end with zeros and it can't be a valid record.

ST_int64 combined with IsPackedCCAndDuration is fairly bogus, ST_uint32_pair should probably be a first class type. IsPackedCCAndDuration can stay, it says what kind of data is in the pair. remove FromPackedCallCountDuration_Duration et al

averages stay as int64, it might be better if they were floats, but then we would probably want a ST_float_pair too.

//@todo merge these two after we have redone the user end

set a DEBUG_STATS define that allows this all to be debugged. Otherwise, inline and get rid of checks to the MAX. Also maybe just turn off stats in debug or debug game.

It should be possible to load stats data without STATS

Put version and FPlatformTime::GetSecondsPerCycle() in the header

//@todo this probably goes away after we redo the programmer interface

We are only saving condensed frames. For the "core view", we want a single frame capture of FStatsThreadState::History. That is the whole enchillada including start and end time for every scope.

//@todo Legacy API, phase out after we have changed the programmer-facing APU

stats2.h, stats2.cpp, statsrender2.cpp - get rid of the 2s.

//@todo, delegate under a global bool?

//@todo The rest is all horrid hacks to bridge the game between the old and the new system while we finish the new system

//@todo can we just use TIndirectArray here?

//@todo split header

//@todo metadata probably needs a different queue, otherwise it would be possible to load a module, run code in a thread and have the events arrive before the meta data

delete "wait for render commands", and generally be on the look out for stats that are never used, also stat system stuff

sweep INC type things for dump code that calls INC in a loop instead of just calling INC_BY once

FORCEINLINE_STATS void Start(FName InStatId)
{
check(InStatId != NAME_None);

^^^should be a checkstats


*/


#if STATS

#include "TaskGraphInterfaces.h"
#include "StatsData.h"

DEFINE_STAT(STAT_FrameTime);
DEFINE_STAT(STAT_FPS);
DEFINE_STAT(STAT_DrawStats);
DEFINE_STAT(STAT_Root);


DECLARE_DWORD_COUNTER_STAT(TEXT("Frame Packets Received"),STAT_StatFramePacketsRecv,STATGROUP_StatSystem);


FName TStatId::TStatId_NAME_None;

class FStartupMessages
{
public:
	TArray<FStatMessage> DelayedMessages;
	FCriticalSection CriticalSection;
	static FStartupMessages& Get()
	{
		static FStartupMessages* Messages = NULL;
		if (!Messages)
		{
			check(IsInGameThread());
			Messages = new FStartupMessages;
		}
		return *Messages;
	}
};

static struct FForceInitAtBootFStartupMessages : public TForceInitAtBoot<FStartupMessages>
{} FForceInitAtBootFStartupMessages;

void FThreadSafeStaticStatBase::DoSetup(const char* InStatName, const TCHAR* InStatDesc, const char* GroupName, const TCHAR* InGroupDesc, bool bDefaultEnable, bool bCanBeDisabled, EStatDataType::Type InStatType, bool bCycleStat, FPlatformMemory::EMemoryCounterRegion InMemoryRegion) const
{
	FName TempName(InStatName);

	{
		// send meta data, we don't use normal messages because the stats thread might not be running yet
		FScopeLock Lock(&FStartupMessages::Get().CriticalSection);
		new (FStartupMessages::Get().DelayedMessages) FStatMessage(GroupName, EStatDataType::ST_None, "Groups", InGroupDesc, false, false);
		new (FStartupMessages::Get().DelayedMessages) FStatMessage(InStatName, InStatType, GroupName, InStatDesc, bCanBeDisabled, bCycleStat, InMemoryRegion);
	}

	FName const* LocalHighPerformanceEnable(IStatGroupEnableManager::Get().GetHighPerformanceEnableForStat(FName(InStatName), GroupName, bDefaultEnable, bCanBeDisabled, InStatType, InStatDesc, bCycleStat, InMemoryRegion).GetRawPointer());
	FName const* OldHighPerformanceEnable = (FName const*)FPlatformAtomics::InterlockedCompareExchangePointer((void**)&HighPerformanceEnable, (void*)LocalHighPerformanceEnable, NULL);
	check(!OldHighPerformanceEnable || HighPerformanceEnable == OldHighPerformanceEnable); // we are assigned two different groups?
}

DEFINE_LOG_CATEGORY_STATIC(LogStatGroupEnableManager, Log, All);

class FStatGroupEnableManager : public IStatGroupEnableManager
{
	struct FGroupEnable
	{
		TMap<FName, FName *> NamesInThisGroup;
		TMap<FName, FName *> AlwaysEnabledNamesInThisGroup;
		bool DefaultEnable; 
		bool CurrentEnable; 

		FGroupEnable(bool InDefaultEnable)
			: DefaultEnable(InDefaultEnable)
			, CurrentEnable(InDefaultEnable)
		{
		}
	};

	enum 
	{
		NumPerBlock = 16384,
	};
	FCriticalSection SynchronizationObject;
	FName* PendingFNames;
	int32 PendingCount;
	TMap<FName, FGroupEnable> HighPerformanceEnable;

	// these control what happens to groups that haven't been registered yet
	bool EnableForNewGroups;
	bool UseEnableForNewGroups;
	TMap<FName, bool> EnableForNewGroup;


	void EnableStat(FName StatName, FName *DisablePtr)
	{
		// This is all complicated to ensure an atomic 8 byte write
		checkAtCompileTime(sizeof(FName) == sizeof(uint64), assumption_about_size_of_fname);
		check(UPTRINT(DisablePtr) % sizeof(FName) == 0);
		MS_ALIGN(8) struct FAligner
		{
			FName Temp;
		} GCC_ALIGN(8);
		FAligner Align;
		check(UPTRINT(&Align.Temp) % sizeof(FName) == 0);

		Align.Temp = StatName;
		*(uint64*)DisablePtr = *(uint64 const*)&Align.Temp;
	}

	void DisableStat(FName *DisablePtr)
	{
		checkAtCompileTime(sizeof(FName) == sizeof(uint64), assumption_about_size_of_fname);
		check(UPTRINT(DisablePtr) % sizeof(FName) == 0);
		*(uint64*)DisablePtr = 0;
	}

public:
	FStatGroupEnableManager()
		: PendingFNames(NULL)
		, PendingCount(0)
	{
		check(IsInGameThread());
	}

	virtual void SetHighPerformanceEnableForGroup(FName Group, bool Enable) OVERRIDE
	{
		FScopeLock ScopeLock(&SynchronizationObject);
		FThreadStats::MasterDisableChangeTagLockAdd();
		FGroupEnable* Found = HighPerformanceEnable.Find(Group);
		if (Found)
		{
			Found->CurrentEnable = Enable;
			if (Enable)
			{
				for (auto It = Found->NamesInThisGroup.CreateIterator(); It; ++It)
				{
					EnableStat(It.Key(), It.Value());
				}
			}
			else
			{
				for (auto It = Found->NamesInThisGroup.CreateIterator(); It; ++It)
				{
					DisableStat(It.Value());
				}
			}
		}
		FThreadStats::MasterDisableChangeTagLockSubtract();
	}

	virtual void SetHighPerformanceEnableForAllGroups(bool Enable) OVERRIDE
	{
		FScopeLock ScopeLock(&SynchronizationObject);
		FThreadStats::MasterDisableChangeTagLockAdd();
		for (auto It = HighPerformanceEnable.CreateIterator(); It; ++It)
		{
			It.Value().CurrentEnable = Enable;
			if (Enable)
			{
				for (auto ItInner = It.Value().NamesInThisGroup.CreateConstIterator(); ItInner; ++ItInner)
				{
					EnableStat(ItInner.Key(), ItInner.Value());
				}
			}
			else
			{
				for (auto ItInner = It.Value().NamesInThisGroup.CreateConstIterator(); ItInner; ++ItInner)
				{
					DisableStat(ItInner.Value());
				}
			}
		}
		FThreadStats::MasterDisableChangeTagLockSubtract();
	}
	virtual void ResetHighPerformanceEnableForAllGroups() OVERRIDE
	{
		FScopeLock ScopeLock(&SynchronizationObject);
		FThreadStats::MasterDisableChangeTagLockAdd();
		for (auto It = HighPerformanceEnable.CreateIterator(); It; ++It)
		{
			It.Value().CurrentEnable = It.Value().DefaultEnable;
			if (It.Value().DefaultEnable)
			{
				for (auto ItInner = It.Value().NamesInThisGroup.CreateConstIterator(); ItInner; ++ItInner)
				{
					EnableStat(ItInner.Key(), ItInner.Value());
				}
			}
			else
			{
				for (auto ItInner = It.Value().NamesInThisGroup.CreateConstIterator(); ItInner; ++ItInner)
				{
					DisableStat(ItInner.Value());
				}
			}
		}
		FThreadStats::MasterDisableChangeTagLockSubtract();
	}

	virtual TStatId GetHighPerformanceEnableForStat(FName StatShortName, const char* InGroup, bool bDefaultEnable, bool bCanBeDisabled, EStatDataType::Type InStatType, TCHAR const* InDescription, bool bCycleStat, FPlatformMemory::EMemoryCounterRegion MemoryRegion = FPlatformMemory::MCR_Invalid) OVERRIDE
	{
		FScopeLock ScopeLock(&SynchronizationObject);

		FName Group(InGroup);
		FStatNameAndInfo LongName(StatShortName, InGroup, InDescription, InStatType, bCanBeDisabled, bCycleStat, MemoryRegion);

		FName Stat = LongName.GetEncodedName();

		FGroupEnable* Found = HighPerformanceEnable.Find(Group);
		if (Found)
		{
			if (Found->DefaultEnable != bDefaultEnable)
			{
				UE_LOG(LogStatGroupEnableManager, Fatal, TEXT("Stat group %s was was defined both on and off by default."), *Group.ToString());
			}
			FName** StatFound = Found->NamesInThisGroup.Find(Stat);
			FName** StatFoundAlways = Found->AlwaysEnabledNamesInThisGroup.Find(Stat);
			if (StatFound)
			{
				if (StatFoundAlways)
				{
					UE_LOG(LogStatGroupEnableManager, Fatal, TEXT("Stat %s is both always enabled and not always enabled, so it was used for two different things."), *Stat.ToString());
				}
				return TStatId(*StatFound);
			}
			else if (StatFoundAlways)
			{
				return TStatId(*StatFoundAlways);
			}
		}
		else
		{
			Found = &HighPerformanceEnable.Add(Group, FGroupEnable(bDefaultEnable || !bCanBeDisabled));

			// this was set up before we saw the group, so set the enable now
			if (EnableForNewGroup.Contains(Group))
			{
				Found->CurrentEnable = EnableForNewGroup.FindChecked(Group);
				EnableForNewGroup.Remove(Group); // by definition, we will never need this again
			}
			else if (UseEnableForNewGroups)
			{
				Found->CurrentEnable = EnableForNewGroups;
			}
		}
		if (PendingCount < 1)
		{
			PendingFNames = new FName[NumPerBlock];
			PendingCount = NumPerBlock;
		}
		PendingCount--;
		FName* Result = PendingFNames++;
		if (Found->CurrentEnable)
		{
			EnableStat(Stat, Result);
		}
		if (bCanBeDisabled)
		{
			Found->NamesInThisGroup.Add(Stat, Result);
		}
		else
		{
			Found->AlwaysEnabledNamesInThisGroup.Add(Stat, Result);
		}
		return TStatId(Result);
	}

	void ListGroup(FName Group)
	{
		FGroupEnable* Found = HighPerformanceEnable.Find(Group);
		if (Found)
		{
			UE_LOG(LogStatGroupEnableManager, Display, TEXT("  %d  default %d %s"), !!Found->CurrentEnable, !!Found->DefaultEnable, *Group.ToString());
		}
	}

	void ListGroups(bool bDetailed = false)
	{
		for (auto It = HighPerformanceEnable.CreateConstIterator(); It; ++It)
		{
			UE_LOG(LogStatGroupEnableManager, Display, TEXT("  %d  default %d %s"), !!It.Value().CurrentEnable, !!It.Value().DefaultEnable, *(It.Key().ToString()));
			if (bDetailed)
			{
				for (auto ItInner = It.Value().NamesInThisGroup.CreateConstIterator(); ItInner; ++ItInner)
				{
					UE_LOG(LogStatGroupEnableManager, Display, TEXT("      %d %s"), !ItInner.Value()->IsNone(), *ItInner.Key().ToString());
				}
				for (auto ItInner = It.Value().AlwaysEnabledNamesInThisGroup.CreateConstIterator(); ItInner; ++ItInner)
				{
					UE_LOG(LogStatGroupEnableManager, Display, TEXT("      (always enabled) %s"), *ItInner.Key().ToString());
				}
			}
		}
	}

	FName CheckGroup(TCHAR const *& Cmd, bool Enable)
	{
		FString MaybeGroup;
		FParse::Token(Cmd, MaybeGroup, false);
		MaybeGroup = FString(TEXT("STATGROUP_")) + MaybeGroup;
		FName MaybeGroupFName(*MaybeGroup);

		FGroupEnable* Found = HighPerformanceEnable.Find(MaybeGroupFName);
		if (!Found)
		{
			EnableForNewGroup.Add(MaybeGroupFName, Enable);
			ListGroups();
			UE_LOG(LogStatGroupEnableManager, Display, TEXT("Group Not Found %s"), *MaybeGroupFName.ToString());
			return NAME_None;
		}
		SetHighPerformanceEnableForGroup(MaybeGroupFName, Enable);
		ListGroup(MaybeGroupFName);
		return MaybeGroupFName;
	}

	void StatGroupEnableManagerCommand(FString const& InCmd)
	{
		FScopeLock ScopeLock(&SynchronizationObject);
		const TCHAR* Cmd = *InCmd;
		if( FParse::Command(&Cmd,TEXT("list")) )
		{
			ListGroups();
		}
		else if( FParse::Command(&Cmd,TEXT("listall")) )
		{
			ListGroups(true);
		}
		else if ( FParse::Command(&Cmd,TEXT("enable")) )
		{
			CheckGroup(Cmd, true);
		}
		else if ( FParse::Command(&Cmd,TEXT("disable")) )
		{
			CheckGroup(Cmd, false);
		}
		else if ( FParse::Command(&Cmd,TEXT("none")) )
		{
			EnableForNewGroups = false;
			UseEnableForNewGroups = true;
			SetHighPerformanceEnableForAllGroups(false);
			ListGroups();
		}
		else if ( FParse::Command(&Cmd,TEXT("all")) )
		{
			EnableForNewGroups = true;
			UseEnableForNewGroups = true;
			SetHighPerformanceEnableForAllGroups(true);
			ListGroups();
		}
		else if ( FParse::Command(&Cmd,TEXT("default")) )
		{
			UseEnableForNewGroups = false;
			EnableForNewGroup.Empty();
			ResetHighPerformanceEnableForAllGroups();
			ListGroups();
		}
	}
};

IStatGroupEnableManager& IStatGroupEnableManager::Get()
{
	static IStatGroupEnableManager* Singleton = NULL;
	if (!Singleton)
	{
		verify(!FPlatformAtomics::InterlockedCompareExchangePointer((void**)&Singleton, (void*)new FStatGroupEnableManager, NULL));
	}
	return *Singleton;
}

static struct FForceInitAtBoot
{
	FForceInitAtBoot()
	{
		IStatGroupEnableManager::Get();
	}
} FForceInitAtBoot;


uint32 FThreadStats::TlsSlot = 0;
FThreadSafeCounter FThreadStats::MasterEnableCounter;
FThreadSafeCounter FThreadStats::MasterEnableUpdateNumber;
FThreadSafeCounter FThreadStats::MasterDisableChangeTagLock;
bool FThreadStats::bMasterEnable = false;
bool FThreadStats::bMasterDisableForever = false;


DECLARE_CYCLE_STAT(TEXT("StatsNew Tick"),STAT_StatsNewTick,STATGROUP_StatSystem);
DECLARE_CYCLE_STAT(TEXT("Parse Meta"),STAT_StatsNewParseMeta,STATGROUP_StatSystem);
DECLARE_CYCLE_STAT(TEXT("Add To History"),STAT_StatsNewAddToHistory,STATGROUP_StatSystem);

FName FStatNameAndInfo::ToLongName(FName InStatName, char const* InGroup, TCHAR const* InDescription)
{
	FString LongName;
	if (InGroup)
	{
		LongName += TEXT("//");
		LongName += InGroup;
		LongName += TEXT("//");
	}
	LongName += InStatName.ToString();
	if (InDescription)
	{
		LongName += TEXT("///");
		LongName += FStatsUtils::ToEscapedFString(InDescription);
		LongName += TEXT("///");
	}
	return FName(*LongName);
}

FName FStatNameAndInfo::GetShortNameFrom(FName InLongName)
{
	FString Input(InLongName.ToString());

	if (Input.StartsWith(TEXT("//")))
	{
		Input = Input.RightChop(2);
		int32 IndexEnd = Input.Find(TEXT("//"));
		if (IndexEnd == INDEX_NONE)
		{
			checkStats(0);
			return InLongName;
		}
		Input = Input.RightChop(IndexEnd + 2);
	}
	if (Input.EndsWith(TEXT("///")))
	{
		int32 Index = Input.Find(TEXT("///"));
		if (Index != INDEX_NONE)
		{
			Input = Input.Left(Index);
		}
	}
	return FName(*Input);
}

FName FStatNameAndInfo::GetGroupNameFrom(FName InLongName)
{
	FString Input(InLongName.ToString());

	if (Input.StartsWith(TEXT("//")))
	{
		Input = Input.RightChop(2);
		int32 IndexEnd = Input.Find(TEXT("//"));
		if (IndexEnd != INDEX_NONE)
		{
			return FName(*Input.Left(IndexEnd));
		}
		checkStats(0);
	}
	return NAME_None;
}

void FStatNameAndInfo::GetDescriptionFrom(FName InLongName, FString& OutDescription)
{
	OutDescription = TEXT("");
	FString Input(InLongName.ToString());

	if (Input.EndsWith(TEXT("///")))
	{
		Input = Input.LeftChop(3);
		int32 Index = Input.Find(TEXT("///"));
		if (Index != INDEX_NONE)
		{
			OutDescription = FStatsUtils::FromEscapedFString(*Input.RightChop(Index + 3));
		}
	}
}

uint32 StatsThreadId = 0;
static TAutoConsoleVariable<int32> CVarDumpStatPackets(	TEXT("DumpStatPackets"),0,	TEXT("If true, dump stat packets."));

/** The rendering thread runnable object. */
class FStatsThread : public FRunnable, FSingleThreadRunnable
{
	FStatPacketArray IncomingData; 
	FRunnableThread* Thread;
	FStatsThreadState& State;
	bool bReadyToProcess;
public:

	FStatsThread()
		: State(FStatsThreadState::GetLocalState())
		, bReadyToProcess(false)
	{
		check(IsInGameThread());
	}

	/**
	 * Returns a pointer to the single threaded interface when mulithreading is disabled.
	 */
	virtual FSingleThreadRunnable* GetSingleThreadInterface() OVERRIDE
	{
		return this;
	}

	virtual uint32 Run()
	{
		checkAtCompileTime(ENamedThreads::StatsThread == 0, delete_this_testing_a_build_fail);
		StatsThreadId = FPlatformTLS::GetCurrentThreadId();
		FTaskGraphInterface::Get().AttachToThread(ENamedThreads::StatsThread);
		FTaskGraphInterface::Get().ProcessThreadUntilRequestReturn(ENamedThreads::StatsThread);
		return 0;
	}

	virtual void Tick() OVERRIDE
	{
		static double LastTime = -1.0;
		if (bReadyToProcess && FPlatformTime::Seconds() - LastTime > .005) // we won't process more than every 5ms
		{
			SCOPE_CYCLE_COUNTER(STAT_StatsNewTick);
			bReadyToProcess = false;
			FStatPacketArray NowData; 
			Exchange(NowData.Packets, IncomingData.Packets);
			INC_DWORD_STAT_BY(STAT_StatFramePacketsRecv, NowData.Packets.Num());
			{
				TArray<FStatMessage> MetaMessages;
				{
					FScopeLock Lock(&FStartupMessages::Get().CriticalSection);
					Exchange(FStartupMessages::Get().DelayedMessages, MetaMessages);
				}
				if (MetaMessages.Num())
				{
					State.ProcessMetaDataOnly(MetaMessages);
				}
			}
			{
				SCOPE_CYCLE_COUNTER(STAT_StatsNewParseMeta);
				State.ScanForAdvance(NowData);
			}
			{
				SCOPE_CYCLE_COUNTER(STAT_StatsNewAddToHistory);
				State.AddToHistoryAndEmpty(NowData);
			}
			check(!NowData.Packets.Num());
			LastTime = FPlatformTime::Seconds();
		}
	}


	static FStatsThread& Get()
	{
		static FStatsThread Singleton;
		return Singleton;
	}

	void StatMessage(FStatPacket* Packet)
	{
		// the parallel case doesn't ever seem to be faster, so it disabled by default.
		if (CVarDumpStatPackets.GetValueOnAnyThread())
		{
			UE_LOG(LogTemp, Log, TEXT("Packet from %x"), Packet->ThreadId);
		}

		bReadyToProcess = Packet->ThreadType != EThreadType::Other;
		IncomingData.Packets.Add(Packet);
		Tick();
	}

	void Start()
	{
		Thread = FRunnableThread::Create(this, TEXT("StatsThread"), 0, 0, 128 * 1024, TPri_BelowNormal);
		check(Thread != NULL);
	}

};

// not using a delgate here to allow higher performance since we may end up sending a lot of small message arrays to the thread.
class FStatMessagesTask
{
	FStatPacket* Packet;
public:
	FStatMessagesTask(FStatPacket* InPacket)
		: Packet(InPacket)
	{
	}
	static const TCHAR* GetTaskName()
	{
		return TEXT("FStatMessagesTask");
	}
	FORCEINLINE static TStatId GetStatId()
	{
		return TStatId(); // we don't want to record this or it spams the stat system; we cover this time when we tick the stats system
	}
	static ENamedThreads::Type GetDesiredThread()
	{
		return FPlatformProcess::SupportsMultithreading() ? ENamedThreads::StatsThread : ENamedThreads::GameThread;
	}
	static ESubsequentsMode::Type GetSubsequentsMode() { return ESubsequentsMode::TrackSubsequents; }

	void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
	{
		FStatsThread::Get().StatMessage(Packet);
		Packet = NULL;
	}
};

FThreadStats::FThreadStats()
	: ScopeCount(0)
	, bSawExplicitFlush(false)
	, bWaitForExplicitFlush(0)
{
	Packet.ThreadId = FPlatformTLS::GetCurrentThreadId();
	if (Packet.ThreadId == GGameThreadId)
	{
		Packet.ThreadType = EThreadType::Game;
	}
	else if (Packet.ThreadId == GRenderThreadId)
	{
		Packet.ThreadType = EThreadType::Renderer;
	}
	else
	{
		Packet.ThreadType = EThreadType::Other;
	}
	check(TlsSlot);
	FPlatformTLS::SetTlsValue(TlsSlot, this);
}

// copy for sending to the stats thread
FThreadStats::FThreadStats(FThreadStats const& Other)
	: ScopeCount(0)
	, bSawExplicitFlush(false)
	, bWaitForExplicitFlush(0)
{
	Packet.ThreadId = Other.Packet.ThreadId;
	Packet.ThreadType = Other.Packet.ThreadType;
}

void FThreadStats::CheckEnable()
{
	bool bOldMasterEnable(bMasterEnable);
	bool bNewMasterEnable(WillEverCollectData() && !IsRunningCommandlet() && IsThreadingReady() && (MasterEnableCounter.GetValue() || GProfilerAttached));
	if (bMasterEnable != bNewMasterEnable)
	{
		MasterDisableChangeTagLockAdd();
		bMasterEnable = bNewMasterEnable;
		MasterDisableChangeTagLockSubtract();
	}
}

enum 
{
	MAX_PRESIZE = 256 * 1024,
};

void FThreadStats::Flush(bool bHasBrokenCallstacks)
{
	if (bMasterDisableForever)
	{
		Packet.StatMessages.Empty();
		return;
	}
	if (!ScopeCount && Packet.StatMessages.Num())
	{
		if (Packet.StatMessagesPresize.Num() >= 10)
		{
			Packet.StatMessagesPresize.RemoveAt(0);
		}
		if (Packet.StatMessages.Num() < MAX_PRESIZE)
		{
			Packet.StatMessagesPresize.Add(Packet.StatMessages.Num());
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("StatMessage Packet has more than 200,000 messages.  Ignoring for the presize history."));
		}
		FStatPacket* ToSend = new FStatPacket(Packet);
		Exchange(ToSend->StatMessages, Packet.StatMessages);
		ToSend->bBrokenCallstacks = bHasBrokenCallstacks;
		check(!Packet.StatMessages.Num());
		int32 MaxPresize = Packet.StatMessagesPresize[0];
		for (int32 Index = 0; Index < Packet.StatMessagesPresize.Num(); ++Index)
		{
			if (MaxPresize < Packet.StatMessagesPresize[Index])
			{
				MaxPresize = Packet.StatMessagesPresize[Index];
			}
		}
		Packet.StatMessages.Empty(MaxPresize);
		{
			TGraphTask<FStatMessagesTask>::CreateTask().ConstructAndDispatchWhenReady(ToSend);
		}
		if (Packet.ThreadType != EThreadType::Other && bSawExplicitFlush)
		{
			bWaitForExplicitFlush = 1;
			ScopeCount++; // prevent sends until the next explicit flush
		}
	}
}

void FThreadStats::ExplicitFlush(bool DiscardCallstack)
{
	FThreadStats* ThreadStats = GetThreadStats();
	check(ThreadStats->Packet.ThreadType != EThreadType::Other);
	if (ThreadStats->bWaitForExplicitFlush)
	{
		ThreadStats->ScopeCount--; // the main thread pre-incremented this to prevent stats from being sent. we send them at the next available opportunity
		ThreadStats->bWaitForExplicitFlush = 0;
	}
	bool bHasBrokenCallstacks = false;
	if (DiscardCallstack && ThreadStats->ScopeCount)
	{
		ThreadStats->ScopeCount = 0;
		bHasBrokenCallstacks = true;
	}
	ThreadStats->bSawExplicitFlush = true;
	ThreadStats->Flush(bHasBrokenCallstacks);
}

void FThreadStats::StartThread()
{
	FThreadStats::FrameDataIsIncomplete(); // make this non-zero
	check(IsInGameThread());
	check(!IsThreadingReady());
	FStatsThreadState::GetLocalState(); // start up the state
	FStatsThread::Get();
	FStatsThread::Get().Start();
	if (!TlsSlot)
	{
		TlsSlot = FPlatformTLS::AllocTlsSlot();
	}
	check(IsThreadingReady());
	CheckEnable();

	{
		FString CmdLine(FCommandLine::Get());
		FString StatCmds(TEXT("-StatCmds="));
		while (1)
		{
			FString Cmds;
			if (!FParse::Value(*CmdLine, *StatCmds, Cmds, false))
			{
				break;
			}
			TArray<FString> CmdsArray;
			Cmds.ParseIntoArray(&CmdsArray, TEXT( "," ), true);
			for (int32 Index = 0; Index < CmdsArray.Num(); Index++)
			{
				FString StatCmd = FString("stat ") + CmdsArray[Index].Trim();
				UE_LOG(LogStatGroupEnableManager, Log, TEXT("Sending Stat Command '%s'"), *StatCmd);
				DirectStatsCommand(*StatCmd);
			}
			int32 Index = CmdLine.Find(*StatCmds);
			ensure(Index >= 0);
			if (Index == INDEX_NONE)
			{
				break;
			}
			CmdLine = CmdLine.Mid(Index + StatCmds.Len());
		}
	}
	{
		if (FParse::Param(FCommandLine::Get(), TEXT("LoadTimeStats")))
		{
			DirectStatsCommand(TEXT("stat group enable LinkerLoad"));
			DirectStatsCommand(TEXT("stat group enable AsyncLoad"));
			DirectStatsCommand(TEXT("stat dumpsum -start -ms=250"));
		}
		if (FParse::Param(FCommandLine::Get(), TEXT("LoadTimeFile")))
		{
			DirectStatsCommand(TEXT("stat group enable LinkerLoad"));
			DirectStatsCommand(TEXT("stat group enable AsyncLoad"));
			DirectStatsCommand(TEXT("stat startfile"));
		}
	}

}

enum 
{
	MAX_STAT_LAG = 4,
};
static FGraphEventRef LastFramesEvents[MAX_STAT_LAG];
static int32 CurrentEventIndex = 0;

void FThreadStats::StopThread()
{
	// Nothing to stop if it was never started
	if (IsThreadingReady())
	{
		FThreadStats::MasterDisableForever();
		WaitForStats();
		for (int32 Index = 0; Index < MAX_STAT_LAG; Index++)
		{
			LastFramesEvents[Index] = NULL;
		}
		FGraphEventRef QuitTask = TGraphTask<FReturnGraphTask>::CreateTask(NULL, ENamedThreads::GameThread).ConstructAndDispatchWhenReady(FPlatformProcess::SupportsMultithreading() ? ENamedThreads::StatsThread : ENamedThreads::GameThread);
		FTaskGraphInterface::Get().WaitUntilTaskCompletes(QuitTask, ENamedThreads::GameThread_Local);	
	}
}

DECLARE_CYCLE_STAT(TEXT("WaitForStats"),STAT_WaitForStats,STATGROUP_Engine);

void FThreadStats::WaitForStats()
{
	check(IsInGameThread());
	if (IsThreadingReady() && !bMasterDisableForever)
	{
		{
			SCOPE_CYCLE_COUNTER(STAT_WaitForStats);
			if (LastFramesEvents[(CurrentEventIndex + MAX_STAT_LAG - 1) % MAX_STAT_LAG].GetReference())
			{
				FTaskGraphInterface::Get().WaitUntilTaskCompletes(LastFramesEvents[(CurrentEventIndex + MAX_STAT_LAG - 1) % MAX_STAT_LAG], ENamedThreads::GameThread_Local);
			}
		}
		LastFramesEvents[(CurrentEventIndex + MAX_STAT_LAG - 1) % MAX_STAT_LAG] = TGraphTask<FNullGraphTask>::CreateTask(NULL, ENamedThreads::GameThread).ConstructAndDispatchWhenReady(TEXT("StatWaitFence"), FPlatformProcess::SupportsMultithreading() ? ENamedThreads::StatsThread : ENamedThreads::GameThread);
		CurrentEventIndex++;
	}
}

#endif

