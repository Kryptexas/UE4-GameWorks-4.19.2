// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	TickTaskManager.cpp: Manager for ticking tasks
=============================================================================*/

#include "EnginePrivate.h"
#include "TickTaskManagerInterface.h"

DEFINE_LOG_CATEGORY_STATIC(LogTick, Log, All);

DECLARE_CYCLE_STAT(TEXT("Queue Ticks"),STAT_QueueTicks,STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("Queue Ticks Wait"),STAT_QueueTicksWait,STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("Queue Tick Task"),STAT_QueueTickTask,STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("Post Queue Tick Task"),STAT_PostTickTask,STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("Cooldown Dequeuing"),STAT_DequeueCooldowns,STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("Schedule cooldowns"),STAT_ScheduleCooldowns,STATGROUP_Game);
DECLARE_DWORD_COUNTER_STAT(TEXT("Ticks Queued"),STAT_TicksQueued,STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("TG_NewlySpawned"), STAT_TG_NewlySpawned, STATGROUP_TickGroups);

static TAutoConsoleVariable<int32> CVarLogTicks(
	TEXT("tick.LogTicks"),0,
	TEXT("Spew ticks for debugging."));

static TAutoConsoleVariable<int32> CVarLogTicksShowPrerequistes(
	TEXT("tick.ShowPrerequistes"),1,
	TEXT("When logging ticks, show the prerequistes; debugging."));

static TAutoConsoleVariable<int32> CVarAllowAsyncComponentTicks(
	TEXT("tick.AllowAsyncComponentTicks"),
	0,
	TEXT("Used to control async component ticks."));

struct FTickContext
{
	/** Delta time to tick **/
	float					DeltaSeconds;
	/** Tick type **/
	ELevelTick				TickType;
	/** Tick type **/
	ETickingGroup			TickGroup;
	/** Current or desired thread **/
	ENamedThreads::Type		Thread;

	FTickContext(float InDeltaSeconds = 0.0f, ELevelTick InTickType = LEVELTICK_All, ETickingGroup InTickGroup = TG_PrePhysics, ENamedThreads::Type InThread = ENamedThreads::GameThread)
		: DeltaSeconds(InDeltaSeconds)
		, TickType(InTickType)
		, TickGroup(InTickGroup)
		, Thread(InThread)
	{
	}

	FTickContext(const FTickContext& In)
		: DeltaSeconds(In.DeltaSeconds)
		, TickType(In.TickType)
		, TickGroup(In.TickGroup)
		, Thread(In.Thread)
	{
	}
	void operator=(const FTickContext& In)
	{
		DeltaSeconds = In.DeltaSeconds;
		TickType = In.TickType;
		TickGroup = In.TickGroup;
		Thread = In.Thread;
	}
};

/**
 * Class that handles the actual tick tasks and starting and completing tick groups
 */
class FTickTaskSequencer
{
	/** Helper class define the task of ticking a component **/
	class FTickFunctionTask
	{
		/** Actor to tick **/
		FTickFunction*			Target;
		/** tick context, here thread is desired execution thread **/
		FTickContext			Context;
		/** If true, log each tick **/
		bool					bLogTick; 
		/** If true, log prereqs **/
		bool					bLogTicksShowPrerequistes; 
	public:
		/** Constructor
		 * @param InTarget - Function to tick
		 * @param InContext - context to tick in, here thread is desired execution thread
		**/
		FORCEINLINE FTickFunctionTask(FTickFunction* InTarget, const FTickContext* InContext, bool InbLogTick, bool bInLogTicksShowPrerequistes)
			: Target(InTarget)
			, Context(*InContext)
			, bLogTick(InbLogTick)
			, bLogTicksShowPrerequistes(bInLogTicksShowPrerequistes)
		{
		}
		FORCEINLINE TStatId GetStatId() const
		{
			RETURN_QUICK_DECLARE_CYCLE_STAT(FTickFunctionTask, STATGROUP_TaskGraphTasks);
		}
		/** return the thread for this task **/
		FORCEINLINE ENamedThreads::Type GetDesiredThread()
		{
			if (Target->bHighPriority)
			{
				return ENamedThreads::HiPri(Context.Thread);
			}
			return Context.Thread;
		}
		FORCEINLINE static ESubsequentsMode::Type GetSubsequentsMode() 
		{ 
			return ESubsequentsMode::TrackSubsequents; 
		}
		/** 
		 *	Actually execute the tick.
		 *	@param	CurrentThread; the thread we are running on
		 *	@param	MyCompletionGraphEvent; my completion event. Not always useful since at the end of DoWork, you can assume you are done and hence further tasks do not need you as a prerequisite. 
		 *	However, MyCompletionGraphEvent can be useful for passing to other routines or when it is handy to set up subsequents before you actually do work.
		 **/
		void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
		{
			if (bLogTick)
			{
				UE_LOG(LogTick, Log, TEXT("tick %s %6d %2d %s"), Target->bHighPriority ? TEXT("*") : TEXT(" "),GFrameCounter, (int32)CurrentThread, *Target->DiagnosticMessage());
				if (bLogTicksShowPrerequistes)
				{
					Target->ShowPrerequistes();
				}
			}
			Target->ExecuteTick(Context.DeltaSeconds, Context.TickType, CurrentThread, MyCompletionGraphEvent);
			Target->CompletionHandle = NULL; // Allow the old completion handle to be recycled
		}
	};


	/** Completion handles for each phase of ticks */
	FGraphEventArray	TickCompletionEvents[TG_MAX];

	/** HiPri Held tasks for each tick group. */
	TArray<TGraphTask<FTickFunctionTask>*> HiPriTickTasks[TG_MAX];

	/** LowPri Held tasks for each tick group. */
	TArray<TGraphTask<FTickFunctionTask>*> TickTasks[TG_MAX];

	/** These are waited for at the end of the frame; they are not on the critical path, but they have to be done before we leave the frame. */
	FGraphEventArray CleanupTasks;

	/** we keep track of the last TG we have blocked for so when we do block, we know which TG's to wait for . */
	ETickingGroup WaitForTickGroup;

	/** If true, allow concurrent ticks **/
	bool				bAllowConcurrentTicks; 

	/** If true, log each tick **/
	bool				bLogTicks; 
	/** If true, log each tick **/
	bool				bLogTicksShowPrerequistes; 

public:

