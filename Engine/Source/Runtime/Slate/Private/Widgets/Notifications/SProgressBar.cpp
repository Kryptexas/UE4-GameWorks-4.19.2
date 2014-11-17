// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlatePrivatePCH.h"


void SProgressBar::Construct( const FArguments& InArgs )
{
	check(InArgs._Style);
	
	Style = InArgs._Style;

	Percent = InArgs._Percent;

	BarFillType = InArgs._BarFillType;
	
	BackgroundImage = InArgs._BackgroundImage;
	FillImage = InArgs._FillImage;
	MarqueeImage = InArgs._MarqueeImage;
	
	FillColorAndOpacity = InArgs._FillColorAndOpacity;
	BorderPadding = InArgs._BorderPadding;

	CurveSequence = FCurveSequence(0.0f, 0.5f);
	CurveSequence.Play();
}

void SProgressBar::SetPercent(TAttribute< TOptional<float> > InPercent)
{
	Percent = InPercent;
}

void SProgressBar::SetStyle(const FProgressBarStyle* InStyle)
{
	Style = InStyle;
}

void SProgressBar::SetBarFillType(EProgressBarFillType::Type InBarFillType)
{
	BarFillType = InBarFillType;
}

void SProgressBar::SetFillColorAndOpacity(TAttribute< FSlateColor > InFillColorAndOpacity)
{
	FillColorAndOpacity = InFillColorAndOpacity;
}

void SProgressBar::SetBorderPadding(TAttribute< FVector2D > InBorderPadding)
{
	BorderPadding = InBorderPadding;
}

void SProgressBar::SetBackgroundImage(const FSlateBrush* InBackgroundImage)
{
	BackgroundImage = InBackgroundImage;
}

void SProgressBar::SetFillImage(const FSlateBrush* InFillImage)
{
	FillImage = InFillImage;
}

void SProgressBar::SetMarqueeImage(const FSlateBrush* InMarqueeImage)
{
	MarqueeImage = InMarqueeImage;
}

const FSlateBrush* SProgressBar::GetBackgroundImage() const
{
	return BackgroundImage ? BackgroundImage : &Style->BackgroundImage;
}

const FSlateBrush* SProgressBar::GetFillImage() const
{
	return FillImage ? FillImage : &Style->FillImage;
}

const FSlateBrush* SProgressBar::GetMarqueeImage() const
{
	return MarqueeImage ? MarqueeImage : &Style->MarqueeImage;
}

int32 SProgressBar::OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
{
	// Used to track the layer ID we will return.
	int32 RetLayerId = LayerId;

	bool bEnabled = ShouldBeEnabled( bParentEnabled );
	const ESlateDrawEffect::Type DrawEffects = bEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
	
	const FSlateBrush* CurrentFillImage = GetFillImage();
	
	const FColor FillColorAndOpacitySRGB( InWidgetStyle.GetColorAndOpacityTint() * FillColorAndOpacity.Get().GetColor(InWidgetStyle) * CurrentFillImage->GetTint( InWidgetStyle ) );
	const FColor ColorAndOpacitySRGB = InWidgetStyle.GetColorAndOpacityTint();

	TOptional<float> ProgressFraction = Percent.Get();	

	// Paint inside the border only. 
	FPaintGeometry ForegroundPaintGeometry = AllottedGeometry.ToInflatedPaintGeometry( -BorderPadding.Get() );
	const FSlateRect ForegroundClippingRect = ForegroundPaintGeometry.ToSlateRect().IntersectionWith( MyClippingRect );
	
	const FSlateBrush* CurrentBackgroundImage = GetBackgroundImage();

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		RetLayerId++,
		AllottedGeometry.ToPaintGeometry(),
		CurrentBackgroundImage,
		MyClippingRect,
		DrawEffects,
		InWidgetStyle.GetColorAndOpacityTint() * CurrentBackgroundImage->GetTint( InWidgetStyle )
	);	
	
	if( ProgressFraction.IsSet() )
	{
		switch (BarFillType)
		{
			case EProgressBarFillType::RightToLeft:
			{
				const float ClampedFraction = FMath::Clamp( ProgressFraction.GetValue(), 0.0f, 1.0f  );
				// Draw Fill
				FSlateDrawElement::MakeBox(
					OutDrawElements,
					RetLayerId++,
					AllottedGeometry.ToPaintGeometry(
						FVector2D( AllottedGeometry.Size.X - (AllottedGeometry.Size.X * ( ClampedFraction )) , 0.0f),
						FVector2D( AllottedGeometry.Size.X * ( ClampedFraction ) , AllottedGeometry.Size.Y )),
					CurrentFillImage,
					ForegroundClippingRect,
					DrawEffects,
					FillColorAndOpacitySRGB
					);
				break;
			}
			case EProgressBarFillType::FillFromCenter:
			{
				const float ClampedFraction = FMath::Clamp( ProgressFraction.GetValue(), 0.0f, 1.0f  );
				// Draw Fill
				FSlateDrawElement::MakeBox(
					OutDrawElements,
					RetLayerId++,
					AllottedGeometry.ToPaintGeometry(
						FVector2D( (AllottedGeometry.Size.X * 0.5f) - ((AllottedGeometry.Size.X * ( ClampedFraction ))*0.5), 0.0f),
						FVector2D( AllottedGeometry.Size.X * ( ClampedFraction ) , AllottedGeometry.Size.Y )),
					CurrentFillImage,
					ForegroundClippingRect,
					DrawEffects,
					FillColorAndOpacitySRGB
					);
				break;
			}
			case EProgressBarFillType::LeftToRight:
			default:
			{
				const float ClampedFraction = FMath::Clamp( ProgressFraction.GetValue(), 0.0f, 1.0f  );
				// Draw Fill
				FSlateDrawElement::MakeBox(
					OutDrawElements,
					RetLayerId++,
					AllottedGeometry.ToPaintGeometry(
						FVector2D::ZeroVector,
						FVector2D( AllottedGeometry.Size.X * ( ClampedFraction ) , AllottedGeometry.Size.Y )),
					CurrentFillImage,
					ForegroundClippingRect,
					DrawEffects,
					FillColorAndOpacitySRGB
					);
				break;
			}
		}
		
	}
	else
	{
		const FSlateBrush* CurrentMarqueeImage = GetMarqueeImage();
		
		// Draw Marquee
		const float MarqueeAnimOffset = CurrentMarqueeImage->ImageSize.X * CurveSequence.GetLerpLooping();
		const float MarqueeImageSize = CurrentMarqueeImage->ImageSize.X;

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			RetLayerId++,
			AllottedGeometry.ToPaintGeometry(
				FVector2D( MarqueeAnimOffset - MarqueeImageSize, 0.0f ),
				FVector2D( AllottedGeometry.Size.X + MarqueeImageSize, AllottedGeometry.Size.Y )),
			CurrentMarqueeImage,
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
	return GetMarqueeImage()->ImageSize;
}
