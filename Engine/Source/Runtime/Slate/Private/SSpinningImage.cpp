// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"

void SSpinningImage::Construct(const FArguments& InArgs)
{
	Image = InArgs._Image;
	ColorAndOpacity = InArgs._ColorAndOpacity;
	OnMouseButtonDownHandler = InArgs._OnMouseButtonDown;
	
	Sequence = FCurveSequence();
	Curve = Sequence.AddCurve(0.f, InArgs._Period);
	Sequence.Play();
}

int32 SSpinningImage::OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
{
	const FSlateBrush* ImageBrush = Image.Get();

	if ((ImageBrush != NULL) && (ImageBrush->DrawAs != ESlateBrushDrawType::NoDrawType))
	{
		const bool bIsEnabled = ShouldBeEnabled(bParentEnabled);
		const uint32 DrawEffects = bIsEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;

		const FColor FinalColorAndOpacity( InWidgetStyle.GetColorAndOpacityTint() * ColorAndOpacity.Get().GetColor(InWidgetStyle) * ImageBrush->GetTint( InWidgetStyle ) );
		
		const float Angle = Curve.GetLerpLooping() * 2.0f * PI;

		FSlateDrawElement::MakeRotatedBox( 
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			ImageBrush,
			MyClippingRect,
			DrawEffects,
			Angle,
			TOptional<FVector2D>(), // Will auto rotate about center
			FSlateDrawElement::RelativeToElement,
			FinalColorAndOpacity
			);
	}
	return LayerId;
}