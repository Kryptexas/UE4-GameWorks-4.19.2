// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "RHI.h"
#include "RHICommandList.h"

DECLARE_CYCLE_STAT(TEXT("Nonimmed. Command List Execute"), STAT_NonImmedCmdListExecuteTime, STATGROUP_RHICMDLIST);
DECLARE_DWORD_COUNTER_STAT(TEXT("Nonimmed. Command List memory"), STAT_NonImmedCmdListMemory, STATGROUP_RHICMDLIST);
DECLARE_DWORD_COUNTER_STAT(TEXT("Nonimmed. Command count"), STAT_NonImmedCmdListCount, STATGROUP_RHICMDLIST);

DECLARE_CYCLE_STAT(TEXT("All Command List Execute"), STAT_ImmedCmdListExecuteTime, STATGROUP_RHICMDLIST);
DECLARE_DWORD_COUNTER_STAT(TEXT("Immed. Command List memory"), STAT_ImmedCmdListMemory, STATGROUP_RHICMDLIST);
DECLARE_DWORD_COUNTER_STAT(TEXT("Immed. Command count"), STAT_ImmedCmdListCount, STATGROUP_RHICMDLIST);


static TAutoConsoleVariable<int32> CVarRHICmdBypass(
	TEXT("r.RHICmdBypass"),
	FRHICommandListExecutor::DefaultBypass,
	TEXT("Whether to bypass the rhi command list and send the rhi commands immediately.\n")
	TEXT("0: Disable, 1: Enable"));

static TAutoConsoleVariable<int32> CVarRHICmdUseParallelAlgorithms(
	TEXT("r.RHICmdUseParallelAlgorithms"),
	FRHICommandListExecutor::DefaultBypass,
	TEXT("True to use parallel algorithms. Ignored if r.RHICmdBypass is 1.\n"));

static TAutoConsoleVariable<int32> CVarRHICmdFlushBeforeWait(
	TEXT("r.RHICmdFlushBeforeWait"),
	1,
	TEXT("Whether to flush the immediate command list before waiting for a command list chain.\n")
	TEXT("0: Disable, 1: Enable"));

TAutoConsoleVariable<int32> CVarRHICmdWidth(
	TEXT("r.RHICmdWidth"), 
	8,
	TEXT("Number of threads."));


RHI_API FRHICommandListExecutor GRHICommandList;


//RHI_API FGlobalRHI GRHI;

void FRHICommandListExecutor::ExecuteInner(FRHICommandListBase& CmdList)
{
	CmdList.bExecuting = true;
	{
		FRHICommandListIterator Iter(CmdList);
		while (Iter.HasCommandsLeft())
		{
			FRHICommandBase* Cmd = Iter.NextCommand();
			Cmd->CallExecuteAndDestruct();
		}
	}
	CmdList.Reset();
}

void FRHICommandListExecutor::ExecuteList(FRHICommandListBase& CmdList)
{
	check(IsInRenderingThread() && &CmdList != &GetImmediateCommandList());

	if (!GetImmediateCommandList().IsExecuting()) // don't flush if this is a recursive call and we are already executing the immediate command list
	{
		GetImmediateCommandList().Flush();
	}

	INC_MEMORY_STAT_BY(STAT_NonImmedCmdListMemory, CmdList.GetUsedMemory());
	INC_DWORD_STAT_BY(STAT_NonImmedCmdListCount, CmdList.NumCommands);

	SCOPE_CYCLE_COUNTER(STAT_NonImmedCmdListExecuteTime);
	ExecuteInner(CmdList);
}

void FRHICommandListExecutor::ExecuteList(FRHICommandListImmediate& CmdList)
{
	check(IsInRenderingThread() && &CmdList == &GetImmediateCommandList());

	INC_MEMORY_STAT_BY(STAT_ImmedCmdListMemory, CmdList.GetUsedMemory());
	INC_DWORD_STAT_BY(STAT_ImmedCmdListCount, CmdList.NumCommands);
#if 0
	static TAutoConsoleVariable<int32> CVarRHICmdMemDump(
		TEXT("r.RHICmdMemDump"),
		0,
		TEXT("dumps callstacks and sizes of the immediate command lists to the console.\n")
		TEXT("0: Disable, 1: Enable"),
		ECVF_Cheat);
	if (CVarRHICmdMemDump.GetValueOnRenderThread() > 0)
	{
		FPlatformMisc::LowLevelOutputDebugStringf(TEXT("Mem %d\n"), CmdList.GetUsedMemory());
		if (CmdList.GetUsedMemory() > 300)
		{
			FPlatformMisc::LowLevelOutputDebugStringf(TEXT("big\n"));
		}
	}
#endif
	{
		SCOPE_CYCLE_COUNTER(STAT_ImmedCmdListExecuteTime);
		//FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::RenderThread_Local); 
		ExecuteInner(CmdList);
	}

#if !UE_BUILD_SHIPPING
	if (GRHICommandList.OutstandingCmdListCount.GetValue() == 1)
	{
		LatchBypass();
	}
#endif
}


