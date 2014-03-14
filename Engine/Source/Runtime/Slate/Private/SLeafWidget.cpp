// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"

FNoChildren SLeafWidget::NoChildrenInstance;


/** @return NoChildren; leaf widgets never have children */
FChildren* SLeafWidget::GetChildren()
{
	return &SLeafWidget::NoChildrenInstance;
}

/**
 * Compute the Geometry of all the children and add populate the ArrangedChildren list with their values.
 * Each type of Layout panel should arrange children based on desired behavior.
 */
void SLeafWidget::ArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const
{
	// Nothing to arrange; Leaf Widgets do not have children.
}

