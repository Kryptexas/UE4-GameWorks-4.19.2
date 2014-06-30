// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	RHICommandList.inl: RHI Command List inline definitions.
=============================================================================*/

#ifndef __RHICOMMANDLIST_INL__
#define __RHICOMMANDLIST_INL__
#include "RHICommandList.h"

FORCEINLINE void FRHICommandList::Flush()
{
	if (HasCommands())
	{
		GRHICommandList.ExecuteList(*this);
	}
}

// Helper class for traversing a FRHICommandList
class FRHICommandListIterator
{
public:
	FRHICommandListIterator(FRHICommandList& CmdList)
	{
		NumCommands = 0;
		CmdListNumCommands = CmdList.NumCommands;
		Page = CmdList.MemManager.FirstPage;
		CmdPtr = Page->Head;
		CmdTail = Page->Current;
	}

	FORCEINLINE bool HasCommandsLeft() const
	{
		return (CmdPtr < CmdTail);
	}

	// Current command
	FORCEINLINE FRHICommand* operator * ()
	{
		return (FRHICommand*)CmdPtr;
	}

	// Get the next RHICommand and advance the iterator
	template <typename TCmd>
	FORCEINLINE TCmd* NextCommand()
	{
		TCmd* RHICmd = (TCmd*)CmdPtr;
		//::OutputDebugStringW(*FString::Printf(TEXT("Exec %d: %d @ 0x%p, %d bytes\n"), NumCommands, (int32)RHICmd->Type, (void*)RHICmd, sizeof(TCmd) + RHICmd->ExtraSize()));
		CmdPtr += sizeof(TCmd) + RHICmd->ExtraSize();
		CmdPtr = Align(CmdPtr, FRHICommandList::Alignment);

		//@todo-rco: Fix me!
		if (!TCmd::IsEndOfPage)
		{
			// Don't count EOP as that is an allocator construct
			++NumCommands;
		}

		if (TCmd::IsEndOfPage || CmdPtr >= CmdTail)
		{
			Page = Page->NextPage;
			CmdPtr = Page ? Page->Head : nullptr;
			CmdTail = Page ? Page->Current : nullptr;
		}

		return RHICmd;
	}

	void CheckNumCommands()
	{
		checkf(CmdListNumCommands == NumCommands, TEXT("Missed %d Commands!"), CmdListNumCommands - NumCommands);
		checkf(CmdPtr == CmdTail, TEXT("Mismatched Tail Pointer!"));
	}

protected:
	FRHICommandList::FMemManager::FPage* Page;
	uint32 NumCommands;
	const uint8* CmdPtr;
	const uint8* CmdTail;
	uint32 CmdListNumCommands;
};

#endif	// __RHICOMMANDLIST_INL__