	/**
	 * Singleton to retrieve the global tick task sequencer
	 * @return Reference to the global tick task sequencer
	**/
	static FTickTaskSequencer& Get()
	{
		static FTickTaskSequencer SingletonInstance;
		return SingletonInstance;
	}
	/**
	 * Return true if we should be running in single threaded mode, ala dedicated server
	**/
	FORCEINLINE static bool SingleThreadedMode()
	{
		if (IsRunningDedicatedServer() || FPlatformMisc::NumberOfCores() < 3)
		{
			return true;
		}
		return false;
	}
	/**
	 * Start a component tick task
	 *
	 * @param	InPrerequisites - prerequisites that must be completed before this tick can begin
	 * @param	TickFunction - the tick function to queue
	 * @param	Context - tick context to tick in. Thread here is the current thread.
	 */
	FORCEINLINE void StartTickTask(const FGraphEventArray* Prerequisites, FTickFunction* TickFunction, const FTickContext& TickContext)
	{
		checkSlow(TickFunction->ActualTickGroup >=0 && TickFunction->ActualTickGroup < TG_MAX);

		FTickContext UseContext = TickContext;
	   
		bool bIsOriginalTickGroup = (TickFunction->ActualTickGroup == TickFunction->TickGroup);

		UseContext.Thread = ENamedThreads::GameThread;
		if (TickFunction->bRunOnAnyThread && bAllowConcurrentTicks && bIsOriginalTickGroup)
		{
			UseContext.Thread = ENamedThreads::AnyThread;
		}
		QUICK_SCOPE_CYCLE_COUNTER(STAT_StartTickTask_ConstructAndDispatchWhenReady);

		TGraphTask<FTickFunctionTask>* Task = TGraphTask<FTickFunctionTask>::CreateTask(Prerequisites, TickContext.Thread).ConstructAndHold(TickFunction, &UseContext, bLogTicks, bLogTicksShowPrerequistes);
		if (TickFunction->bHighPriority)
		{
			HiPriTickTasks[TickFunction->ActualTickGroup].Add(Task);
		}
		else
		{
			TickTasks[TickFunction->ActualTickGroup].Add(Task);
		}
		TickFunction->CompletionHandle = Task->GetCompletionEvent();
	}

	/** Add a completion handle to a tick group **/
	FORCEINLINE void AddTickTaskCompletion(ETickingGroup TickGroup, const FGraphEventRef& CompletionHandle)
	{
		checkSlow(TickGroup >=0 && TickGroup < TG_MAX);
		new (TickCompletionEvents[TickGroup]) FGraphEventRef(CompletionHandle);
	}
	/**
	 * Start a component tick task and add the completion handle
	 *
	 * @param	InPrerequisites - prerequisites that must be completed before this tick can begin
	 * @param	TickFunction - the tick function to queue
	 * @param	Context - tick context to tick in. Thread here is the current thread.
	 */
	FORCEINLINE void QueueTickTask(const FGraphEventArray* Prerequisites, FTickFunction* TickFunction, const FTickContext& TickContext)
	{
		checkSlow(TickContext.Thread == ENamedThreads::GameThread);
		StartTickTask(Prerequisites, TickFunction, TickContext);
		AddTickTaskCompletion(TickFunction->ActualTickGroup, TickFunction->CompletionHandle);
	}

	/** 
	 * Release the queued ticks for a given tick group and process them. 
	 * @param WorldTickGroup - tick group to release
	 * @param bBlockTillComplete - if true, do not return until all ticks are complete
	**/
	void ReleaseTickGroup(ETickingGroup WorldTickGroup, bool bBlockTillComplete)
	{
		if (bLogTicks)
		{
			UE_LOG(LogTick, Log, TEXT("tick %6d ---------------------------------------- Release tick group %d"),GFrameCounter, (int32)WorldTickGroup);
		}
		checkSlow(WorldTickGroup >=0 && WorldTickGroup < TG_MAX);

		if (SingleThreadedMode())
		{
			DispatchTickGroupInner(ENamedThreads::GameThread, WorldTickGroup);
		}
		else
		{
			DECLARE_CYCLE_STAT(TEXT("FDelegateGraphTask.DispatchTickGroup"), STAT_FDelegateGraphTask_DispatchTickGroup, STATGROUP_TaskGraphTasks);

			// dispatch the tick group on another thread, that way, the game thread can be processing ticks while ticks are being queued by another thread
			QUICK_SCOPE_CYCLE_COUNTER(STAT_ReleaseTickGroup);
			FTaskGraphInterface::Get().WaitUntilTaskCompletes(
				FDelegateGraphTask::CreateAndDispatchWhenReady(
					FDelegateGraphTask::FDelegate::CreateRaw(this, &FTickTaskSequencer::DispatchTickGroup, WorldTickGroup),
					GET_STATID(STAT_FDelegateGraphTask_DispatchTickGroup), 
					nullptr,
					ENamedThreads::GameThread, 
					ENamedThreads::HiPri(ENamedThreads::AnyThread)),
				ENamedThreads::HiPri(ENamedThreads::GameThread));
		}

		if (bBlockTillComplete || SingleThreadedMode())
		{
			QUICK_SCOPE_CYCLE_COUNTER(STAT_ReleaseTickGroup_Block);
			for (ETickingGroup Block = WaitForTickGroup; Block <= WorldTickGroup; Block = ETickingGroup(Block + 1))
			{
				if (TickCompletionEvents[Block].Num())
				{
					FTaskGraphInterface::Get().WaitUntilTasksComplete(TickCompletionEvents[Block], ENamedThreads::GameThread);
					if (SingleThreadedMode() || WorldTickGroup == TG_NewlySpawned)
					{
						ResetTickGroupInner(Block);
					}
					else
					{
						DECLARE_CYCLE_STAT(TEXT("FDelegateGraphTask.ResetTickGroup"), STAT_FDelegateGraphTask_ResetTickGroup, STATGROUP_TaskGraphTasks);

						CleanupTasks.Add(FDelegateGraphTask::CreateAndDispatchWhenReady(
							FDelegateGraphTask::FDelegate::CreateRaw(this, &FTickTaskSequencer::ResetTickGroup, Block),
							GET_STATID(STAT_FDelegateGraphTask_ResetTickGroup)));
					}
				}
			}
			WaitForTickGroup = ETickingGroup(WorldTickGroup + (WorldTickGroup == TG_NewlySpawned ? 0 : 1)); // don't advance for newly spawned
		}
		else
		{
			// since this is used to soak up some async time for another task (physics), we should process whatever we have now
			FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::GameThread);
			check(WorldTickGroup + 1 < TG_MAX && WorldTickGroup != TG_NewlySpawned); // you must block on the last tick group! And we must block on newly spawned
		}
	}

	/**
	 * Resets the internal state of the object at the start of a frame
	 */
	void StartFrame()
	{
		bLogTicks = !!CVarLogTicks.GetValueOnGameThread();
		bLogTicksShowPrerequistes = !!CVarLogTicksShowPrerequistes.GetValueOnGameThread();

		if (bLogTicks)
		{
			UE_LOG(LogTick, Log, TEXT("tick %6d ---------------------------------------- Start Frame"),GFrameCounter);
		}

		if (SingleThreadedMode())
		{
			bAllowConcurrentTicks = false;
		}
		else
		{
			bAllowConcurrentTicks = !!CVarAllowAsyncComponentTicks.GetValueOnGameThread();
		}
		for (int32 Index = 0; Index < TG_MAX; Index++)
		{
			check(!TickCompletionEvents[Index].Num());  // we should not be adding to these outside of a ticking proper and they were already cleared after they were ticked
			TickCompletionEvents[Index].Reset();
			check(!TickTasks[Index].Num() && !HiPriTickTasks[Index].Num());  // we should not be adding to these outside of a ticking proper and they were already cleared after they were ticked
			TickTasks[Index].Reset();
			HiPriTickTasks[Index].Reset();
		}
		WaitForTickGroup = (ETickingGroup)0;
	}
	/**
	 * Checks that everything is clean at the end of a frame
	 */
	void EndFrame()
	{
		if (bLogTicks)
		{
			UE_LOG(LogTick, Log, TEXT("tick %6d ---------------------------------------- End Frame"),GFrameCounter);
		}
		FTaskGraphInterface::Get().WaitUntilTasksComplete(CleanupTasks, ENamedThreads::GameThread);
		CleanupTasks.Reset();
		for (int32 Index = 0; Index < TG_MAX; Index++)
		{
			check(!TickCompletionEvents[Index].Num());  // we should not be adding to these outside of a ticking proper and they were already cleared after they were ticked
			check(!TickTasks[Index].Num() && !HiPriTickTasks[Index].Num());  // we should not be adding to these outside of a ticking proper and they were already cleared after they were ticked
		}
	}
