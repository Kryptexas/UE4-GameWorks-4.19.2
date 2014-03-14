// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

 #include "Slate.h"

 #include "LayoutUtils.h"

/**
 * Construct this widget
 *
 * @param	InArgs	The declaration data for this widget
 */
void SOverlay::Construct( const SOverlay::FArguments& InArgs )
{
	const int32 NumSlots = InArgs.Slots.Num();
	for ( int32 SlotIndex = 0; SlotIndex < NumSlots; ++SlotIndex )
	{
		Children.Add( InArgs.Slots[SlotIndex] );
	}
}


void SOverlay::ArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const
{
	for ( int32 ChildIndex = 0; ChildIndex < Children.Num(); ++ChildIndex )
	{
		const FOverlaySlot& CurChild = Children[ChildIndex];
		const EVisibility ChildVisibility = CurChild.Widget->GetVisibility();
		if ( ArrangedChildren.Accepts(ChildVisibility) )
		{
			const FMargin SlotPadding(CurChild.SlotPadding.Get());
			AlignmentArrangeResult XResult = AlignChild<Orient_Horizontal>(AllottedGeometry.Size.X, CurChild, SlotPadding);
			AlignmentArrangeResult YResult = AlignChild<Orient_Vertical>(AllottedGeometry.Size.Y, CurChild, SlotPadding);

			ArrangedChildren.AddWidget( ChildVisibility, AllottedGeometry.MakeChild(
				CurChild.Widget,
				FVector2D(XResult.Offset,YResult.Offset),
				FVector2D(XResult.Size, YResult.Size)
			) );
		}
	}
}


/**
 * A Panel's desired size in the space required to arrange of its children on the screen while respecting all of
 * the children's desired sizes and any layout-related options specified by the user. See StackPanel for an example.
 *
 * @return The desired size.
 */
FVector2D SOverlay::ComputeDesiredSize() const
{
	FVector2D MaxSize(0,0);
	for ( int32 ChildIndex=0; ChildIndex < Children.Num(); ++ChildIndex )
	{
		const FOverlaySlot& CurSlot = Children[ChildIndex];
		const EVisibility ChildVisibilty = CurSlot.Widget->GetVisibility();
		if ( ChildVisibilty != EVisibility::Collapsed )
		{
			FVector2D ChildDesiredSize = CurSlot.Widget->GetDesiredSize() + CurSlot.SlotPadding.Get().GetDesiredSize();
			MaxSize.X = FMath::Max( MaxSize.X, ChildDesiredSize.X );
			MaxSize.Y = FMath::Max( MaxSize.Y, ChildDesiredSize.Y );
		}
	}

	return MaxSize;
}

/** @return  The children of a panel in a slot-agnostic way. */
FChildren* SOverlay::GetChildren()
{
	return &Children;
}

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
int32 SOverlay::OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
{
	FArrangedChildren ArrangedChildren(EVisibility::Visible);
	{
#if SLATE_HD_STATS
		SCOPE_CYCLE_COUNTER( STAT_SlateOnPaint_SOverlay );
#endif
		// The box panel has no visualization of its own; it just visualizes its children.
		this->ArrangeChildren(AllottedGeometry, ArrangedChildren);
	}

	// Because we paint multiple children, we must track the maximum layer id that they produced in case one of our parents
	// wants to an overlay for all of its contents.
	int32 MaxLayerId = LayerId;

	for (int32 ChildIndex = 0; ChildIndex < ArrangedChildren.Num(); ++ChildIndex)
	{
		FArrangedWidget& CurWidget = ArrangedChildren(ChildIndex);
		FSlateRect ChildClipRect = MyClippingRect.IntersectionWith( CurWidget.Geometry.GetClippingRect() );
		const int32 CurWidgetsMaxLayerId = CurWidget.Widget->OnPaint( CurWidget.Geometry, ChildClipRect, OutDrawElements, MaxLayerId + 1, InWidgetStyle, ShouldBeEnabled( bParentEnabled ) );

		MaxLayerId = FMath::Max( MaxLayerId, CurWidgetsMaxLayerId );
	}

	return MaxLayerId;
}

SOverlay::FOverlaySlot& SOverlay::AddSlot( int32 ZOrder )
{
	FOverlaySlot& NewSlot = SOverlay::Slot();
	if ( ZOrder == INDEX_NONE )
	{
		// No ZOrder was specified; just add to the end of the list.
		// Use a ZOrder index one after the last elements.
		NewSlot.ZOrder = (Children.Num() == 0)
			? 0
			: ( Children[ Children.Num()-1 ].ZOrder + 1 );

		this->Children.Add( &NewSlot );
	}
	else
	{
		// Figure out where to add the widget based on ZOrder
		bool bFoundSlot = false;
		int32 CurSlotIndex = 0;
		for( ; CurSlotIndex < Children.Num(); ++CurSlotIndex )
		{
			const FOverlaySlot& CurSlot = Children[ CurSlotIndex ];
			if( ZOrder < CurSlot.ZOrder )
			{
				// Insert before
				bFoundSlot = true;
				break;
			}
		}

		// Add a slot at the desired location
		this->Children.Insert( &NewSlot, CurSlotIndex );
	}

	NewSlot.ZOrder = ZOrder;
	return NewSlot;
}

void SOverlay::RemoveSlot( int32 ZOrder )
{
	if (ZOrder != INDEX_NONE)
	{
		for( int32 ChildIndex=0; ChildIndex < Children.Num(); ++ChildIndex )
		{
			if ( Children[ChildIndex].ZOrder == ZOrder )
			{
				Children.RemoveAt( ChildIndex );
				return;
			}
		}

		ensureMsgf(false, TEXT("Could not remove slot. There are no children with ZOrder %d."));
	}
	else if (Children.Num() > 0)
	{
		Children.RemoveAt( Children.Num() - 1 );
	}
	else
	{
		ensureMsgf(false, TEXT("Could not remove slot. There are no slots left."));
	}
}

void SOverlay::ClearChildren()
{
	Children.Empty();
}

/** Returns the number of child widgets */
int32 SOverlay::GetNumWidgets() const
{
	return Children.Num();
}


void SOverlay::RemoveSlot( TSharedRef< SWidget > Widget )
{
	// Search and remove
	for( int32 CurSlotIndex = 0; CurSlotIndex < Children.Num(); ++CurSlotIndex )
	{
		const FOverlaySlot& CurSlot = Children[ CurSlotIndex ];
		if( CurSlot.Widget == Widget )
		{
			Children.RemoveAt( CurSlotIndex );
			break;
		}
	}
}