void FRHICommandListExecutor::LatchBypass()
{
#if !UE_BUILD_SHIPPING

	static bool bOnce = false;
	if (!bOnce)
	{
		bOnce = true;
		if (FParse::Param(FCommandLine::Get(),TEXT("parallelrendering")) && CVarRHICmdBypass.GetValueOnRenderThread() >= 1)
		{
			IConsoleVariable* BypassVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.RHICmdBypass"));
			BypassVar->Set(0);
		}
	}

	check(GRHICommandList.OutstandingCmdListCount.GetValue() == 1 && !GRHICommandList.GetImmediateCommandList().HasCommands());
	bool NewBypass = (CVarRHICmdBypass.GetValueOnAnyThread() >= 1);
	if (NewBypass && !bLatchedBypass)
	{
		FRHIResource::FlushPendingDeletes();
	}
	bLatchedBypass = NewBypass;
	if (!bLatchedBypass)
	{
		bLatchedUseParallelAlgorithms = (CVarRHICmdUseParallelAlgorithms.GetValueOnAnyThread() >= 1);
	}
	else
	{
		bLatchedUseParallelAlgorithms = false;
	}
#endif
}

void FRHICommandListExecutor::CheckNoOutstandingCmdLists()
{
	check(GRHICommandList.OutstandingCmdListCount.GetValue() == 1); // else we are attempting to delete resources while there is still a live cmdlist (other than the immediate cmd list) somewhere.
}

FRHICommandListBase::FRHICommandListBase()
	: MemManager(0)
{
	GRHICommandList.OutstandingCmdListCount.Increment();
	Reset();
}

FRHICommandListBase::~FRHICommandListBase()
{
	Flush();
	GRHICommandList.OutstandingCmdListCount.Decrement();
}

const int32 FRHICommandListBase::GetUsedMemory() const
{
	return MemManager.GetByteCount();
}

void FRHICommandListBase::Reset()
{
	bExecuting = false;
	MemManager.Flush();
	NumCommands = 0;
	Root = nullptr;
	CommandLink = &Root;
	UID = GRHICommandList.UIDCounter.Increment();
}

DECLARE_DWORD_COUNTER_STAT(TEXT("Num Async Chains Links"), STAT_ChainLinkCount, STATGROUP_RHICMDLIST);
DECLARE_CYCLE_STAT(TEXT("Wait for Async CmdList"), STAT_ChainWait, STATGROUP_RHICMDLIST);
DECLARE_CYCLE_STAT(TEXT("Async Chain Execute"), STAT_ChainExecute, STATGROUP_RHICMDLIST);

struct FRHICommandWaitForAndSubmitSubList : public FRHICommand<FRHICommandWaitForAndSubmitSubList>
{
	FGraphEventRef EventToWaitFor;
	FRHICommandList* RHICmdList;
	FORCEINLINE_DEBUGGABLE FRHICommandWaitForAndSubmitSubList(FGraphEventRef& InEventToWaitFor, FRHICommandList* InRHICmdList)
		: EventToWaitFor(InEventToWaitFor)
		, RHICmdList(InRHICmdList)
	{
	}
	void Execute()
	{
		INC_DWORD_STAT_BY(STAT_ChainLinkCount, 1);
		{
			SCOPE_CYCLE_COUNTER(STAT_ChainWait);
			FTaskGraphInterface::Get().WaitUntilTaskCompletes(EventToWaitFor, ENamedThreads::RenderThread_Local);
		}
		{
			SCOPE_CYCLE_COUNTER(STAT_ChainExecute);
			delete RHICmdList;
		}
	}
};