private:

	FTickTaskSequencer()
		: bAllowConcurrentTicks(false)
		, bLogTicks(false)
		, bLogTicksShowPrerequistes(false)
	{
	}

	void ResetTickGroupInner(ETickingGroup WorldTickGroup)
	{
		TickCompletionEvents[WorldTickGroup].Reset();
	}
	void ResetTickGroup(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent, ETickingGroup WorldTickGroup)
	{
		ResetTickGroupInner(WorldTickGroup);
	}

	void DispatchTickGroupInner(ENamedThreads::Type CurrentThread, ETickingGroup WorldTickGroup)
	{
		{
			TArray<TGraphTask<FTickFunctionTask>*>& TickArray = HiPriTickTasks[WorldTickGroup];
			for (int32 Index = 0; Index < TickArray.Num(); Index++)
			{
				TickArray[Index]->Unlock(CurrentThread);
			}
			TickArray.Reset();
		}
		{
			TArray<TGraphTask<FTickFunctionTask>*>& TickArray = TickTasks[WorldTickGroup];
			for (int32 Index = 0; Index < TickArray.Num(); Index++)
			{
				TickArray[Index]->Unlock(CurrentThread);
			}
			TickArray.Reset();
		}
	}

	void DispatchTickGroup(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent, ETickingGroup WorldTickGroup)
	{
		DispatchTickGroupInner(CurrentThread, WorldTickGroup);
	}

};


class FTickTaskLevel
{
public:
	/** Constructor, grabs, the sequencer singleton **/
	FTickTaskLevel()
		: TickTaskSequencer(FTickTaskSequencer::Get())
		, bTickNewlySpawned(false)
	{
	}
	~FTickTaskLevel()
	{
		for (TSet<FTickFunction*>::TIterator It(AllEnabledTickFunctions); It; ++It)
		{
			(*It)->bRegistered = false;
		}
		for (TSet<FTickFunction*>::TIterator It(AllDisabledTickFunctions); It; ++It)
		{
			(*It)->bRegistered = false;
		}
		FTickFunction* CoolingDownNode = AllCoolingDownTickFunctions.Head;
		while (CoolingDownNode)
		{
			CoolingDownNode->bRegistered = false;
			CoolingDownNode = CoolingDownNode->Next;
		}
	}

	/**
	 * Queues the ticks for this level
	 * @param InContext - information about the tick
	 * @return the total number of ticks we will be ticking
	 */
	int32 StartFrame(const FTickContext& InContext)
	{
		check(!NewlySpawnedTickFunctions.Num()); // There shouldn't be any in here at this point in the frame
		Context.TickGroup = TG_PrePhysics; // reset this to the start tick group
		Context.DeltaSeconds = InContext.DeltaSeconds;
		Context.TickType = InContext.TickType;
		Context.Thread = ENamedThreads::GameThread;
		bTickNewlySpawned = true;

		int32 CooldownTicksEnabled = 0;
		{
			SCOPE_CYCLE_COUNTER(STAT_DequeueCooldowns);
			// Determine which cooled down ticks will be enabled this frame
			float CumulativeCooldown = 0.f;
			FTickFunction* TickFunction = AllCoolingDownTickFunctions.Head;
			while (TickFunction)
			{
				if (CumulativeCooldown + TickFunction->RelativeTickCooldown >= Context.DeltaSeconds)
				{
					TickFunction->RelativeTickCooldown -= (Context.DeltaSeconds - CumulativeCooldown);
					break;
				}
				CumulativeCooldown += TickFunction->RelativeTickCooldown;

				TickFunction->TickState = FTickFunction::ETickState::Enabled;
				TickFunction = TickFunction->Next;
				++CooldownTicksEnabled;
			}
		}

		return AllEnabledTickFunctions.Num() + CooldownTicksEnabled;
	}

	struct FTickScheduleDetails
	{
		FTickScheduleDetails(FTickFunction* InTickFunction, const float InCooldown)
			: TickFunction(InTickFunction)
			, Cooldown(InCooldown)
		{}

		FTickFunction* TickFunction;
		float Cooldown;
	};

	/* Puts a TickFunction in to the cooldown state*/
	void ScheduleTickFunctionCooldowns(TArray<FTickScheduleDetails>& TickFunctionsToSchedule)
	{
		SCOPE_CYCLE_COUNTER(STAT_ScheduleCooldowns);

		TickFunctionsToSchedule.Sort([](const FTickScheduleDetails& A, const FTickScheduleDetails& B)
		{
			return A.Cooldown < B.Cooldown;
		});

		int32 RescheduleIndex = 0;
		float CumulativeCooldown = 0.f;
		FTickFunction* PrevComparisonTickFunction = nullptr;
		FTickFunction* ComparisonTickFunction = AllCoolingDownTickFunctions.Head;
		while (ComparisonTickFunction && RescheduleIndex < TickFunctionsToSchedule.Num())
		{
			const float CooldownTime = TickFunctionsToSchedule[RescheduleIndex].Cooldown;
			if ((CumulativeCooldown + ComparisonTickFunction->RelativeTickCooldown) > CooldownTime)
			{
				FTickFunction* TickFunction = TickFunctionsToSchedule[RescheduleIndex].TickFunction;
				TickFunction->TickState = FTickFunction::ETickState::CoolingDown;
				TickFunction->RelativeTickCooldown = CooldownTime - CumulativeCooldown;

				if (PrevComparisonTickFunction)
				{
					PrevComparisonTickFunction->Next = TickFunction;
				}
				else
				{
					check(ComparisonTickFunction == AllCoolingDownTickFunctions.Head);
					AllCoolingDownTickFunctions.Head = TickFunction;
				}
				TickFunction->Next = ComparisonTickFunction;
				PrevComparisonTickFunction = TickFunction;
				ComparisonTickFunction->RelativeTickCooldown -= TickFunction->RelativeTickCooldown;
				CumulativeCooldown += TickFunction->RelativeTickCooldown;
				++RescheduleIndex;
			}
			else
			{
				CumulativeCooldown += ComparisonTickFunction->RelativeTickCooldown;
				PrevComparisonTickFunction = ComparisonTickFunction;
				ComparisonTickFunction = ComparisonTickFunction->Next;
			}
		}
		for ( ; RescheduleIndex < TickFunctionsToSchedule.Num(); ++RescheduleIndex)
		{
			FTickFunction* TickFunction = TickFunctionsToSchedule[RescheduleIndex].TickFunction;
			const float CooldownTime = TickFunctionsToSchedule[RescheduleIndex].Cooldown;

			TickFunction->TickState = FTickFunction::ETickState::CoolingDown;
			TickFunction->RelativeTickCooldown = CooldownTime - CumulativeCooldown;

			TickFunction->Next = nullptr;
			if (PrevComparisonTickFunction)
			{
				PrevComparisonTickFunction->Next = TickFunction;
			}
			else
			{
				check(ComparisonTickFunction == AllCoolingDownTickFunctions.Head);
				AllCoolingDownTickFunctions.Head = TickFunction;
			}
			PrevComparisonTickFunction = TickFunction;

			CumulativeCooldown += TickFunction->RelativeTickCooldown;
		}
	}

