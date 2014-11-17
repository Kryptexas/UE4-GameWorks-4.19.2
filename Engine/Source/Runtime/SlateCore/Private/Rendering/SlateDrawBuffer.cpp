// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SlateDrawBuffer.cpp: Implements the FSlateDrawBuffer class.
=============================================================================*/

#include "SlateCorePrivatePCH.h"


/* FSlateDrawBuffer interface
 *****************************************************************************/

FSlateWindowElementList& FSlateDrawBuffer::AddWindowElementList( TSharedRef<SWindow> ForWindow )
{
	int32 Index = WindowElementLists.Add(FSlateWindowElementList(ForWindow));

	return WindowElementLists[Index];
}


bool FSlateDrawBuffer::Lock( )
{
	return FPlatformAtomics::InterlockedCompareExchange(&Locked, 1, 0) == 0;
}


void FSlateDrawBuffer::Unlock( )
{
	FPlatformAtomics::InterlockedExchange(&Locked, 0);
}
