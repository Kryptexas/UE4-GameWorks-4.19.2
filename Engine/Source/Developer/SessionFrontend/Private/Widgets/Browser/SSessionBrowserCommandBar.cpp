// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionBrowserCommandBar.cpp: Implements the SSessionBrowserCommandBar class.
=============================================================================*/

#include "SessionFrontendPrivatePCH.h"


#define LOCTEXT_NAMESPACE "SSessionBrowserCommandBar"


/* SSessionBrowserCommandBar interface
 *****************************************************************************/

void SSessionBrowserCommandBar::Construct( const FArguments& InArgs )
{
	ChildSlot
	[
		SNew(SHorizontalBox)
	];
}


#undef LOCTEXT_NAMESPACE
