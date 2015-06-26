// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SlatePrivatePCH.h"

#include "LayoutUtils.h"
#include "SScaleBox.h"


/* SScaleBox interface
 *****************************************************************************/

void SScaleBox::Construct( const SScaleBox::FArguments& InArgs )
{
	Stretch = InArgs._Stretch;
	StretchDirection = InArgs._StretchDirection;
	UserSpecifiedScale = InArgs._UserSpecifiedScale;

	ChildSlot
	.HAlign(InArgs._HAlign)
	.VAlign(InArgs._VAlign)
	[
		InArgs._Content.Widget
	];
}

/* SWidget overrides
 *****************************************************************************/

void SScaleBox::OnArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const
{
	const EVisibility ChildVisibility = ChildSlot.GetWidget()->GetVisibility();
	if ( ArrangedChildren.Accepts(ChildVisibility) )
	{
		const FVector2D AreaSize = AllottedGeometry.GetLocalSize();
		FVector2D DesiredSize = ChildSlot.GetWidget()->GetDesiredSize();

		float FinalScale = 1;

		const EStretch::Type CurrentStretch = Stretch.Get();
		const EStretchDirection::Type CurrentStretchDirection = StretchDirection.Get();

		if ( DesiredSize.X != 0 && DesiredSize.Y != 0 )
		{
			switch ( CurrentStretch )
			{
			case EStretch::None:
				break;
			case EStretch::Fill:
				DesiredSize = AreaSize;
				break;
			case EStretch::ScaleToFit:
				FinalScale = FMath::Min(AreaSize.X / DesiredSize.X, AreaSize.Y / DesiredSize.Y);
				break;
			case EStretch::ScaleToFill:
				FinalScale = FMath::Max(AreaSize.X / DesiredSize.X, AreaSize.Y / DesiredSize.Y);
				break;
			case EStretch::UserSpecified:
				FinalScale = UserSpecifiedScale.Get(1.0f);
				break;
			}

			switch ( CurrentStretchDirection )
			{
			case EStretchDirection::DownOnly:
				FinalScale = FMath::Min(FinalScale, 1.0f);
				break;
			case EStretchDirection::UpOnly:
				FinalScale = FMath::Max(FinalScale, 1.0f);
				break;
			case EStretchDirection::Both:
				break;
			}
		}

		FVector2D FinalOffset(0, 0);

		// If we're just filling, there's no scale applied, we're just filling the area.
		if ( CurrentStretch != EStretch::Fill )
		{
			const FMargin SlotPadding(ChildSlot.SlotPadding.Get());
			AlignmentArrangeResult XResult = AlignChild<Orient_Horizontal>(AreaSize.X, ChildSlot, SlotPadding, FinalScale, false);
			AlignmentArrangeResult YResult = AlignChild<Orient_Vertical>(AreaSize.Y, ChildSlot, SlotPadding, FinalScale, false);

			FinalOffset = FVector2D(XResult.Offset, YResult.Offset) / FinalScale;

			// If the layout horizontally is fill, then we need the desired size to be the whole size of the widget, 
			// but scale the inverse of the scale we're applying.
			if ( ChildSlot.HAlignment == HAlign_Fill )
			{
				DesiredSize.X = AreaSize.X / FinalScale;
			}

			// If the layout vertically is fill, then we need the desired size to be the whole size of the widget, 
			// but scale the inverse of the scale we're applying.
			if ( ChildSlot.VAlignment == VAlign_Fill )
			{
				DesiredSize.Y = AreaSize.Y / FinalScale;
			}
		}

		ArrangedChildren.AddWidget(ChildVisibility, AllottedGeometry.MakeChild(
			ChildSlot.GetWidget(),
			FinalOffset,
			DesiredSize,
			FinalScale
		) );
	}
}

int32 SScaleBox::OnPaint( const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
{
	return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
}

void SScaleBox::SetContent(TSharedRef<SWidget> InContent)
{
	ChildSlot
	[
		InContent
	];
}

void SScaleBox::SetHAlign(EHorizontalAlignment HAlign)
{
	ChildSlot.HAlignment = HAlign;
}

void SScaleBox::SetVAlign(EVerticalAlignment VAlign)
{
	ChildSlot.VAlignment = VAlign;
}

void SScaleBox::SetStretchDirection(EStretchDirection::Type InStretchDirection)
{
	StretchDirection = InStretchDirection;
}

void SScaleBox::SetStretch(EStretch::Type InStretch)
{
	Stretch = InStretch;
}

void SScaleBox::SetUserSpecifiedScale(float InUserSpecifiedScale)
{
	UserSpecifiedScale = InUserSpecifiedScale;
}
