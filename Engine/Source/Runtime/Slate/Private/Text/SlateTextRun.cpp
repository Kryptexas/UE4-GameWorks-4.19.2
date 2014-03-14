// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"
#include "SlateTextRun.h"

FNoChildren FSlateTextRun::NoChildrenInstance;

TSharedRef< FSlateTextRun > FSlateTextRun::Create( const TSharedRef< const FString >& InText, const FTextBlockStyle& Style )
{
	return MakeShareable( new FSlateTextRun( InText, Style ) );
}

TSharedRef< FSlateTextRun > FSlateTextRun::Create( const TSharedRef< const FString >& InText, const FTextBlockStyle& Style, const FTextRange& InRange )
{
	return MakeShareable( new FSlateTextRun( InText, Style, InRange ) );
}

FTextRange FSlateTextRun::GetTextRange() const 
{
	return Range;
}

int16 FSlateTextRun::GetBaseLine( float Scale ) const 
{
	const TSharedRef< FSlateFontMeasure > FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	return FontMeasure->GetBaseline( Style.Font, Scale );
}

int16 FSlateTextRun::GetMaxHeight( float Scale ) const 
{
	const TSharedRef< FSlateFontMeasure > FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	return FontMeasure->GetMaxCharacterHeight( Style.Font, Scale );
}

FVector2D FSlateTextRun::Measure( int32 BeginIndex, int32 EndIndex, float Scale ) const 
{
	if ( EndIndex - BeginIndex == 0 )
	{
		return FVector2D( 0, GetMaxHeight( Scale ) );
	}

	const TSharedRef< FSlateFontMeasure > FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	return FontMeasure->Measure( **Text, BeginIndex, EndIndex, Style.Font, true, Scale );
}

uint8 FSlateTextRun::GetKerning( int32 CurrentIndex, float Scale ) const 
{
	const int32 PreviousIndex = CurrentIndex - 1;
	if ( PreviousIndex < 0 )
	{
		return 0;
	}

	const TSharedRef< FSlateFontMeasure > FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	return FontMeasure->GetKerning( Style.Font, Scale, (*Text)[ PreviousIndex ], (*Text)[ CurrentIndex ] );
}

TSharedRef< ILayoutBlock > FSlateTextRun::CreateBlock( int32 BeginIndex, int32 EndIndex, FVector2D Size, const TSharedPtr< IRunHighlighter >& Highlighter )
{
	return FDefaultLayoutBlock::Create( SharedThis( this ), FTextRange( BeginIndex, EndIndex ), Size, Highlighter );
}

int32 FSlateTextRun::OnPaint( const FTextLayout::FLineView& Line, const TSharedRef< ILayoutBlock >& Block, const FTextBlockStyle& DefaultStyle, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const 
{
	const FSlateRect ClippingRect = AllottedGeometry.GetClippingRect().IntersectionWith(MyClippingRect);
	const ESlateDrawEffect::Type DrawEffects = bParentEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;

	const bool ShouldDropShadow = Style.ShadowOffset.Size() > 0;
	const FVector2D BlockLocationOffset = Block->GetLocationOffset();
	const FTextRange BlockRange = Block->GetTextRange();

	// Draw the optional shadow
	if ( ShouldDropShadow )
	{
		FSlateDrawElement::MakeText(
			OutDrawElements,
			++LayerId,
			FPaintGeometry( AllottedGeometry.AbsolutePosition + Block->GetLocationOffset() + Style.ShadowOffset, Block->GetSize(), AllottedGeometry.Scale ),
			Text.Get(),
			BlockRange.BeginIndex,
			BlockRange.EndIndex,
			Style.Font,
			ClippingRect,
			DrawEffects,
			InWidgetStyle.GetColorAndOpacityTint() * Style.ShadowColorAndOpacity
			);
	}

	// Draw the text itself
	FSlateDrawElement::MakeText(
		OutDrawElements,
		++LayerId,
		FPaintGeometry( AllottedGeometry.AbsolutePosition + Block->GetLocationOffset(), Block->GetSize(), AllottedGeometry.Scale ),
		Text.Get(),
		BlockRange.BeginIndex,
		BlockRange.EndIndex,
		Style.Font,
		ClippingRect,
		DrawEffects,
		InWidgetStyle.GetColorAndOpacityTint() * Style.ColorAndOpacity.GetColor(InWidgetStyle)
		);

	return LayerId;
}

FChildren* FSlateTextRun::GetChildren()
{
	return &NoChildrenInstance;
}

void FSlateTextRun::ArrangeChildren( const TSharedRef< ILayoutBlock >& Block, const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const 
{
	// no widgets
}

FSlateTextRun::FSlateTextRun( const TSharedRef< const FString >& InText, const FTextBlockStyle& InStyle ) : Text( InText )
	, Style( InStyle )
	, Range( 0, Text->Len() )
#if TEXT_LAYOUT_DEBUG
	, DebugSlice( FString( Text->Len(), **Text ) )
#endif
{

}

FSlateTextRun::FSlateTextRun( const TSharedRef< const FString >& InText, const FTextBlockStyle& InStyle, const FTextRange& InRange ) : Text( InText )
	, Style( InStyle )
	, Range( InRange )
#if TEXT_LAYOUT_DEBUG
	, DebugSlice( FString( InRange.EndIndex - InRange.BeginIndex, (**Text) + InRange.BeginIndex ) )
#endif
{

}

FSlateTextRun::FSlateTextRun( const FSlateTextRun& Run ) : Text( Run.Text )
	, Style( Run.Style )
	, Range( Run.Range )
#if TEXT_LAYOUT_DEBUG
	, DebugSlice( Run.DebugSlice )
#endif
{

}

