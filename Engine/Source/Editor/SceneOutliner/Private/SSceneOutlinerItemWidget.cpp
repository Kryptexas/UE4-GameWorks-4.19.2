// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "SceneOutlinerPrivatePCH.h"

namespace SceneOutliner
{
	const float SSceneOutlinerItemWidget::HighlightRectLeftOffset = 0.0f;
	const float SSceneOutlinerItemWidget::HighlightRectRightOffset = 0.0f;
	const float SSceneOutlinerItemWidget::HighlightTargetSpringConstant = 25.0f;
	const float SSceneOutlinerItemWidget::HighlightTargetEffectDuration = 0.5f;
	const float SSceneOutlinerItemWidget::HighlightTargetOpacity = 0.8f;
	const float SSceneOutlinerItemWidget::LabelChangedAnimOffsetPercent = 0.2f;

	SSceneOutlinerItemWidget::SSceneOutlinerItemWidget()
		: LastHighlightInteractionTime( -1000.0 )
	{
		// Setup springs
		FFloatSpring1D::FSpringConfig SpringConfig;
		SpringConfig.SpringConstant = HighlightTargetSpringConstant;
		HighlightTargetLeftSpring.SetConfig( SpringConfig );
		HighlightTargetRightSpring.SetConfig( SpringConfig );
	}

	void SSceneOutlinerItemWidget::Construct(const FArguments& InArgs)
	{
		ChildSlot
			[
				InArgs._Content.Widget
			];
	}

	void SSceneOutlinerItemWidget::FlashHighlight()
	{
		LastHighlightInteractionTime = FSlateApplication::Get().GetCurrentTime();
	}

	void SSceneOutlinerItemWidget::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
	{
		// Call parent implementation.
		SCompoundWidget::Tick( AllottedGeometry, InCurrentTime, InDeltaTime );

		// We'll draw with the 'focused' look if we're either focused or we have a context menu summoned
		const bool bShouldAppearFocused = HasKeyboardFocus();

		// Update highlight 'target' effect
		{
			const float HighlightLeftX = HighlightRectLeftOffset;
			const float HighlightRightX = HighlightRectRightOffset + AllottedGeometry.Size.X;

			HighlightTargetLeftSpring.SetTarget( HighlightLeftX );
			HighlightTargetRightSpring.SetTarget( HighlightRightX );

			float TimeSinceHighlightInteraction = (float)( InCurrentTime - LastHighlightInteractionTime );
			if( TimeSinceHighlightInteraction <= HighlightTargetEffectDuration || bShouldAppearFocused )
			{
				HighlightTargetLeftSpring.Tick( InDeltaTime );
				HighlightTargetRightSpring.Tick( InDeltaTime );
			}
		}
	}

	int32 SSceneOutlinerItemWidget::OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
	{
		int32 StartLayer = SCompoundWidget::OnPaint( AllottedGeometry, MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled );

		const int32 TextLayer = 1;	

		// See if a disabled effect should be used
		bool bEnabled = ShouldBeEnabled( bParentEnabled );
		ESlateDrawEffect::Type DrawEffects = (bEnabled) ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;

		const double CurrentTime = FSlateApplication::Get().GetCurrentTime();

		// We'll draw with the 'focused' look if we're either focused or we have a context menu summoned
		const bool bShouldAppearFocused = HasKeyboardFocus();

		const float DrawPositionY = ( AllottedGeometry.Size.Y / 2 ) - ( AllottedGeometry.Size.Y / 2 );

		// Draw highlight targeting effect
		const float TimeSinceHighlightInteraction = (float)( CurrentTime - LastHighlightInteractionTime );
		if( TimeSinceHighlightInteraction <= HighlightTargetEffectDuration )
		{

			// Compute animation progress
			float EffectAlpha = FMath::Clamp( TimeSinceHighlightInteraction / HighlightTargetEffectDuration, 0.0f, 1.0f );
			EffectAlpha = 1.0f - EffectAlpha * EffectAlpha;  // Inverse square falloff (looks nicer!)

			// Apply extra opacity falloff when dehighlighting
			float EffectOpacity = EffectAlpha;

			// Figure out a universally visible highlight color.
			FColor HighlightTargetColorAndOpacity = ( (FLinearColor::White - ColorAndOpacity.Get())*0.5f + FLinearColor(+0.4f, +0.1f, -0.2f)) * InWidgetStyle.GetColorAndOpacityTint();
			HighlightTargetColorAndOpacity.A = HighlightTargetOpacity * EffectOpacity * 255.0f;

			// Compute the bounds offset of the highlight target from where the highlight target spring
			// extents currently lie.  This is used to "grow" or "shrink" the highlight as needed.
			const float LabelChangedAnimOffset = LabelChangedAnimOffsetPercent * AllottedGeometry.Size.Y;

			// Choose an offset amount depending on whether we're highlighting, or clearing highlight
			const float EffectOffset = EffectAlpha * LabelChangedAnimOffset;

			const float HighlightLeftX = HighlightTargetLeftSpring.GetPosition() - EffectOffset;
			const float HighlightRightX = HighlightTargetRightSpring.GetPosition() + EffectOffset;
			const float HighlightTopY = 0.0f - LabelChangedAnimOffset;
			const float HighlightBottomY = AllottedGeometry.Size.Y + EffectOffset;

			const FVector2D DrawPosition = FVector2D( HighlightLeftX, HighlightTopY );
			const FVector2D DrawSize = FVector2D( HighlightRightX - HighlightLeftX, HighlightBottomY - HighlightTopY );

			const FSlateBrush* StyleInfo = FEditorStyle::GetBrush("SceneOutliner.ChangedItemHighlight");

			// NOTE: We rely on scissor clipping for the highlight rectangle
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId + TextLayer,
				AllottedGeometry.ToPaintGeometry( DrawPosition, DrawSize ),	// Position, Size, Scale
				StyleInfo,													// Style
				MyClippingRect,												// Clipping rect
				DrawEffects,												// Effects to use
				HighlightTargetColorAndOpacity );							// Color
		}

		return LayerId + TextLayer;
	}
}