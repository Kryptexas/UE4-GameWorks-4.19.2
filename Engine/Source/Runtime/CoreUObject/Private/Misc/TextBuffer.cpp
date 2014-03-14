// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	TextBuffer.cpp: UObject class for storing text
=============================================================================*/

#include "CoreUObjectPrivate.h"


IMPLEMENT_CORE_INTRINSIC_CLASS(UTextBuffer, UObject, { });


/* UTextBuffer constructors
 *****************************************************************************/

UTextBuffer::UTextBuffer (const class FPostConstructInitializeProperties& PCIP, const TCHAR* InText)
	: UObject(PCIP)
	, Text(InText)
{}


/* FOutputDevice interface
 *****************************************************************************/

void UTextBuffer::Serialize (const TCHAR* Data, ELogVerbosity::Type Verbosity, const class FName& Category)
{
	Text += (TCHAR*)Data;
}


/* UObject interface
 *****************************************************************************/

void UTextBuffer::Serialize (FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar << Pos << Top << Text;
}