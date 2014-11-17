// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "RHI.h"
#include "RHICommandList.h"

DEFINE_STAT(STAT_ImmedCmdListExecuteTime);
DEFINE_STAT(STAT_ImmedCmdListMemory);
DEFINE_STAT(STAT_ImmedCmdListCount);

DEFINE_STAT(STAT_NonImmedCmdListExecuteTime);
DEFINE_STAT(STAT_NonImmedCmdListMemory);
DEFINE_STAT(STAT_NonImmedCmdListCount);

static TAutoConsoleVariable<int32> CVarRHICmdBypass(
	TEXT("r.RHICmdBypass"),
	FRHICommandListExecutor::DefaultBypass,
	TEXT("Whether to bypass the rhi command list and send the rhi commands immediately.\n")
	TEXT("0: Disable, 1: Enable"),
	ECVF_Cheat);

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

	if (!GetImmediateCommandList().bExecuting) // don't flush if this is a recursive call and we are already executing the immediate command list
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

	{
		SCOPE_CYCLE_COUNTER(STAT_ImmedCmdListExecuteTime);
		ExecuteInner(CmdList);
	}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (GRHICommandList.OutstandingCmdListCount.GetValue() == 1)
	{
		LatchBypass();
	}
#endif
}


void FRHICommandListExecutor::LatchBypass()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

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


