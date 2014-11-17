// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	WidgetStyle.cpp: Implements the FWidgetStyle structure.
=============================================================================*/

#include "SlateCorePrivatePCH.h"


/* Static initialization
 *****************************************************************************/

const float FWidgetStyle::SubdueAmount = 0.6f;


/* FWidgetStyle interface
 *****************************************************************************/

FWidgetStyle& FWidgetStyle::SetForegroundColor( const TAttribute<struct FSlateColor>& InForeground )
{
	ForegroundColor = InForeground.Get().GetColor(*this);

	SubduedForeground = ForegroundColor;
	SubduedForeground.A *= SubdueAmount;

	return *this;
}
