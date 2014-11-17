//// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SLeafWidget.cpp: Implements the SLeafWidget class.
=============================================================================*/

#include "SlateCorePrivatePCH.h"


/* Static initialization
 *****************************************************************************/

FNoChildren SLeafWidget::NoChildrenInstance;


/* SLeafWidget interface
 *****************************************************************************/

FChildren* SLeafWidget::GetChildren( )
{
	return &SLeafWidget::NoChildrenInstance;
}


void SLeafWidget::ArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const
{
	// Nothing to arrange; Leaf Widgets do not have children.
}
