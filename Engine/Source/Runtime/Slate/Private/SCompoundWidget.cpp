// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"

#include "LayoutUtils.h"


/**
 * The widget should respond by populating the OutDrawElements array with FDrawElements 
 * that represent it and any of its children.
 *
 * @param AllottedGeometry  The FGeometry that describes an area in which the widget should appear.
 * @param MyClippingRect    The clipping rectangle allocated for this widget and its children.
 * @param OutDrawElements   A list of FDrawElements to populate with the output.
 * @param LayerId           The Layer onto which this widget should be rendered.
 * @param InColorAndOpacity Color and Opacity to be applied to all the descendants of the widget being painted
 * @param bParentEnabled	True if the parent of this widget is enabled.
 *
 * @return The maximum layer ID attained by this widget or any of its children.
 */
int32 SCompoundWidget::OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
{
	// A CompoundWidget just draws its children
	FArrangedChildren ArrangedChildren(EVisibility::Visible);
	{
#if SLATE_HD_STATS
		SCOPE_CYCLE_COUNTER( STAT_SlateOnPaint_SCompoundWidget );
#endif
		this->ArrangeChildren(AllottedGeometry, ArrangedChildren);
	}

	// There may be zero elements in this array if our child collapsed/hidden
	if( ArrangedChildren.Num() > 0 )
	{
		check( ArrangedChildren.Num() == 1 );
		FArrangedWidget& TheChild = ArrangedChildren(0);

		const FSlateRect ChildClippingRect = AllottedGeometry.GetClippingRect().InsetBy( ChildSlot.SlotPadding.Get() * AllottedGeometry.Scale ).IntersectionWith(MyClippingRect);

		FWidgetStyle CompoundedWidgetStyle = FWidgetStyle(InWidgetStyle)
			.BlendColorAndOpacityTint(ColorAndOpacity.Get())
			.SetForegroundColor( ForegroundColor );

		return TheChild.Widget->OnPaint( TheChild.Geometry, ChildClippingRect, OutDrawElements, LayerId + 1, CompoundedWidgetStyle, ShouldBeEnabled( bParentEnabled ) );
	}
	return LayerId;
}

FChildren* SCompoundWidget::GetChildren()
{
	return &ChildSlot;
}


FVector2D SCompoundWidget::ComputeDesiredSize() const
{
	EVisibility ChildVisibility = ChildSlot.Widget->GetVisibility();
	if ( ChildVisibility != EVisibility::Collapsed )
	{
		return ChildSlot.Widget->GetDesiredSize() + ChildSlot.SlotPadding.Get().GetDesiredSize();
	}
	
	return FVector2D::ZeroVector;
}

void SCompoundWidget::ArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const
{
	ArrangeSingleChild( AllottedGeometry, ArrangedChildren, ChildSlot, ContentScale );
}

FSlateColor SCompoundWidget::GetForegroundColor() const
{
	return ForegroundColor.Get();
}