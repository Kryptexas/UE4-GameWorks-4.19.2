// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SPanel.cpp: Implements the SPanel class.
=============================================================================*/

#include "SlateCorePrivatePCH.h"


/* SWidget overrides
 *****************************************************************************/

int32 SPanel::OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
{
	// REMOVE THIS: Currently adding a debug quad so that BoxPanels are visible during development
	// OutDrawElements.AddItem( FSlateDrawElement::MakeQuad( AllottedGeometry.AbsolutePosition, AllottedGeometry.Size ) );	
	FArrangedChildren ArrangedChildren(EVisibility::Visible);
	{
#if SLATE_HD_STATS
		SCOPE_CYCLE_COUNTER( STAT_SlateOnPaint_SPanel );
#endif
		// The box panel has no visualization of its own; it just visualizes its children.
		
		this->ArrangeChildren(AllottedGeometry, ArrangedChildren);
	}

	return PaintArrangedChildren(ArrangedChildren, MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
}


/* SPanel implementation
 *****************************************************************************/

int32 SPanel::PaintArrangedChildren( const FArrangedChildren& ArrangedChildren, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled  ) const
{
	// Because we paint multiple children, we must track the maximum layer id that they produced in case one of our parents
	// wants to an overlay for all of its contents.
	int32 MaxLayerId = LayerId;

	for (int32 ChildIndex = 0; ChildIndex < ArrangedChildren.Num(); ++ChildIndex)
	{
		const FArrangedWidget& CurWidget = ArrangedChildren(ChildIndex);
		FSlateRect ChildClipRect = MyClippingRect.IntersectionWith(CurWidget.Geometry.GetClippingRect());
		
		if (ChildClipRect.GetSize().Size() > 0)
		{
			const int32 CurWidgetsMaxLayerId = CurWidget.Widget->OnPaint(CurWidget.Geometry, ChildClipRect, OutDrawElements, LayerId, InWidgetStyle, ShouldBeEnabled(bParentEnabled));
			MaxLayerId = FMath::Max(MaxLayerId, CurWidgetsMaxLayerId);
		}		
		
	}
	
	return MaxLayerId;
}