void FRHICommandListBase::QueueAsyncCommandListSubmit(FGraphEventRef& AnyThreadCompletionEvent, class FRHICommandList* CmdList)
{
	new (AllocCommand<FRHICommandWaitForAndSubmitSubList>()) FRHICommandWaitForAndSubmitSubList(AnyThreadCompletionEvent, CmdList);
}

DECLARE_DWORD_COUNTER_STAT(TEXT("Num RT Chains Links"), STAT_RTChainLinkCount, STATGROUP_RHICMDLIST);
DECLARE_CYCLE_STAT(TEXT("Wait for RT CmdList"), STAT_RTChainWait, STATGROUP_RHICMDLIST);
DECLARE_CYCLE_STAT(TEXT("RT Chain Execute"), STAT_RTChainExecute, STATGROUP_RHICMDLIST);

struct FRHICommandWaitForAndSubmitRTSubList : public FRHICommand<FRHICommandWaitForAndSubmitRTSubList>
{
	FGraphEventRef EventToWaitFor;
	FRHICommandList* RHICmdList;
	FORCEINLINE_DEBUGGABLE FRHICommandWaitForAndSubmitRTSubList(FGraphEventRef& InEventToWaitFor, FRHICommandList* InRHICmdList)
		: EventToWaitFor(InEventToWaitFor)
		, RHICmdList(InRHICmdList)
	{
	}
	void Execute()
	{
		INC_DWORD_STAT_BY(STAT_RTChainLinkCount, 1);
		{
			SCOPE_CYCLE_COUNTER(STAT_RTChainWait);
			if (IsInRenderingThread())
			{
				if (!EventToWaitFor->IsComplete())
				{
					if (FTaskGraphInterface::Get().IsThreadProcessingTasks(ENamedThreads::RenderThread_Local))
					{
						// this is a deadlock. RT tasks must be done by now or they won't be done. We could add a third queue...
						UE_LOG(LogRHI, Fatal, TEXT("Deadlock in command list processing."));
					}
#if 0
					while (!EventToWaitFor->IsComplete())
					{
						FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::RenderThread_Local);
					}
#endif
					FTaskGraphInterface::Get().WaitUntilTaskCompletes(EventToWaitFor, ENamedThreads::RenderThread_Local);
				}
			}
			else
			{
				FScopedEvent Waiter;
				FTaskGraphInterface::Get().TriggerEventWhenTaskCompletes(Waiter.Get(), EventToWaitFor);
			}
		}
		{
			SCOPE_CYCLE_COUNTER(STAT_RTChainExecute);
			delete RHICmdList;
		}
	}
};

void FRHICommandListBase::QueueRenderThreadCommandListSubmit(FGraphEventRef& RenderThreadCompletionEvent, class FRHICommandList* CmdList)
{
	new (AllocCommand<FRHICommandWaitForAndSubmitRTSubList>()) FRHICommandWaitForAndSubmitRTSubList(RenderThreadCompletionEvent, CmdList);
}

struct FRHICommandSubmitSubList : public FRHICommand<FRHICommandSubmitSubList>
{
	FRHICommandList* RHICmdList;
	FORCEINLINE_DEBUGGABLE FRHICommandSubmitSubList(FRHICommandList* InRHICmdList)
		: RHICmdList(InRHICmdList)
	{
	}
	void Execute()
	{
		INC_DWORD_STAT_BY(STAT_ChainLinkCount, 1);
		SCOPE_CYCLE_COUNTER(STAT_ChainExecute);
		delete RHICmdList;
	}
};

void FRHICommandListBase::QueueCommandListSubmit(class FRHICommandList* CmdList)
{
	new (AllocCommand<FRHICommandSubmitSubList>()) FRHICommandSubmitSubList(CmdList);
}

static TLockFreeFixedSizeAllocator<sizeof(FRHICommandList), FThreadSafeCounter> RHICommandListAllocator;

void* FRHICommandList::operator new(size_t Size)
{
	// doesn't support derived classes with a different size
	check(Size == sizeof(FRHICommandList));
	return RHICommandListAllocator.Allocate();
	//return FMemory::Malloc(Size);
}

/**
 * Custom delete
 */
void FRHICommandList::operator delete(void *RawMemory)
{
	RHICommandListAllocator.Free(RawMemory);
	//FMemory::Free(RawMemory);
}	

