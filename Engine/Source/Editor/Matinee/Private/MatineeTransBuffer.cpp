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
	const int32 Result = ActiveCount;
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
	const int32 PriorRecordsCount = (Result > 0 ? ActiveRecordCounts[Result - 1] : 0);
	ActiveRecordCounts.Add(UndoBuffer.Last().GetRecordCount() - PriorRecordsCount);
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
	ActiveRecordCounts.Pop();
	CheckState();
}
