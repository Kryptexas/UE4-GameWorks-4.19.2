// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"
#include "SlateHyperlinkRun.h"

TSharedRef< FSlateHyperlinkRun > FSlateHyperlinkRun::Create( const TSharedRef< const FString >& InText, const FHyperlinkStyle& InStyle, const TMap<FString,FString>& Metadata, FOnClick NavigateDelegate )
{
	return MakeShareable( new FSlateHyperlinkRun( InText, InStyle, Metadata, NavigateDelegate ) );
}

TSharedRef< FSlateHyperlinkRun > FSlateHyperlinkRun::Create( const TSharedRef< const FString >& InText, const FHyperlinkStyle& InStyle, const TMap<FString,FString>& Metadata, FOnClick NavigateDelegate, const FTextRange& InRange )
{
	return MakeShareable( new FSlateHyperlinkRun( InText, InStyle, Metadata, NavigateDelegate, InRange ) );
}

FTextRange FSlateHyperlinkRun::GetTextRange() const 
{
	return Range;
}

int16 FSlateHyperlinkRun::GetBaseLine( float Scale ) const 
{
	const TSharedRef< FSlateFontMeasure > FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	return FontMeasure->GetBaseline( Style.TextStyle.Font, Scale );
}

int16 FSlateHyperlinkRun::GetMaxHeight( float Scale ) const 
{
	const TSharedRef< FSlateFontMeasure > FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	return FontMeasure->GetMaxCharacterHeight( Style.TextStyle.Font, Scale );
}

FVector2D FSlateHyperlinkRun::Measure( int32 StartIndex, int32 EndIndex, float Scale ) const 
{
	if ( EndIndex - StartIndex == 0 )
	{
		return FVector2D( 0, GetMaxHeight( Scale ) );
	}

	const TSharedRef< FSlateFontMeasure > FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	return FontMeasure->Measure( **Text, StartIndex, EndIndex, Style.TextStyle.Font, true, Scale );
}

uint8 FSlateHyperlinkRun::GetKerning( int32 CurrentIndex, float Scale ) const 
{
	return 0;
}

TSharedRef< ILayoutBlock > FSlateHyperlinkRun::CreateBlock( int32 StartIndex, int32 EndIndex, FVector2D Size, const TSharedPtr< IRunHighlighter >& Highlighter )
{
	TSharedRef< SWidget > Widget = SNew( SRichTextHyperlink, ViewModel )
		.Style( &Style )
		.Text( FText::FromString( FString( EndIndex - StartIndex, **Text + StartIndex ) ) )
		.OnNavigate( this, &FSlateHyperlinkRun::OnNavigate );

	Children.Add( Widget );

	return FWidgetLayoutBlock::Create( SharedThis( this ), Widget, FTextRange( StartIndex, EndIndex ), Size, Highlighter );
}

void FSlateHyperlinkRun::OnNavigate()
{
	NavigateDelegate.Execute( Metadata );
}

int32 FSlateHyperlinkRun::OnPaint( const FTextLayout::FLineView& Line, const TSharedRef< ILayoutBlock >& Block, const FTextBlockStyle& DefaultStyle, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const 
{
	const TSharedRef< FWidgetLayoutBlock > WidgetBlock = StaticCastSharedRef< FWidgetLayoutBlock >( Block );

	FGeometry WidgetGeometry = AllottedGeometry.MakeChild( Block->GetLocationOffset() * ( 1 / AllottedGeometry.Scale ), Block->GetSize() * ( 1 / AllottedGeometry.Scale ), 1.0f );
	return WidgetBlock->GetWidget()->OnPaint( WidgetGeometry, MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled );
}

FChildren* FSlateHyperlinkRun::GetChildren()
{
	return &Children;
}

void FSlateHyperlinkRun::ArrangeChildren( const TSharedRef< ILayoutBlock >& Block, const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const 
{
	const TSharedRef< FWidgetLayoutBlock > WidgetBlock = StaticCastSharedRef< FWidgetLayoutBlock >( Block );
	ArrangedChildren.AddWidget( AllottedGeometry.MakeChild( WidgetBlock->GetWidget(), Block->GetLocationOffset() * ( 1 / AllottedGeometry.Scale ), Block->GetSize() * ( 1 / AllottedGeometry.Scale ), 1.0f ) );
}

FSlateHyperlinkRun::FSlateHyperlinkRun( const TSharedRef< const FString >& InText, const FHyperlinkStyle& InStyle, const FMetadata& InMetadata, FOnClick InNavigateDelegate ) 
	: Text( InText )
	, Range( 0, Text->Len() )
	, Style( InStyle )
	, Metadata( InMetadata )
	, NavigateDelegate( InNavigateDelegate )
	, ViewModel( MakeShareable( new FSlateHyperlinkRun::FWidgetViewModel() ) )
	, Children()
{

}

FSlateHyperlinkRun::FSlateHyperlinkRun( const TSharedRef< const FString >& InText, const FHyperlinkStyle& InStyle, const FMetadata& InMetadata, FOnClick InNavigateDelegate, const FTextRange& InRange ) 
	: Text( InText )
	, Range( InRange )
	, Style( InStyle )
	, Metadata( InMetadata )
	, NavigateDelegate( InNavigateDelegate )
	, ViewModel( MakeShareable( new FSlateHyperlinkRun::FWidgetViewModel() ) )
	, Children()
{

}

FSlateHyperlinkRun::FSlateHyperlinkRun( const FSlateHyperlinkRun& Run ) 
	: Text( Run.Text )
	, Range( Run.Range )
	, Style( Run.Style )
	, Metadata( Run.Metadata )
	, NavigateDelegate( Run.NavigateDelegate )
	, ViewModel( MakeShareable( new FSlateHyperlinkRun::FWidgetViewModel() ) )
	, Children()
{

}

