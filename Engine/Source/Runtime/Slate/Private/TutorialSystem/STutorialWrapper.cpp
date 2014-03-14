// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"
#include "STutorialWrapper.h"

FOnWidgetHighlight STutorialWrapper::OnWidgetHighlightDelegate;

namespace TutorialConstants
{
	const float BorderPulseAnimationLength = 1.0f;
	const float BorderIntroAnimationLength = 0.5f;
	const float MinBorderOpacity = 0.1f;
	const float ShadowScale = 8.0f;
	const float MaxBorderOffset = 8.0f;
}

void STutorialWrapper::Construct( const FArguments& InArgs )
{
	Name = InArgs._Name;

	BorderIntroAnimation.AddCurve(0.0f, TutorialConstants::BorderIntroAnimationLength, ECurveEaseFunction::CubicOut);
	BorderPulseAnimation.AddCurve(0.0f, TutorialConstants::BorderPulseAnimationLength, ECurveEaseFunction::Linear);

	bIsPlaying = false;

	SBorder::Construct(SBorder::FArguments()
		.BorderImage(FStyleDefaults::GetNoBrush())
		.Padding(0.0f)
		[
			InArgs._Content.Widget
		]);
}

void STutorialWrapper::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	if(OnWidgetHighlightDelegate.IsBound() && OnWidgetHighlightDelegate.Execute(Name))
	{
		if(!bIsPlaying)
		{
			BorderIntroAnimation.Play();
			BorderPulseAnimation.Play();
		}

		bIsPlaying = true;
	}
	else
	{
		bIsPlaying = false;
	}

	CachedGeometry = AllottedGeometry;
}

int32 STutorialWrapper::OnPaint(const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
{
	if(OnWidgetHighlightDelegate.IsBound() && OnWidgetHighlightDelegate.Execute(Name))
	{
		float AlphaFactor;
		float PulseFactor;
		FLinearColor ShadowTint;
		FLinearColor BorderTint;
		GetAnimationValues(AlphaFactor, PulseFactor, ShadowTint, BorderTint);

		const FSlateBrush* ShadowBrush = FCoreStyle::Get().GetBrush(TEXT("Tutorials.Shadow"));
		const FSlateBrush* BorderBrush = FCoreStyle::Get().GetBrush(TEXT("Tutorials.Border"));
					
		const FGeometry& WidgetGeometry = CachedGeometry;
		const FVector2D WindowPos = OutDrawElements.GetWindow()->GetPositionInScreen();
		const FVector2D WindowSize = OutDrawElements.GetWindow()->GetSizeInScreen();

		// We should be clipped by the window size, not our containing widget, as we want to draw outside the widget
		FSlateRect WindowClippingRect(0.0f, 0.0f, WindowSize.X, WindowSize.Y);
		// We want to draw on-top of everything else
		int32 OverlayLayerId = MAX_int32 - 2;

		FPaintGeometry ShadowGeometry((WidgetGeometry.AbsolutePosition - FVector2D(ShadowBrush->Margin.Left, ShadowBrush->Margin.Top) * ShadowBrush->ImageSize * WidgetGeometry.Scale * TutorialConstants::ShadowScale) - WindowPos,
										((WidgetGeometry.Size * WidgetGeometry.Scale) + (FVector2D(ShadowBrush->Margin.Right * 2.0f, ShadowBrush->Margin.Bottom * 2.0f) * ShadowBrush->ImageSize * WidgetGeometry.Scale * TutorialConstants::ShadowScale)),
										WidgetGeometry.Scale * TutorialConstants::ShadowScale);
		// draw highlight shadow
		FSlateDrawElement::MakeBox(OutDrawElements, OverlayLayerId + 0, ShadowGeometry, ShadowBrush, WindowClippingRect, ESlateDrawEffect::None, ShadowTint);

		FVector2D PulseOffset = FVector2D(PulseFactor * TutorialConstants::MaxBorderOffset, PulseFactor * TutorialConstants::MaxBorderOffset);

		FVector2D BorderPosition = (WidgetGeometry.AbsolutePosition - ((FVector2D(BorderBrush->Margin.Left, BorderBrush->Margin.Top) * BorderBrush->ImageSize * WidgetGeometry.Scale) + PulseOffset)) - WindowPos;
		FVector2D BorderSize = ((WidgetGeometry.Size * WidgetGeometry.Scale) + (PulseOffset * 2.0f) + (FVector2D(BorderBrush->Margin.Right * 2.0f, BorderBrush->Margin.Bottom * 2.0f) * BorderBrush->ImageSize * WidgetGeometry.Scale));

		FPaintGeometry BorderGeometry(BorderPosition, BorderSize, WidgetGeometry.Scale);

		// draw highlight border
		FSlateDrawElement::MakeBox(OutDrawElements, OverlayLayerId + 1, BorderGeometry, BorderBrush, WindowClippingRect, ESlateDrawEffect::None, BorderTint);
	}

	return SBorder::OnPaint(AllottedGeometry, MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
}

void STutorialWrapper::GetAnimationValues(float& OutAlphaFactor, float& OutPulseFactor, FLinearColor& OutShadowTint, FLinearColor& OutBorderTint) const
{
	if(BorderIntroAnimation.IsPlaying())
	{
		OutAlphaFactor = BorderIntroAnimation.GetLerp();
		OutPulseFactor = (1.0f - OutAlphaFactor) * 50.0f;
		OutShadowTint = FLinearColor(1.0f, 1.0f, 0.0f, OutAlphaFactor);
		OutBorderTint = FLinearColor(1.0f, 1.0f, 0.0f, OutAlphaFactor * OutAlphaFactor);
	}
	else
	{
		OutAlphaFactor = 1.0f - (0.5f + (FMath::Cos(2.0f * PI * BorderPulseAnimation.GetLerpLooping()) * 0.5f));
		OutPulseFactor = 0.5f + (FMath::Cos(2.0f * PI * BorderPulseAnimation.GetLerpLooping()) * 0.5f);
		OutShadowTint = FLinearColor(1.0f, 1.0f, 0.0f, 1.0f);
		OutBorderTint = FLinearColor(1.0f, 1.0f, 0.0f, TutorialConstants::MinBorderOpacity + ((1.0f - TutorialConstants::MinBorderOpacity) * OutAlphaFactor));
	}
}

FOnWidgetHighlight& STutorialWrapper::OnWidgetHighlight()
{
	return OnWidgetHighlightDelegate;
}
