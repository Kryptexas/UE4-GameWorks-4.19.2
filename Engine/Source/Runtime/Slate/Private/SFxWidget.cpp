// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"


void SFxWidget::Construct( const FArguments& InArgs )
{
	RenderScale = InArgs._RenderScale;
	RenderScaleOrigin = InArgs._RenderScaleOrigin;
	LayoutScale = InArgs._LayoutScale;
	VisualOffset = InArgs._VisualOffset;
	bIgnoreClipping = InArgs._IgnoreClipping;
	ColorAndOpacity = InArgs._ColorAndOpacity;

	this->ChildSlot
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Center)
	[
		InArgs._Content.Widget
	];
}

void SFxWidget::SetVisualOffset( const TAttribute<FVector2D>& InOffset )
{
	VisualOffset = InOffset;
}

void SFxWidget::SetVisualOffset( FVector InOffset )
{
	VisualOffset = FVector2D(InOffset.X, InOffset.Y);
}

void SFxWidget::SetRenderScale( const TAttribute<float>& InScale )
{
	RenderScale = InScale;
}

void SFxWidget::SetRenderScale( float InScale )
{
	RenderScale = InScale;
}

void SFxWidget::SetColorAndOpacity( const TAttribute<FLinearColor>& InColorAndOpacity )
{
	ColorAndOpacity = InColorAndOpacity;
}

void SFxWidget::SetColorAndOpacity( FLinearColor InColorAndOpacity )
{
	ColorAndOpacity = InColorAndOpacity;
}

/**
 * Transform the geometry for rendering only:
 * 
 * @param RenderScale     Zoom by this amount.
 * @param ScaleOrigin     Where the origin of the transforms is within the object (range 0..1)
 * @param Offset          Offset in either direction as multiple of unscaled object size.
 */
static FGeometry MakeScaledGeometry( const FGeometry& Geometry, const float RenderScale, const FVector2D& ScaleOrigin, const FVector2D& Offset )
{
	const FVector2D CenteringAdjustment = Geometry.Size*ScaleOrigin*Geometry.Scale - Geometry.Size*ScaleOrigin*Geometry.Scale*RenderScale;
	const FVector2D OffsetAdjustment = Geometry.Size*Offset*Geometry.Scale;
	
	return FGeometry(
		FVector2D::ZeroVector,
		Geometry.AbsolutePosition + CenteringAdjustment + OffsetAdjustment,
		Geometry.Size,
		Geometry.Scale * RenderScale );
}

int32 SFxWidget::OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
{
	// Transformt the geometry for rendering only. Layout is unaffected.
	const FGeometry ModifiedGeometry = MakeScaledGeometry( AllottedGeometry, RenderScale.Get(), RenderScaleOrigin.Get(), VisualOffset.Get() );
	
	FArrangedChildren ArrangedChildren(EVisibility::Visible);
	this->ArrangeChildren(ModifiedGeometry, ArrangedChildren);

	// There may be zero elements in this array if our child collapsed/hidden
	if( ArrangedChildren.Num() > 0 )
	{
		// We can only have one direct descendant.
		check( ArrangedChildren.Num() == 1 );
		const FArrangedWidget& TheChild = ArrangedChildren(0);

		// SFxWidgets are able to ignore parent clipping.
		const FSlateRect ChildClippingRect = (bIgnoreClipping.Get())
			? ModifiedGeometry.GetClippingRect()
			: MyClippingRect.IntersectionWith(ModifiedGeometry.GetClippingRect());

		FWidgetStyle CompoundedWidgetStyle = FWidgetStyle(InWidgetStyle)
			.BlendColorAndOpacityTint(ColorAndOpacity.Get())
			.SetForegroundColor( ForegroundColor );

		return TheChild.Widget->OnPaint( TheChild.Geometry, ChildClippingRect, OutDrawElements, LayerId + 1, CompoundedWidgetStyle, ShouldBeEnabled( bParentEnabled ) );
	}
	return LayerId;

}


FVector2D SFxWidget::ComputeDesiredSize() const
{
	// Layout scale affects out desired size.
	return LayoutScale.Get() * ChildSlot.Widget->GetDesiredSize();
}


void SFxWidget::ArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const
{
	const EVisibility MyVisibility = this->GetVisibility();
	if ( ArrangedChildren.Accepts( MyVisibility ) )
	{
		const float MyDPIScale = LayoutScale.Get();

		// Only layout scale affects the arranged geometry. This part is identical to DPI scaling.
		ArrangedChildren.AddWidget( AllottedGeometry.MakeChild(
			this->ChildSlot.Widget,
			FVector2D::ZeroVector,
			AllottedGeometry.Size / MyDPIScale,
			MyDPIScale
		));
	}
}
