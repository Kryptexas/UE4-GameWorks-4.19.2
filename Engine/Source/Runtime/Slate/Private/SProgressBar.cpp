// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"


void SProgressBar::Construct( const FArguments& InArgs )
{
	check(InArgs._Style);

	Percent = InArgs._Percent;

	BackgroundImage = &InArgs._Style->BackgroundImage;
	FillImage = &InArgs._Style->FillImage;
	MarqueeImage = &InArgs._Style->MarqueeImage;
	FillColorAndOpacity = InArgs._FillColorAndOpacity;
	BorderPadding = InArgs._BorderPadding;

	CurveSequence = FCurveSequence(0.0f, 0.5f);
	CurveSequence.Play();
}


int32 SProgressBar::OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
{
	// Used to track the layer ID we will return.
	int32 RetLayerId = LayerId;

	bool bEnabled = ShouldBeEnabled( bParentEnabled );
	const ESlateDrawEffect::Type DrawEffects = bEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
	
	const FColor FillColorAndOpacitySRGB( InWidgetStyle.GetColorAndOpacityTint() * FillColorAndOpacity.Get().GetColor(InWidgetStyle) * FillImage->GetTint( InWidgetStyle ) );
	const FColor ColorAndOpacitySRGB = InWidgetStyle.GetColorAndOpacityTint();

	TOptional<float> ProgressFraction = Percent.Get();	

	// Paint inside the border only. 
	FPaintGeometry ForegroundPaintGeometry = AllottedGeometry.ToInflatedPaintGeometry( -BorderPadding.Get() );
	const FSlateRect ForegroundClippingRect = ForegroundPaintGeometry.ToSlateRect().IntersectionWith( MyClippingRect );

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		RetLayerId++,
		AllottedGeometry.ToPaintGeometry(),
		BackgroundImage,
		MyClippingRect,
		DrawEffects,
		InWidgetStyle.GetColorAndOpacityTint() * BackgroundImage->GetTint( InWidgetStyle )
	);	
	
	if( ProgressFraction.IsSet() )
	{
		const float ClampedFraction = FMath::Clamp( ProgressFraction.GetValue(), 0.0f, 1.0f  );

		// Draw Fill
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			RetLayerId++,
			AllottedGeometry.ToPaintGeometry(
				FVector2D::ZeroVector,
				FVector2D( AllottedGeometry.Size.X * ( ClampedFraction ) , AllottedGeometry.Size.Y )),
			FillImage,
			ForegroundClippingRect,
			DrawEffects,
			FillColorAndOpacitySRGB
			);
		
	}
	else
	{
		// Draw Marquee
		const float MarqueeAnimOffset = MarqueeImage->ImageSize.X * CurveSequence.GetLerpLooping();
		const float MarqueeImageSize = MarqueeImage->ImageSize.X;

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			RetLayerId++,
			AllottedGeometry.ToPaintGeometry(
				FVector2D( MarqueeAnimOffset - MarqueeImageSize, 0.0f ),
				FVector2D( AllottedGeometry.Size.X + MarqueeImageSize, AllottedGeometry.Size.Y )),
			MarqueeImage,
			ForegroundClippingRect,
			DrawEffects,
			ColorAndOpacitySRGB
			);

	}

	return RetLayerId - 1;
}

/**
 * Computes the desired size of this widget (SWidget)
 *
 * @return  The widget's desired size
 */
FVector2D SProgressBar::ComputeDesiredSize() const
{
	return MarqueeImage->ImageSize;
}