	/* Queue all tick functions for execution */
	void QueueAllTicks()
	{
		FTickTaskSequencer& TTS = FTickTaskSequencer::Get();
		for (TSet<FTickFunction*>::TIterator It(AllEnabledTickFunctions); It; ++It)
		{
			FTickFunction* TickFunction = *It;
			TickFunction->QueueTickFunction(TTS, Context);

			if (TickFunction->TickInterval > 0.f)
			{
				It.RemoveCurrent();
				TickFunctionsToReschedule.Add(FTickScheduleDetails(TickFunction, TickFunction->TickInterval));
			}
		}
		int32 EnabledCooldownTicks = 0;
		float CumulativeCooldown = 0.f;
		while (FTickFunction* TickFunction = AllCoolingDownTickFunctions.Head)
		{
			if (TickFunction->TickState == FTickFunction::ETickState::Enabled)
			{
				CumulativeCooldown += TickFunction->RelativeTickCooldown;
				TickFunction->QueueTickFunction(TTS, Context);
				TickFunctionsToReschedule.Add(FTickScheduleDetails(TickFunction, TickFunction->TickInterval - (Context.DeltaSeconds - CumulativeCooldown))); // Give credit for any overrun
				AllCoolingDownTickFunctions.Head = TickFunction->Next;
			}
			else
			{
				break;
			}
		}

		if (TickFunctionsToReschedule.Num() > 0)
		{
			ScheduleTickFunctionCooldowns(TickFunctionsToReschedule);
			TickFunctionsToReschedule.Reset();
		}
	}
	/**
	 * Queues the newly spawned ticks for this level
	 * @return - the number of items 
	 */
	int32 QueueNewlySpawned(ETickingGroup CurrentTickGroup)
	{
		Context.TickGroup = CurrentTickGroup;
		int32 Num = 0;
		FTickTaskSequencer& TTS = FTickTaskSequencer::Get();
		for (TSet<FTickFunction*>::TIterator It(NewlySpawnedTickFunctions); It; ++It)
		{
			FTickFunction* TickFunction = *It;
			TickFunction->QueueTickFunction(TTS, Context);
			Num++;

			if (TickFunction->TickInterval > 0.f)
			{
				AllEnabledTickFunctions.Remove(TickFunction);
				TickFunctionsToReschedule.Add(FTickScheduleDetails(TickFunction, TickFunction->TickInterval));
			}
		}
		if (TickFunctionsToReschedule.Num() > 0)
		{
			ScheduleTickFunctionCooldowns(TickFunctionsToReschedule);
			TickFunctionsToReschedule.Reset();
		}
		NewlySpawnedTickFunctions.Empty();
		return Num;
	}

	/**
	 * Run all of the ticks for a pause frame synchronously on the game thread.
	 * The capability of pause ticks are very limited. There are no dependencies or ordering or tick groups.
	 * @param InContext - information about the tick
	 */
	void RunPauseFrame(const FTickContext& InContext)
	{
		check(!NewlySpawnedTickFunctions.Num()); // There shouldn't be any in here at this point in the frame
		
		float CumulativeCooldown = 0.f;
		FTickFunction* PrevTickFunction = nullptr;
		FTickFunction* TickFunction = AllCoolingDownTickFunctions.Head;
		while (TickFunction)
		{
			CumulativeCooldown += TickFunction->RelativeTickCooldown;
			if (TickFunction->bTickEvenWhenPaused)
			{
				if (CumulativeCooldown < InContext.DeltaSeconds)
				{
					TickFunction->TickVisitedGFrameCounter = GFrameCounter;
					TickFunction->TickQueuedGFrameCounter = GFrameCounter;
					TickFunction->ExecuteTick(InContext.DeltaSeconds, InContext.TickType, ENamedThreads::GameThread, FGraphEventRef());
					TickFunction->CompletionHandle = NULL; // Allow the old completion handle to be recycled

					TickFunctionsToReschedule.Add(FTickScheduleDetails(TickFunction, TickFunction->TickInterval - (InContext.DeltaSeconds - CumulativeCooldown))); // Give credit for any overrun
				}
				else
				{
					TickFunctionsToReschedule.Add(FTickScheduleDetails(TickFunction, CumulativeCooldown - InContext.DeltaSeconds));
				}
				if (PrevTickFunction)
				{
					PrevTickFunction->Next = TickFunction->Next;
				}
				else
				{
					check(TickFunction == AllCoolingDownTickFunctions.Head);
					AllCoolingDownTickFunctions.Head = TickFunction->Next;
				}
				if (TickFunction->Next)
				{
					TickFunction->Next->RelativeTickCooldown += TickFunction->RelativeTickCooldown;
					CumulativeCooldown -= TickFunction->RelativeTickCooldown; // Since the next object in the list will have this cooldown included take it back out of the cumulative
				}
			}
			PrevTickFunction = TickFunction;
			TickFunction = TickFunction->Next;
		}

		for (TSet<FTickFunction*>::TIterator It(AllEnabledTickFunctions); It; ++It)
		{
			TickFunction = *It;
			TickFunction->CompletionHandle = NULL; // might as well NULL this out to allow these handles to be recycled
			if (TickFunction->bTickEvenWhenPaused && TickFunction->TickState == FTickFunction::ETickState::Enabled)
			{
				TickFunction->TickVisitedGFrameCounter = GFrameCounter;
				TickFunction->TickQueuedGFrameCounter = GFrameCounter;
				TickFunction->ExecuteTick(InContext.DeltaSeconds, InContext.TickType, ENamedThreads::GameThread, FGraphEventRef());
				TickFunction->CompletionHandle = NULL; // Allow the old completion handle to be recycled

				if (TickFunction->TickInterval > 0.f)
				{
					It.RemoveCurrent();
					TickFunctionsToReschedule.Add(FTickScheduleDetails(TickFunction, TickFunction->TickInterval));
				}
			}
		}

		if (TickFunctionsToReschedule.Num() > 0)
		{
			ScheduleTickFunctionCooldowns(TickFunctionsToReschedule);
			TickFunctionsToReschedule.Reset();
		}

		check(!NewlySpawnedTickFunctions.Num()); // We don't support new spawns during pause ticks
	}

