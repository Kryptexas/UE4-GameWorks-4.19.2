// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"
#include "LayoutUtils.h"

void SBox::Construct( const FArguments& InArgs )
{
	WidthOverride = InArgs._WidthOverride;
	HeightOverride = InArgs._HeightOverride;

	ChildSlot
		.HAlign( InArgs._HAlign )
		.VAlign( InArgs._VAlign )
		.Padding( InArgs._Padding )
	[
		InArgs._Content.Widget
	];
}



FVector2D SBox::ComputeDesiredSize() const
{
	EVisibility ChildVisibility = ChildSlot.Widget->GetVisibility();

	if ( ChildVisibility != EVisibility::Collapsed )
	{
		// If the user specified a fixed width or height, those values override the Box's content.
		const FVector2D& UnmodifiedChildDesiredSize = ChildSlot.Widget->GetDesiredSize() + ChildSlot.SlotPadding.Get().GetDesiredSize();
		const FOptionalSize CurrentWidthOverride = WidthOverride.Get();
		const FOptionalSize CurrentHeightOverride = HeightOverride.Get();
		return FVector2D(
			(CurrentWidthOverride.IsSet()) ? CurrentWidthOverride.Get() : UnmodifiedChildDesiredSize.X,
			(CurrentHeightOverride.IsSet()) ? CurrentHeightOverride.Get() : UnmodifiedChildDesiredSize.Y
		);
	}
	
	return FVector2D::ZeroVector;
}

void SBox::ArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const
{
	const EVisibility& MyCurrentVisibility = this->GetVisibility();
	if ( ArrangedChildren.Accepts( MyCurrentVisibility ) )
	{
		const FMargin SlotPadding(ChildSlot.SlotPadding.Get());
		AlignmentArrangeResult XAlignmentResult = AlignChild<Orient_Horizontal>( AllottedGeometry.Size.X, ChildSlot, SlotPadding );
		AlignmentArrangeResult YAlignmentResult = AlignChild<Orient_Vertical>( AllottedGeometry.Size.Y, ChildSlot, SlotPadding );

		ArrangedChildren.AddWidget(
			AllottedGeometry.MakeChild(
				ChildSlot.Widget,
				FVector2D(XAlignmentResult.Offset, YAlignmentResult.Offset),
				FVector2D(XAlignmentResult.Size, YAlignmentResult.Size)
			)
		);
	}
}

FChildren* SBox::GetChildren()
{
	return &ChildSlot;
}

int32 SBox::OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
{
	// An SBox just draws its only child
	FArrangedChildren ArrangedChildren(EVisibility::Visible);
	{
#if SLATE_HD_STATS
		SCOPE_CYCLE_COUNTER( STAT_SlateOnPaint_SBox );
#endif
		this->ArrangeChildren(AllottedGeometry, ArrangedChildren);
	}

	// Maybe none of our children are visible
	if( ArrangedChildren.Num() > 0 )
	{
		check( ArrangedChildren.Num() == 1 );
		FArrangedWidget& TheChild = ArrangedChildren(0);

		const FSlateRect ChildClippingRect = AllottedGeometry.GetClippingRect().InsetBy( ChildSlot.SlotPadding.Get() * AllottedGeometry.Scale ).IntersectionWith(MyClippingRect);

		return TheChild.Widget->OnPaint( TheChild.Geometry, ChildClippingRect, OutDrawElements, LayerId, InWidgetStyle, ShouldBeEnabled( bParentEnabled ) );
	}
	return LayerId;
}


