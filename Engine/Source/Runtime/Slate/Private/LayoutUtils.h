// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

struct AlignmentArrangeResult
{
	AlignmentArrangeResult( float InOffset, float InSize )
	: Offset(InOffset)
	, Size(InSize)
	{
	}
	
	float Offset;
	float Size;
};

namespace ArrangeUtils
{
	/** Gets the alignment of an axis-agnostic int32 so that we can do aligment on an axis without caring about its orientation */
	template<EOrientation Orientation>
	struct GetChildAlignment
	{
		template<typename SlotType>
		static int32 AsInt( const SlotType& InSlot );
	};

	template<>
	struct GetChildAlignment<Orient_Horizontal>
	{
		template<typename SlotType>
		static int32 AsInt( const SlotType& InSlot )
		{
			return static_cast<int32>(InSlot.HAlignment);
		}
	};

	template<>
	struct GetChildAlignment<Orient_Vertical>
	{
		template<typename SlotType>
		static int32 AsInt( const SlotType& InSlot )
		{
			return static_cast<int32>(InSlot.VAlignment);
		}
	};

}





/**
 * Helper method to BoxPanel::ArrangeChildren.
 * 
 * @param AllottedSize         The size available to arrange the widget along the given orientation
 * @param ChildToArrange       The widget and associated layout information
 * 
 * @return  Offset and Size of widget
 */
template<EOrientation Orientation, typename SlotType>
static AlignmentArrangeResult AlignChild( float AllottedSize, const SlotType& ChildToArrange, const FMargin& SlotPadding, const float& ContentScale = 1.0f )
{
	float ChildDesiredSize = (Orientation == Orient_Horizontal)
		? ( ChildToArrange.Widget->GetDesiredSize().X * ContentScale )
		: ( ChildToArrange.Widget->GetDesiredSize().Y * ContentScale );

	const FMargin& Margin = SlotPadding;
	const float TotalMargin = Margin.GetTotalSpaceAlong<Orientation>();	
	const float MarginPre = (Orientation == Orient_Horizontal) ? Margin.Left : Margin.Top;
	const float MarginPost = (Orientation == Orient_Horizontal) ? Margin.Right : Margin.Bottom;
	
	const int32 Alignment = ArrangeUtils::GetChildAlignment<Orientation>::AsInt(ChildToArrange);
		
	const float ChildSize = FMath::Min(ChildDesiredSize, AllottedSize-TotalMargin);
		
	switch( Alignment )	
	{
		default:
		case HAlign_Fill:
			return AlignmentArrangeResult( MarginPre, (AllottedSize - TotalMargin)*ContentScale );
			break;
		
		case HAlign_Left: // same as Align_Top
			return AlignmentArrangeResult( MarginPre, ChildSize );
			break;
		
		case HAlign_Center:
			return AlignmentArrangeResult( (AllottedSize - ChildSize ) / 2.0f + MarginPre - MarginPost, ChildSize );
			break;
		
		case HAlign_Right: // same as Align_Bottom		
			return AlignmentArrangeResult( AllottedSize - ChildSize - MarginPost, ChildSize );
			break;
	}
}


/**
 * Arrange a ChildSlot within the AllottedGeometry and populate ArrangedChildren with the arranged result.
 * The code makes certain assumptions about the type of ChildSlot.
 */
template<typename SlotType>
void ArrangeSingleChild( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren, const SlotType& ChildSlot, const TAttribute<FVector2D>& ContentScale )
{
	const EVisibility ChildVisibility = ChildSlot.Widget->GetVisibility();
	if ( ArrangedChildren.Accepts(ChildVisibility) )
	{
		const FVector2D ThisContentScale = ContentScale.Get();
		const FMargin SlotPadding(ChildSlot.SlotPadding.Get());
		AlignmentArrangeResult XResult = AlignChild<Orient_Horizontal>(AllottedGeometry.Size.X, ChildSlot, SlotPadding, ThisContentScale.X);
		AlignmentArrangeResult YResult = AlignChild<Orient_Vertical>(AllottedGeometry.Size.Y, ChildSlot, SlotPadding, ThisContentScale.Y);

		ArrangedChildren.AddWidget( ChildVisibility, AllottedGeometry.MakeChild(
				ChildSlot.Widget,
				FVector2D(XResult.Offset,YResult.Offset),
				FVector2D(XResult.Size, YResult.Size)
		) );
	}

}