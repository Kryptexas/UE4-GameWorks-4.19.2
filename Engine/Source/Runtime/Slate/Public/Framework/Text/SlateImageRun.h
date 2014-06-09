// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

class SLATE_API FSlateImageRun : public ISlateRun, public TSharedFromThis< FSlateImageRun >
{
public:

	static TSharedRef< FSlateImageRun > Create( const TSharedRef< const FString >& InText, const FSlateBrush* InImage, int16 InBaseline );
	static TSharedRef< FSlateImageRun > Create( const TSharedRef< const FString >& InText, const FSlateBrush* InImage, int16 InBaseline, const FTextRange& InRange );

public:

	virtual ~FSlateImageRun() {}

	virtual FTextRange GetTextRange() const override;
	virtual void SetTextRange( const FTextRange& Value ) override;

	virtual int16 GetBaseLine( float Scale ) const override;
	virtual int16 GetMaxHeight( float Scale ) const override;

	virtual FVector2D Measure( int32 StartIndex, int32 EndIndex, float Scale ) const override;

	virtual int8 GetKerning(int32 CurrentIndex, float Scale) const override;


	virtual TSharedRef< ILayoutBlock > CreateBlock( int32 StartIndex, int32 EndIndex, FVector2D Size, const TSharedPtr< IRunHighlighter >& Highlighter ) override;

	virtual int32 OnPaint( const FTextLayout::FLineView& Line, const TSharedRef< ILayoutBlock >& Block, const FTextBlockStyle& DefaultStyle, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const override;

	virtual FChildren* GetChildren() override;

	virtual void ArrangeChildren( const TSharedRef< ILayoutBlock >& Block, const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const override;

	virtual int32 GetTextIndexAt( const TSharedRef< ILayoutBlock >& Block, const FVector2D& Location, float Scale ) const override;

	virtual FVector2D GetLocationAt( const TSharedRef< ILayoutBlock >& Block, int32 Offset, float Scale ) const override;

	virtual void BeginLayout() override {}
	virtual void EndLayout() override {}

	virtual void Move(const TSharedRef<FString>& NewText, const FTextRange& NewRange) override;
	virtual TSharedRef<IRun> Clone() const override;

	virtual void AppendText(FString& Text) const override;
	virtual void AppendText(FString& AppendToText, const FTextRange& PartialRange) const override;

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