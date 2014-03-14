// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	UBrushBuilder.cpp: UnrealEd brush builder.
=============================================================================*/

#include "EnginePrivate.h"

/*-----------------------------------------------------------------------------
	UBrushBuilder.
-----------------------------------------------------------------------------*/
UBrushBuilder::UBrushBuilder(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	BitmapFilename = TEXT("BBGeneric");
	ToolTip = TEXT("BrushBuilderName_Generic");
	NotifyBadParams = true;
}