// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlatePrivatePCH.h"

#if WITH_FANCY_TEXT

#include "SlateTextHighlightRunRenderer.h"

FSlateTextHighlightRunRenderer::FSlateTextHighlightRunRenderer()
{

}

int32 FSlateTextHighlightRunRenderer::OnPaint( const FPaintArgs& Args, const FTextLayout::FLineView& Line, const TSharedRef< ISlateRun >& Run, const TSharedRef< ILayoutBlock >& Block, const FTextBlockStyle& DefaultStyle, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const 
{
	FVector2D Location( Block->GetLocationOffset() );
	Location.Y = Line.Offset.Y;

	// Draw the actual highlight rectangle
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		++LayerId,
		FPaintGeometry( AllottedGeometry.AbsolutePosition + Location, FVector2D( Block->GetSize().X, Line.Size.Y ), AllottedGeometry.Scale ),
		&DefaultStyle.HighlightShape,
		MyClippingRect,
		bParentEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect,
		InWidgetStyle.GetColorAndOpacityTint() * DefaultStyle.HighlightColor
		);

	FLinearColor InvertedHighlightColor = FLinearColor::White - DefaultStyle.HighlightColor;
	InvertedHighlightColor.A = InWidgetStyle.GetForegroundColor().A;

	FWidgetStyle WidgetStyle( InWidgetStyle );
	WidgetStyle.SetForegroundColor( InvertedHighlightColor );

	return Run->OnPaint( Args, Line, Block, DefaultStyle, AllottedGeometry, MyClippingRect, OutDrawElements, LayerId, WidgetStyle, bParentEnabled );
}

TSharedRef< FSlateTextHighlightRunRenderer > FSlateTextHighlightRunRenderer::Create()
{
	return MakeShareable( new FSlateTextHighlightRunRenderer() );
}



#endif //WITH_FANCY_TEXT