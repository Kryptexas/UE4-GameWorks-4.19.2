// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SlateRect.cpp: Implements the FSlateRect structure.
=============================================================================*/

#include "SlateCorePrivatePCH.h"


/* FSlateRect interface
 *****************************************************************************/

FSlateRect FSlateRect::InsetBy( const FMargin& InsetAmount ) const
{
	return FSlateRect(Left + InsetAmount.Left, Top + InsetAmount.Top, Right - InsetAmount.Right, Bottom - InsetAmount.Bottom);
}
