// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

class SLATE_API FSlateSimpleRunHighlighter : public ISlateRunHighlighter
{
public:

	static TSharedRef< FSlateSimpleRunHighlighter > Create();

	virtual ~FSlateSimpleRunHighlighter() {}

	virtual int32 OnPaint( const FTextLayout::FLineView& Line, const TSharedRef< ISlateRun >& Run, const TSharedRef< ILayoutBlock >& Block, const FTextBlockStyle& DefaultStyle, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const override;

	virtual FChildren* GetChildren() override;

	virtual void OnArrangeChildren( const TSharedRef< ILayoutBlock >& Block, const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const override;

private:

	FSlateSimpleRunHighlighter();

private:

	static FNoChildren NoChildrenInstance;
};