	/** End a tick frame **/
	void EndFrame()
	{
		bTickNewlySpawned = false;
		check(!NewlySpawnedTickFunctions.Num()); // hmmm, this might be ok, but basically anything that was added this late cannot be ticked until the next frame
	}
	// Interface that is private to FTickFunction

	/** Return true if this tick function is in the master list **/
	bool HasTickFunction(FTickFunction* TickFunction)
	{
		return AllEnabledTickFunctions.Contains(TickFunction) || AllDisabledTickFunctions.Contains(TickFunction) || AllCoolingDownTickFunctions.Contains(TickFunction);
	}
	/** Add the tick function to the master list **/
	void AddTickFunction(FTickFunction* TickFunction)
	{
		check(!HasTickFunction(TickFunction));
		if (TickFunction->TickState == FTickFunction::ETickState::Enabled)
		{
			AllEnabledTickFunctions.Add(TickFunction);
			if (bTickNewlySpawned)
			{
				NewlySpawnedTickFunctions.Add(TickFunction);
			}
		}
		else
		{
			check(TickFunction->TickState == FTickFunction::ETickState::Disabled);
			AllDisabledTickFunctions.Add(TickFunction);
		}
	}

	/** Dumps info about a tick function to output device. */
	FORCEINLINE void DumpTickFunction(FOutputDevice& Ar, FTickFunction* Function, UEnum* TickGroupEnum, const float RemainingCooldown = 0.f)
	{
		// Info about the function.
		Ar.Logf(TEXT("%s, %s, ActualTickGroup: %s, Prerequesities: %d"),
			*Function->DiagnosticMessage(),
			Function->IsTickFunctionEnabled() ? (RemainingCooldown > 0.f ? *FString::Printf(TEXT("Cooling Down for %.4g seconds"),RemainingCooldown) : TEXT("Enabled")) : TEXT("Disabled"),
			*TickGroupEnum->GetEnumName(Function->ActualTickGroup),			
			Function->Prerequisites.Num());

		// List all prerequisities
		for (int32 Index = 0; Index < Function->Prerequisites.Num(); ++Index)
		{
			FTickPrerequisite& Prerequisite = Function->Prerequisites[Index];
			if (Prerequisite.PrerequisiteObject.IsValid())
			{
				Ar.Logf(TEXT("    %s, %s"), *Prerequisite.PrerequisiteObject->GetFullName(), *Prerequisite.PrerequisiteTickFunction->DiagnosticMessage());
			}
			else
			{
				Ar.Logf(TEXT("    Invalid Prerequisite"));
			}
		}
	}

	/** Dumps all tick functions to output device. */
	void DumpAllTickFunctions(FOutputDevice& Ar, int32& EnabledCount, int32& DisabledCount, bool bEnabled, bool bDisabled)
	{
		UEnum* TickGroupEnum = CastChecked<UEnum>(StaticFindObject(UEnum::StaticClass(), ANY_PACKAGE, TEXT("ETickingGroup"), true));
		if (bEnabled)
		{
			for (TSet<FTickFunction*>::TIterator It(AllEnabledTickFunctions); It; ++It)
			{
				DumpTickFunction(Ar, *It, TickGroupEnum);
			}
			float CumulativeCooldown = 0.f;
			FTickFunction* TickFunction = AllCoolingDownTickFunctions.Head;
			while (TickFunction)
			{
				CumulativeCooldown += TickFunction->RelativeTickCooldown;
				DumpTickFunction(Ar, TickFunction, TickGroupEnum, CumulativeCooldown);
				TickFunction = TickFunction->Next;
				++EnabledCount;
			}
		}
		else
		{
			FTickFunction* TickFunction = AllCoolingDownTickFunctions.Head;
			while (TickFunction)
			{
				TickFunction = TickFunction->Next;
				++EnabledCount;
			}
		}
		EnabledCount += AllEnabledTickFunctions.Num();
		if (bDisabled)
		{
			for (TSet<FTickFunction*>::TIterator It(AllDisabledTickFunctions); It; ++It)
			{
				DumpTickFunction(Ar, *It, TickGroupEnum);
			}
		}
		DisabledCount += AllDisabledTickFunctions.Num();
	}

	/** Remove the tick function from the master list **/
	void RemoveTickFunction(FTickFunction* TickFunction)
	{
		switch(TickFunction->TickState)
		{
		case FTickFunction::ETickState::Enabled:
			if (TickFunction->TickInterval > 0.f)
			{
				// An enabled function with a tick interval could be in either the enabled or cooling down list
				if (AllEnabledTickFunctions.Remove(TickFunction) == 0)
				{
					FTickFunction* PrevComparisionFunction = nullptr;
					FTickFunction* ComparisonFunction = AllCoolingDownTickFunctions.Head;
					bool bFound = false;
					while (ComparisonFunction)
					{
						if (ComparisonFunction == TickFunction)
						{
							bFound = true;
							if (PrevComparisionFunction)
							{
								PrevComparisionFunction->Next = TickFunction->Next;
							}
							else
							{
								check(TickFunction == AllCoolingDownTickFunctions.Head);
								AllCoolingDownTickFunctions.Head = TickFunction->Next;
							}
							TickFunction->Next = nullptr;
						}
						else
						{
							PrevComparisionFunction = ComparisonFunction;
							ComparisonFunction = ComparisonFunction->Next;
						}
					}
					check(bFound); // otherwise you changed TickState while the tick function was registered. Call SetTickFunctionEnable instead.
				}
			}
			else
			{
				verify(AllEnabledTickFunctions.Remove(TickFunction) == 1); // otherwise you changed TickState while the tick function was registered. Call SetTickFunctionEnable instead.
			}
			break;

		case FTickFunction::ETickState::Disabled:
			verify(AllDisabledTickFunctions.Remove(TickFunction) == 1); // otherwise you changed TickState while the tick function was registered. Call SetTickFunctionEnable instead.
			break;

		case FTickFunction::ETickState::CoolingDown:
			FTickFunction* PrevComparisionFunction = nullptr;
			FTickFunction* ComparisonFunction = AllCoolingDownTickFunctions.Head;
			bool bFound = false;
			while (ComparisonFunction)
			{
				if (ComparisonFunction == TickFunction)
				{
					bFound = true;
					if (PrevComparisionFunction)
					{
						PrevComparisionFunction->Next = TickFunction->Next;
					}
					else
					{
						check(TickFunction == AllCoolingDownTickFunctions.Head);
						AllCoolingDownTickFunctions.Head = TickFunction->Next;
					}
					if (TickFunction->Next)
					{
						TickFunction->Next->RelativeTickCooldown += TickFunction->RelativeTickCooldown;
						TickFunction->Next = nullptr;
					}
				}
				else
				{
					PrevComparisionFunction = ComparisonFunction;
					ComparisonFunction = ComparisonFunction->Next;
				}
			}
			check(bFound); // otherwise you changed TickState while the tick function was registered. Call SetTickFunctionEnable instead.
			break;
		}
		if (bTickNewlySpawned)
		{
			NewlySpawnedTickFunctions.Remove(TickFunction);
		}
	}

private:

	struct FCoolingDownTickFunctionList
	{
		FCoolingDownTickFunctionList()
			: Head(nullptr)
		{
		}

