// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"
#include "SlateWidgetRun.h"

TSharedRef< FSlateWidgetRun > FSlateWidgetRun::Create( const TSharedRef< const FString >& InText, const FWidgetRunInfo& InWidgetInfo )
{
	return MakeShareable( new FSlateWidgetRun( InText, InWidgetInfo ) );
}

TSharedRef< FSlateWidgetRun > FSlateWidgetRun::Create( const TSharedRef< const FString >& InText, const FWidgetRunInfo& InWidgetInfo, const FTextRange& InRange )
{
	return MakeShareable( new FSlateWidgetRun( InText, InWidgetInfo, InRange ) );
}

FTextRange FSlateWidgetRun::GetTextRange() const 
{
	return Range;
}

int16 FSlateWidgetRun::GetBaseLine( float Scale ) const 
{
	return Info.Baseline * Scale;
}

int16 FSlateWidgetRun::GetMaxHeight( float Scale ) const 
{
	return Info.Size.Get( Info.Widget->GetDesiredSize() ).Y * Scale;
}

FVector2D FSlateWidgetRun::Measure( int32 StartIndex, int32 EndIndex, float Scale ) const 
{
	return Info.Size.Get( Info.Widget->GetDesiredSize() ) * Scale;
}

uint8 FSlateWidgetRun::GetKerning( int32 CurrentIndex, float Scale ) const 
{
	return 0;
}

TSharedRef< ILayoutBlock > FSlateWidgetRun::CreateBlock( int32 StartIndex, int32 EndIndex, FVector2D Size, const TSharedPtr< IRunHighlighter >& Highlighter )
{
	return FDefaultLayoutBlock::Create( SharedThis( this ), FTextRange( StartIndex, EndIndex ), Size, Highlighter );
}

int32 FSlateWidgetRun::OnPaint( const FTextLayout::FLineView& Line, const TSharedRef< ILayoutBlock >& Block, const FTextBlockStyle& DefaultStyle, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const 
{
	FGeometry WidgetGeometry = AllottedGeometry.MakeChild( Block->GetLocationOffset() * ( 1 / AllottedGeometry.Scale ), Block->GetSize() * ( 1 / AllottedGeometry.Scale ), 1.0f );
	return Info.Widget->OnPaint( WidgetGeometry, MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled );
}

FChildren* FSlateWidgetRun::GetChildren()
{
	return &Children;
}

void FSlateWidgetRun::ArrangeChildren( const TSharedRef< ILayoutBlock >& Block, const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const
{
	ArrangedChildren.AddWidget( AllottedGeometry.MakeChild( Info.Widget, Block->GetLocationOffset() * ( 1 / AllottedGeometry.Scale ), Block->GetSize() * ( 1 / AllottedGeometry.Scale ), 1.0f ) );
}

FSlateWidgetRun::FSlateWidgetRun( const TSharedRef< const FString >& InText, const FWidgetRunInfo& InWidgetInfo ) 
	: Text( InText )
	, Range( 0, Text->Len() )
	, Info( InWidgetInfo )
	, Children()
{
	Info.Widget->SlatePrepass();
	Children.Add( Info.Widget );
}

FSlateWidgetRun::FSlateWidgetRun( const TSharedRef< const FString >& InText, const FWidgetRunInfo& InWidgetInfo, const FTextRange& InRange ) 
	: Text( InText )
	, Range( InRange )
	, Info( InWidgetInfo )
	, Children()
{
	Info.Widget->SlatePrepass();
	Children.Add( Info.Widget );
}

FSlateWidgetRun::FSlateWidgetRun( const FSlateWidgetRun& Run ) 
	: Text( Run.Text )
	, Range( Run.Range )
	, Info( Run.Info )
	, Children( Run.Children )
{

}

