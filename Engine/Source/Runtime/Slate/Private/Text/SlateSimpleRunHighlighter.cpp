// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"
#include "SlateSimpleRunHighlighter.h"

FNoChildren FSlateSimpleRunHighlighter::NoChildrenInstance;

FSlateSimpleRunHighlighter::FSlateSimpleRunHighlighter()
{

}

void FSlateSimpleRunHighlighter::ArrangeChildren( const TSharedRef< ILayoutBlock >& Block, const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const 
{
	//No Widgets
}

FChildren* FSlateSimpleRunHighlighter::GetChildren()
{
	return &NoChildrenInstance;
}

int32 FSlateSimpleRunHighlighter::OnPaint( const FTextLayout::FLineView& Line, const TSharedRef< ISlateRun >& Run, const TSharedRef< ILayoutBlock >& Block, const FTextBlockStyle& DefaultStyle, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const 
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

	FLinearColor InvertedForeground = FLinearColor::White - InWidgetStyle.GetForegroundColor();
	InvertedForeground.A = InWidgetStyle.GetForegroundColor().A;

	FWidgetStyle WidgetStyle( InWidgetStyle );
	WidgetStyle.SetForegroundColor( InvertedForeground );

	return Run->OnPaint( Line, Block, DefaultStyle, AllottedGeometry, MyClippingRect, OutDrawElements, LayerId, WidgetStyle, bParentEnabled );
}

TSharedRef< FSlateSimpleRunHighlighter > FSlateSimpleRunHighlighter::Create()
{
	return MakeShareable( new FSlateSimpleRunHighlighter() );
}