		bool Contains(FTickFunction* TickFunction) const
		{
			FTickFunction* Node = Head;
			while (Node)
			{
				if (Node == TickFunction)
				{
					return true;
				}
				Node = Node->Next;
			}
			return false;
		}

		FTickFunction* Head;
	};

	/** Global Sequencer														*/
	FTickTaskSequencer&							TickTaskSequencer;
	/** Master list of enabled tick functions **/
	TSet<FTickFunction*>						AllEnabledTickFunctions;
	/** Master list of enabled tick functions **/
	FCoolingDownTickFunctionList				AllCoolingDownTickFunctions;
	/** Master list of disabled tick functions **/
	TSet<FTickFunction*>						AllDisabledTickFunctions;
	/** Utility array to avoid memory reallocations when collecting functions to reschedule **/
	TArray<FTickScheduleDetails>				TickFunctionsToReschedule;
	/** List of tick functions added during a tick phase; these items are also duplicated in AllLiveTickFunctions for future frames **/
	TSet<FTickFunction*>						NewlySpawnedTickFunctions;
	/** tick context **/
	FTickContext								Context;
	/** true during the tick phase, when true, tick function adds also go to the newly spawned list. **/
	bool										bTickNewlySpawned;
};

/** Class that aggregates the individual levels and deals with parallel tick setup **/
class FTickTaskManager : public FTickTaskManagerInterface
{
public:
	/**
	 * Singleton to retrieve the global tick task manager
	 * @return Reference to the global tick task manager
	**/
	static FTickTaskManager& Get()
	{
		static FTickTaskManager SingletonInstance;
		return SingletonInstance;
	}

	/** Allocate a new ticking structure for a ULevel **/
	virtual FTickTaskLevel* AllocateTickTaskLevel() override
	{
		return new FTickTaskLevel;
	}

	/** Free a ticking structure for a ULevel **/
	virtual void FreeTickTaskLevel(FTickTaskLevel* TickTaskLevel) override
	{
		delete TickTaskLevel;
	}

	/**
	 * Ticks the world's dynamic actors based upon their tick group. This function
	 * is called once for each ticking group
	 *
	 * @param World	- World currently ticking
	 * @param DeltaSeconds - time in seconds since last tick
	 * @param TickType - type of tick (viewports only, time only, etc)
	 */
	virtual void StartFrame(UWorld* InWorld, float InDeltaSeconds, ELevelTick InTickType) override
	{
		SCOPE_CYCLE_COUNTER(STAT_QueueTicks);
		World = InWorld;
		Context.TickGroup = TG_PrePhysics; // reset this to the start tick group
		Context.DeltaSeconds = InDeltaSeconds;
		Context.TickType = InTickType;
		Context.Thread = ENamedThreads::GameThread;

		bTickNewlySpawned = true;
		TickTaskSequencer.StartFrame();
		FillLevelList();
		int32 TotalTickFunctions = 0;
		for( int32 LevelIndex = 0; LevelIndex < LevelList.Num(); LevelIndex++ )
		{
			TotalTickFunctions += LevelList[LevelIndex]->StartFrame(Context);
		}
		INC_DWORD_STAT_BY(STAT_TicksQueued, TotalTickFunctions);

		for( int32 LevelIndex = 0; LevelIndex < LevelList.Num(); LevelIndex++ )
		{
			LevelList[LevelIndex]->QueueAllTicks();
		}
	}

	/**
	 * Run all of the ticks for a pause frame synchronously on the game thread.
	 * The capability of pause ticks are very limited. There are no dependencies or ordering or tick groups.
	 * @param World	- World currently ticking
	 * @param DeltaSeconds - time in seconds since last tick
	 * @param TickType - type of tick (viewports only, time only, etc)
	 */
	virtual void RunPauseFrame(UWorld* InWorld, float InDeltaSeconds, ELevelTick InTickType) override
	{
		bTickNewlySpawned = true; // we don't support new spawns, but lets at least catch them.
		Context.TickGroup = TG_PrePhysics; // reset this to the start tick group
		Context.DeltaSeconds = InDeltaSeconds;
		Context.TickType = InTickType;
		Context.Thread = ENamedThreads::GameThread;
		World = InWorld;
		FillLevelList();
		for( int32 LevelIndex = 0; LevelIndex < LevelList.Num(); LevelIndex++ )
		{
			LevelList[LevelIndex]->RunPauseFrame(Context);
		}
		World = NULL;
		bTickNewlySpawned = false;
		LevelList.Reset();
	}
	/**
		* Run a tick group, ticking all actors and components
		* @param Group - Ticking group to run
		* @param bBlockTillComplete - if true, do not return until all ticks are complete
	*/
	virtual void RunTickGroup(ETickingGroup Group, bool bBlockTillComplete ) override
	{
		check(Context.TickGroup == Group); // this should already be at the correct value, but we want to make sure things are happening in the right order
		check(bTickNewlySpawned); // we should be in the middle of ticking
		TickTaskSequencer.ReleaseTickGroup(Group, bBlockTillComplete);
		Context.TickGroup = ETickingGroup(Context.TickGroup + 1); // new actors go into the next tick group because this one is already gone
		if (bBlockTillComplete) // we don't deal with newly spawned ticks within the async tick group, they wait until after the async stuff
		{
			QUICK_SCOPE_CYCLE_COUNTER(STAT_TickTask_RunTickGroup_BlockTillComplete);

			for (int32 Iterations = 0;Iterations < 101; Iterations++)
			{
				check(Iterations < 100); // this is runaway recursive spawning.
				int32 Num = 0;
				for( int32 LevelIndex = 0; LevelIndex < LevelList.Num(); LevelIndex++ )
				{
					Num += LevelList[LevelIndex]->QueueNewlySpawned(Context.TickGroup);
				}
				if (Num && Context.TickGroup == TG_NewlySpawned)
				{
					SCOPE_CYCLE_COUNTER(STAT_TG_NewlySpawned);
					TickTaskSequencer.ReleaseTickGroup(TG_NewlySpawned, true);
				}
				else
				{
					break;
				}
			}
		}
	}

	/** Finish a frame of ticks **/
	virtual void EndFrame() override
	{
		TickTaskSequencer.EndFrame();
		bTickNewlySpawned = false;
		for( int32 LevelIndex = 0; LevelIndex < LevelList.Num(); LevelIndex++ )
		{
			LevelList[LevelIndex]->EndFrame();
		}
		World = NULL;
		LevelList.Reset();
	}

	// Interface that is private to FTickFunction

