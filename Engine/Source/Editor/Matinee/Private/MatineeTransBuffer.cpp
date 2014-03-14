// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MatineeModule.h"
#include "MatineeTransaction.h"
#include "MatineeTransBuffer.h"

/*-----------------------------------------------------------------------------
	UMatineeTransBuffer / FMatineeTransaction
-----------------------------------------------------------------------------*/
UMatineeTransBuffer::UMatineeTransBuffer(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UMatineeTransBuffer::BeginSpecial(const FText& Description)
{
	CheckState();
	if( ActiveCount++==0 )
	{
		// Cancel redo buffer.
		if( UndoCount )
		{
			UndoBuffer.RemoveAt( UndoBuffer.Num()-UndoCount, UndoCount );
		}

		UndoCount = 0;

		// Purge previous transactions if too much data occupied.
		while( GetUndoSize() > MaxMemory )
		{
			UndoBuffer.RemoveAt( 0 );
		}

		// Begin a new transaction.
		GUndo = new(UndoBuffer)FMatineeTransaction( Description, 1 );
	}
	CheckState();
}

void UMatineeTransBuffer::EndSpecial()
{
	CheckState();
	check(ActiveCount>=1);
	if( --ActiveCount==0 )
	{
		GUndo = NULL;
	}
	CheckState();
}
