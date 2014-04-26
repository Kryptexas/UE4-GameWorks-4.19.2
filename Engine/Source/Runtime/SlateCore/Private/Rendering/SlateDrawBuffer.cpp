// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SlateDrawBuffer.cpp: Implements the FSlateDrawBuffer class.
=============================================================================*/

#include "SlateCorePrivatePCH.h"


/* Local constants
 *****************************************************************************/

uint32 GDrawBuffers = 0;
uint32 MaxDrawBuffers = 0;


/* FSlateDrawBuffer structors
 *****************************************************************************/

FSlateDrawBuffer::FSlateDrawBuffer( ) 
	: Locked(0)
{
	++GDrawBuffers;

	if (GDrawBuffers > MaxDrawBuffers)
	{
		MaxDrawBuffers = GDrawBuffers;
	}
}


FSlateDrawBuffer::~FSlateDrawBuffer( )
{
	--GDrawBuffers;
}


/* FSlateDrawBuffer interface
 *****************************************************************************/

void FSlateDrawBuffer::ClearBuffer( )
{
	WindowElementLists.Empty();
}


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