	/** Return true if this tick function is in the master list **/
	bool HasTickFunction(ULevel* InLevel, FTickFunction* TickFunction)
	{
		FTickTaskLevel* Level = TickTaskLevelForLevel(InLevel);
		return Level->HasTickFunction(TickFunction);
	}
	/** Add the tick function to the master list **/
	void AddTickFunction(ULevel* InLevel, FTickFunction* TickFunction)
	{
		check(TickFunction->TickGroup >= 0 && TickFunction->TickGroup < TG_NewlySpawned); // You may not schedule a tick in the newly spawned group...they can only end up there if they are spawned late in a frame.
		FTickTaskLevel* Level = TickTaskLevelForLevel(InLevel);
		Level->AddTickFunction(TickFunction);
		TickFunction->TickTaskLevel = Level;
	}
	/** Remove the tick function from the master list **/
	void RemoveTickFunction(FTickFunction* TickFunction)
	{
		FTickTaskLevel* Level = TickFunction->TickTaskLevel;
		check(Level);
		Level->RemoveTickFunction(TickFunction);
	}

private:
	/** Default constructor **/
	FTickTaskManager()
		: TickTaskSequencer(FTickTaskSequencer::Get())
		, World(NULL)
		, bTickNewlySpawned(false)
	{
		IConsoleManager::Get().RegisterConsoleCommand(TEXT("dumpticks"), TEXT("Dumps all tick functions registered with FTickTaskManager to log."));
	}

	/** Fill the level list **/
	void FillLevelList()
	{
		check(!LevelList.Num());
		check(World->TickTaskLevel);
		LevelList.Add(World->TickTaskLevel);
		for( int32 LevelIndex = 0; LevelIndex < World->GetNumLevels(); LevelIndex++ )
		{
			ULevel* Level = World->GetLevel(LevelIndex);
			if (Level->bIsVisible)
			{
				check(Level->TickTaskLevel);
				LevelList.Add(Level->TickTaskLevel);
			}
		}
	}

	/** Find the tick level for this actor **/
	FTickTaskLevel* TickTaskLevelForLevel(ULevel* Level)
	{
		check(Level);
		check(Level->TickTaskLevel);
		return Level->TickTaskLevel;
	}

	/** Dumps all tick functions to output device */
	virtual void DumpAllTickFunctions(FOutputDevice& Ar, UWorld* InWorld, bool bEnabled, bool bDisabled) override
	{
		int32 EnabledCount = 0, DisabledCount = 0;

		Ar.Logf(TEXT(""));
		Ar.Logf(TEXT("============================ Tick Functions (%s) ============================"), (bEnabled && bDisabled) ? TEXT("All") : (bEnabled ? TEXT("Enabled") : TEXT("Disabled")));

		check(InWorld);
		check(InWorld->TickTaskLevel);
		InWorld->TickTaskLevel->DumpAllTickFunctions(Ar, EnabledCount, DisabledCount, bEnabled, bDisabled);
		for( int32 LevelIndex = 0; LevelIndex < InWorld->GetNumLevels(); LevelIndex++ )
		{
			ULevel* Level = InWorld->GetLevel(LevelIndex);
			if (Level->bIsVisible)
			{
				check(Level->TickTaskLevel);
				Level->TickTaskLevel->DumpAllTickFunctions(Ar, EnabledCount, DisabledCount, bEnabled, bDisabled);
			}
		}

		Ar.Logf(TEXT(""));
		Ar.Logf(TEXT("Total registered tick functions: %d, enabled: %d, disabled: %d."), EnabledCount + DisabledCount, EnabledCount, DisabledCount);
		Ar.Logf(TEXT(""));
	}

	/** Global Sequencer														*/
	FTickTaskSequencer&							TickTaskSequencer;
	/** World currently ticking **/
	UWorld*										World;
	/** List of current levels **/
	TArray<FTickTaskLevel*>						LevelList;
	/** tick context **/
	FTickContext								Context;
	/** true during the tick phase, when true, tick function adds also go to the newly spawned list. **/
	bool										bTickNewlySpawned;

};


/** Default constructor, intitalizes to reasonable defaults **/
FTickFunction::FTickFunction()
	: TickGroup(TG_PrePhysics)
	, ActualTickGroup(TG_PrePhysics)
	, bTickEvenWhenPaused(false)
	, bCanEverTick(false)
	, bAllowTickOnDedicatedServer(true)
	, bHighPriority(false)
	, bRunOnAnyThread(false)
	, bRegistered(false)
	, TickState(ETickState::Enabled)
	, TickVisitedGFrameCounter(0)
	, TickQueuedGFrameCounter(0)
	, RelativeTickCooldown(0.f)
	, TickInterval(0.f)
	, TickTaskLevel(NULL)
{
}

/** Destructor, unregisters the tick function **/
FTickFunction::~FTickFunction()
{
	UnRegisterTickFunction();
}


/** 
* Adds the tick function to the master list of tick functions. 
* @param Level - level to place this tick function in
**/
void FTickFunction::RegisterTickFunction(ULevel* Level)
{
	if (!bRegistered)
	{
		// Only allow registration of tick if we are are allowed on dedicated server, or we are not a dedicated server
		if(bAllowTickOnDedicatedServer || !IsRunningDedicatedServer())
		{
			FTickTaskManager::Get().AddTickFunction(Level, this);
			bRegistered = true;
		}
	}
	else
	{
		check(FTickTaskManager::Get().HasTickFunction(Level, this));
	}
}

/** Removes the tick function from the master list of tick functions. **/
void FTickFunction::UnRegisterTickFunction()
{
	if (bRegistered)
	{
		FTickTaskManager::Get().RemoveTickFunction(this);
		bRegistered = false;
	}
}

/** Enables or disables this tick function. **/
void FTickFunction::SetTickFunctionEnable(bool bInEnabled)
{
	if (bRegistered && (bInEnabled == (TickState == ETickState::Disabled)))
	{
		check(TickTaskLevel);
		TickTaskLevel->RemoveTickFunction(this);
		TickState = (bInEnabled ? ETickState::Enabled : ETickState::Disabled);
		TickTaskLevel->AddTickFunction(this);
	}
	else
	{
		TickState = (bInEnabled ? ETickState::Enabled : ETickState::Disabled);
	}
}

void FTickFunction::AddPrerequisite(UObject* TargetObject, struct FTickFunction& TargetTickFunction)
{
	const bool bThisCanTick = (bCanEverTick || IsTickFunctionRegistered());
	const bool bTargetCanTick = (TargetTickFunction.bCanEverTick || TargetTickFunction.IsTickFunctionRegistered());
	
	if (bThisCanTick && bTargetCanTick)
	{
		Prerequisites.AddUnique(FTickPrerequisite(TargetObject, TargetTickFunction));
	}
}

void FTickFunction::RemovePrerequisite(UObject* TargetObject, struct FTickFunction& TargetTickFunction)
{
	Prerequisites.RemoveSwap(FTickPrerequisite(TargetObject, TargetTickFunction));
}

void FTickFunction::SetPriorityIncludingPrerequisites(bool bInHighPriority)
{
	if (bHighPriority != bInHighPriority)
	{
		bHighPriority = bInHighPriority;
		for (auto& Prereq : Prerequisites)
		{
			if (Prereq.PrerequisiteObject.Get() && Prereq.PrerequisiteTickFunction && Prereq.PrerequisiteTickFunction->bHighPriority != bInHighPriority)
			{
				Prereq.PrerequisiteTickFunction->SetPriorityIncludingPrerequisites(bInHighPriority);
			}
		}
	}
}

