// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"
#include "SlateImageRun.h"

FNoChildren FSlateImageRun::NoChildrenInstance;

TSharedRef< FSlateImageRun > FSlateImageRun::Create( const TSharedRef< const FString >& InText, const FSlateBrush* InImage, int16 InBaseline )
{
	if ( InImage == NULL )
	{
		InImage = FStyleDefaults::GetNoBrush();
	}

	return MakeShareable( new FSlateImageRun( InText, InImage, InBaseline ) );
}

TSharedRef< FSlateImageRun > FSlateImageRun::Create( const TSharedRef< const FString >& InText, const FSlateBrush* InImage, int16 InBaseline, const FTextRange& InRange )
{
	if ( InImage == NULL )
	{
		InImage = FStyleDefaults::GetNoBrush();
	}

	return MakeShareable( new FSlateImageRun( InText, InImage, InBaseline, InRange ) );
}

FSlateImageRun::FSlateImageRun( const TSharedRef< const FString >& InText, const FSlateBrush* InImage, int16 InBaseline, const FTextRange& InRange ) : Text( InText )
	, Range( InRange )
	, Image( InImage )
	, Baseline( InBaseline )
{
	check( Image != NULL );
}

FSlateImageRun::FSlateImageRun( const TSharedRef< const FString >& InText, const FSlateBrush* InImage, int16 InBaseline ) : Text( InText )
	, Range( 0, Text->Len() )
	, Image( InImage )
	, Baseline( InBaseline )
{
	check( Image != NULL );
}

FChildren* FSlateImageRun::GetChildren() 
{
	return &NoChildrenInstance;
}

void FSlateImageRun::ArrangeChildren( const TSharedRef< ILayoutBlock >& Block, const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const 
{
	// no widgets
}

int32 FSlateImageRun::OnPaint( const FTextLayout::FLineView& Line, const TSharedRef< ILayoutBlock >& Block, const FTextBlockStyle& DefaultStyle, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const 
{
	if ( Image->DrawAs != ESlateBrushDrawType::NoDrawType )
	{
		const FColor FinalColorAndOpacity( InWidgetStyle.GetColorAndOpacityTint() * Image->GetTint( InWidgetStyle ) );
		const uint32 DrawEffects = bParentEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
		FSlateDrawElement::MakeBox(OutDrawElements, ++LayerId, FPaintGeometry( AllottedGeometry.AbsolutePosition + Block->GetLocationOffset(), Block->GetSize(), AllottedGeometry.Scale ), Image, MyClippingRect, DrawEffects, FinalColorAndOpacity );
	}

	return LayerId;
}

TSharedRef< ILayoutBlock > FSlateImageRun::CreateBlock( int32 BeginIndex, int32 EndIndex, FVector2D Size, const TSharedPtr< IRunHighlighter >& Highlighter )
{
	return FDefaultLayoutBlock::Create( SharedThis( this ), FTextRange( BeginIndex, EndIndex ), Size, Highlighter );
}

uint8 FSlateImageRun::GetKerning( int32 CurrentIndex, float Scale ) const 
{
	return 0;
}

FVector2D FSlateImageRun::Measure( int32 BeginIndex, int32 EndIndex, float Scale ) const 
{
	check( BeginIndex == Range.BeginIndex && EndIndex == Range.EndIndex );
	return Image->ImageSize * Scale;
}

int16 FSlateImageRun::GetMaxHeight( float Scale ) const 
{
	return Image->ImageSize.Y * Scale;
}

int16 FSlateImageRun::GetBaseLine( float Scale ) const 
{
	return Baseline * Scale;
}

FTextRange FSlateImageRun::GetTextRange() const 
{
	return Range;
}

