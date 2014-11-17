// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SCompoundWidget.h"

namespace SceneOutliner
{
	class SSceneOutlinerItemWidget : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS( SSceneOutlinerItemWidget )
			: _Content()
			{}
	
			SLATE_DEFAULT_SLOT( FArguments, Content )

		SLATE_END_ARGS()

		/** Constructor */
		SSceneOutlinerItemWidget();

		void Construct(const FArguments& InArgs);

		void FlashHighlight();

	protected:

		virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) override;

		virtual int32 OnPaint( const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const override;

	private:
		/** How many pixels to extend the highlight rectangle's left side horizontally */
		static const float HighlightRectLeftOffset;

		/** How many pixels to extend the highlight rectangle's right side horizontally */
		static const float HighlightRectRightOffset;

		/** How quickly the highlight 'targeting' rectangle will slide around.  Larger is faster. */
		static const float HighlightTargetSpringConstant;

		/** Duration of animation highlight target effects */
		static const float HighlightTargetEffectDuration;

		/** Opacity of the highlight target effect overlay */
		static const float HighlightTargetOpacity;

		/** How large the highlight target effect will be when highlighting, as a scalar percentage of font height */
		static const float LabelChangedAnimOffsetPercent;

		/** Highlight "targeting" visual effect left position */
		FFloatSpring1D HighlightTargetLeftSpring;

		/** Highlight "targeting" visual effect right position */
		FFloatSpring1D HighlightTargetRightSpring;

		/** Last time that the user had a major interaction with the highlight */
		double LastHighlightInteractionTime;
	};
}