void FTickFunction::ShowPrerequistes(int32 Indent)
{
	for (auto& Prereq : Prerequisites)
	{
		if (Prereq.PrerequisiteTickFunction)
		{
			UE_LOG(LogTick, Log, TEXT("%s prereq %s"), FCString::Spc(Indent * 2), *Prereq.PrerequisiteTickFunction->DiagnosticMessage());
			Prereq.PrerequisiteTickFunction->ShowPrerequistes(Indent + 1);
		}
	}
}

void FTickFunction::QueueTickFunction(FTickTaskSequencer& TTS, const struct FTickContext& TickContext)
{
	checkSlow(TickContext.Thread == ENamedThreads::GameThread); // we assume same thread here
	check(bRegistered);
		
	if (TickVisitedGFrameCounter != GFrameCounter)
	{
		TickVisitedGFrameCounter = GFrameCounter;
		if (TickState != FTickFunction::ETickState::Disabled)
		{
			ETickingGroup MaxPrerequisiteTickGroup = ETickingGroup(0);

			FGraphEventArray TaskPrerequisites;
			for (int32 PrereqIndex = 0; PrereqIndex < Prerequisites.Num(); PrereqIndex++)
			{
				FTickFunction* Prereq = Prerequisites[PrereqIndex].Get();
				if (!Prereq)
				{
					// stale prereq, delete it
					Prerequisites.RemoveAtSwap(PrereqIndex--);
				}
				else if (Prereq->bRegistered)
				{
					// recursive call to make sure my prerequisite is set up so I can use its completion handle
					Prereq->QueueTickFunction(TTS, TickContext);
					if (Prereq->TickQueuedGFrameCounter != GFrameCounter)
					{
						// this must be up the call stack, therefore this is a cycle
						UE_LOG(LogTick, Warning, TEXT("While processing prerequisites for %s, could use %s because it would form a cycle."),*DiagnosticMessage(), *Prereq->DiagnosticMessage());
					}
					else if (!Prereq->CompletionHandle.GetReference())
					{
						//ok UE_LOG(LogTick, Warning, TEXT("While processing prerequisites for %s, could use %s because it is disabled."),*DiagnosticMessage(), *Prereq->DiagnosticMessage());
					}
					else
					{
						MaxPrerequisiteTickGroup = FMath::Max<ETickingGroup>(MaxPrerequisiteTickGroup, Prereq->ActualTickGroup);
						TaskPrerequisites.Add(Prereq->CompletionHandle);
					}
				}
			}

			// tick group is the max of the prerequisites, the current tick group, and the desired tick group
			ETickingGroup MyActualTickGroup =  FMath::Max<ETickingGroup>(MaxPrerequisiteTickGroup, FMath::Max<ETickingGroup>(TickGroup,TickContext.TickGroup));
			if (MyActualTickGroup == TG_DuringPhysics && TickGroup != TG_DuringPhysics)
			{
				MyActualTickGroup = ETickingGroup(MyActualTickGroup + 1); // if the tick was "promoted" to during async, but it was not designed for that, then it needs to go later
			}
			ActualTickGroup = MyActualTickGroup;

			if (TickState == FTickFunction::ETickState::Enabled)
			{
				TTS.QueueTickTask(&TaskPrerequisites, this, TickContext);
			}
		}
		TickQueuedGFrameCounter = GFrameCounter;
	}
}


/**
 * Singleton to retrieve the global tick task manager
 * @return Reference to the global tick task manager
**/
FTickTaskManagerInterface& FTickTaskManagerInterface::Get()
{
	return FTickTaskManager::Get();
}


struct FTestTickFunction : public FTickFunction
{
	FTestTickFunction()
	{
		TickGroup = TG_PrePhysics;
		bTickEvenWhenPaused = true;
	}
	virtual void ExecuteTick(float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent) override
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_TestStatOverhead_FTestTickFunction);
		QUICK_SCOPE_CYCLE_COUNTER(STAT_TestStatOverhead_FTestTickFunction_Inner);
	}
	/** Abstract function to describe this tick. Used to print messages about illegal cycles in the dependency graph **/
	virtual FString DiagnosticMessage() override
	{
		return FString(TEXT("test"));
	}
};

static const int32 NumTestTickFunctions = 10000;
static TArray<FTestTickFunction> TestTickFunctions;
static TArray<FTestTickFunction*> IndirectTestTickFunctions;

static void RemoveTestTickFunctions(const TArray<FString>& Args)
{
	if (TestTickFunctions.Num() || IndirectTestTickFunctions.Num())
	{
		UE_LOG(LogConsoleResponse, Display, TEXT("Removing Test Tick Functions."));
		TestTickFunctions.Empty(NumTestTickFunctions);
		for (int32 Index = 0; Index < IndirectTestTickFunctions.Num(); Index++)
		{
			delete IndirectTestTickFunctions[Index];
		}
		IndirectTestTickFunctions.Empty(NumTestTickFunctions);
	}
}

static void AddTestTickFunctions(const TArray<FString>& Args)
{
	RemoveTestTickFunctions(Args);
	ULevel* Level = GWorld->GetCurrentLevel();
	UE_LOG(LogConsoleResponse, Display, TEXT("Adding 1000 ticks in a cache coherent fashion."));


	TestTickFunctions.Reserve(NumTestTickFunctions);
	for (int32 Index = 0; Index < NumTestTickFunctions; Index++)
	{
		(new (TestTickFunctions) FTestTickFunction())->RegisterTickFunction(Level);
	}
}

static void AddIndirectTestTickFunctions(const TArray<FString>& Args)
{
	RemoveTestTickFunctions(Args);
	ULevel* Level = GWorld->GetCurrentLevel();
	UE_LOG(LogConsoleResponse, Display, TEXT("Adding 1000 ticks in a cache coherent fashion."));
	TArray<FTestTickFunction*> Junk;
	for (int32 Index = 0; Index < NumTestTickFunctions; Index++)
	{
		for (int32 JunkIndex = 0; JunkIndex < 8; JunkIndex++)
		{
			Junk.Add(new FTestTickFunction); // don't give the allocator an easy ride
		}
		FTestTickFunction* NewTick = new FTestTickFunction;
		NewTick->RegisterTickFunction(Level);
		IndirectTestTickFunctions.Add(NewTick);
	}
	for (int32 JunkIndex = 0; JunkIndex < 8; JunkIndex++)
	{
		delete Junk[JunkIndex];
	}
}

static FAutoConsoleCommand RemoveTestTickFunctionsCmd(
	TEXT("tick.RemoveTestTickFunctions"),
	TEXT("Remove no-op ticks to test performance of ticking infrastructure."),
	FConsoleCommandWithArgsDelegate::CreateStatic(&RemoveTestTickFunctions)
	);

static FAutoConsoleCommand AddTestTickFunctionsCmd(
	TEXT("tick.AddTestTickFunctions"),
	TEXT("Add no-op ticks to test performance of ticking infrastructure."),
	FConsoleCommandWithArgsDelegate::CreateStatic(&AddTestTickFunctions)
	);

static FAutoConsoleCommand AddIndirectTestTickFunctionsCmd(
	TEXT("tick.AddIndirectTestTickFunctions"),
	TEXT("Add no-op ticks to test performance of ticking infrastructure."),
	FConsoleCommandWithArgsDelegate::CreateStatic(&AddIndirectTestTickFunctions)
	);



