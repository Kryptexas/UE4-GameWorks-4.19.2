// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ArrangedWidget.cpp: Implements the FArrangedWidget class.
=============================================================================*/

#include "SlateCorePrivatePCH.h"


/* FArrangedWidget interface
 *****************************************************************************/

FString FArrangedWidget::ToString( ) const
{
	return FString::Printf(TEXT("%s @ %s"), *Widget->ToString(), *Geometry.ToString());
}
