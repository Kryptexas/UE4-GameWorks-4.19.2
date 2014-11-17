// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ArrangedChildren.cpp: Implements the FArrangedChildren class.
=============================================================================*/

#include "SlateCorePrivatePCH.h"


/* FArrangedChildren interface
 *****************************************************************************/

void FArrangedChildren::AddWidget( const FArrangedWidget& InWidgetGeometry )
{
	AddWidget(InWidgetGeometry.Widget->GetVisibility(), InWidgetGeometry);
}


void FArrangedChildren::AddWidget( EVisibility VisibilityOverride, const FArrangedWidget& InWidgetGeometry )
{
	if (this->Accepts(VisibilityOverride))
	{
		Array.Add(InWidgetGeometry);
	}
}
