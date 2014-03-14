// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"
#include "SCanvas.h"
#include "LayoutUtils.h"


/**
 * Construct this widget
 *
 * @param	InArgs	The declaration data for this widget
 */
void SCanvas::Construct( const SCanvas::FArguments& InArgs )
{
	const int32 NumSlots = InArgs.Slots.Num();
	for ( int32 SlotIndex = 0; SlotIndex < NumSlots; ++SlotIndex )
	{
		Children.Add( InArgs.Slots[SlotIndex] );
	}
}


/**
 * Panels arrange their children in a space described by the AllottedGeometry parameter. The results of the arrangement
 * should be returned by appending a FArrangedWidget pair for every child widget. See StackPanel for an example
 *
 * @param AllottedGeometry    The geometry allotted for this widget by its parent.
 * @param ArrangedChildren    The array to which to add the WidgetGeometries that represent the arranged children.
 */
void SCanvas::ArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const
{
	if (Children.Num() > 0)
	{
		for( int32 ChildIndex=0; ChildIndex < Children.Num(); ++ChildIndex )
		{
			const SCanvas::FSlot& CurChild = Children[ChildIndex];
			//grab the size
			const FVector2D Size = CurChild.SizeAttr.Get();
			//Handle HAlignment
			FVector2D Offset(0,0);
			switch (CurChild.HAlignment)
			{
			case HAlign_Center:
				Offset.X = -Size.X / 2.0f;
				break;
			case HAlign_Right:
				Offset.X = -Size.X;
				break;
			}

			//handle VAlignment
			switch (CurChild.VAlignment)
			{
			case VAlign_Bottom:
				Offset.Y = -Size.Y;
				break;
			case VAlign_Center:
				Offset.Y = -Size.Y / 2.0f;
				break;
			}
			// Add the information about this child to the output list (ArrangedChildren)
			ArrangedChildren.AddWidget( AllottedGeometry.MakeChild(
				// The child widget being arranged
				CurChild.Widget,
				// Child's local position (i.e. position within parent)
				CurChild.PositionAttr.Get() + Offset,
				// Child's size
				Size
			));
		}
	}
}


int32 SCanvas::OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
{
	FArrangedChildren ArrangedChildren(EVisibility::Visible);
	this->ArrangeChildren(AllottedGeometry, ArrangedChildren);

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


/**
 * A Panel's desired size in the space required to arrange of its children on the screen while respecting all of
 * the children's desired sizes and any layout-related options specified by the user. See StackPanel for an example.
 *
 * @return The desired size.
 */
FVector2D SCanvas::ComputeDesiredSize() const
{
	// Canvas widgets have no desired size -- their size is always determined by their container
	return FVector2D::ZeroVector;
}


/** @return  The children of a panel in a slot-agnostic way. */
FChildren* SCanvas::GetChildren()
{
	return &Children;
}

/** 
* Removes a slot from this box panel which contains the specified SWidget
*
* @param SlotWidget The widget to match when searching through the slots
* @returns The index in the children array where the slot was removed and -1 if no slot was found matching the widget
*/
int32 SCanvas::RemoveSlot( const TSharedRef<SWidget>& SlotWidget )
{
	for (int32 SlotIdx = 0; SlotIdx < Children.Num(); ++SlotIdx)
	{
		if ( SlotWidget == Children[SlotIdx].Widget )
		{
			Children.RemoveAt(SlotIdx);
			return SlotIdx;
		}
	}

	return -1;
}

void SCanvas::ClearChildren()
{
	Children.Empty();
}
