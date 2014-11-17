// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

class SLATE_API FSlateImageRun : public ISlateRun, public TSharedFromThis< FSlateImageRun >
{
public:

	static TSharedRef< FSlateImageRun > Create( const TSharedRef< const FString >& InText, const FSlateBrush* InImage, int16 InBaseline );
	static TSharedRef< FSlateImageRun > Create( const TSharedRef< const FString >& InText, const FSlateBrush* InImage, int16 InBaseline, const FTextRange& InRange );

public:

	virtual ~FSlateImageRun() {}

	virtual FTextRange GetTextRange() const OVERRIDE;
	virtual int16 GetBaseLine( float Scale ) const OVERRIDE;
	virtual int16 GetMaxHeight( float Scale ) const OVERRIDE;

	virtual FVector2D Measure( int32 StartIndex, int32 EndIndex, float Scale ) const OVERRIDE;

	virtual uint8 GetKerning( int32 CurrentIndex, float Scale ) const OVERRIDE;


	virtual TSharedRef< ILayoutBlock > CreateBlock( int32 StartIndex, int32 EndIndex, FVector2D Size, const TSharedPtr< IRunHighlighter >& Highlighter ) OVERRIDE;

	virtual int32 OnPaint( const FTextLayout::FLineView& Line, const TSharedRef< ILayoutBlock >& Block, const FTextBlockStyle& DefaultStyle, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const OVERRIDE;

	virtual FChildren* GetChildren() OVERRIDE;

	virtual void ArrangeChildren( const TSharedRef< ILayoutBlock >& Block, const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const OVERRIDE;

	virtual void BeginLayout() OVERRIDE {}
	virtual void EndLayout() OVERRIDE {}

private:

	FSlateImageRun( const TSharedRef< const FString >& InText, const FSlateBrush* InImage, int16 InBaseline );

	FSlateImageRun( const TSharedRef< const FString >& InText, const FSlateBrush* InImage, int16 InBaseline, const FTextRange& InRange );

private:

	TSharedRef< const FString > Text;
	FTextRange Range;
	const FSlateBrush* Image;
	int16 Baseline;

	static FNoChildren NoChildrenInstance;
};