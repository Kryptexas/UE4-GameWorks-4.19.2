// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

class ISlateRun;
class ILayoutBlock;

class SLATE_API ISlateRunHighlighter : public IRunHighlighter
{
public:
	virtual ~ISlateRunHighlighter() {}

	virtual int32 OnPaint( const FTextLayout::FLineView& Line, const TSharedRef< ISlateRun >& Run, const TSharedRef< ILayoutBlock >& Block, const FTextBlockStyle& DefaultStyle, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const = 0;

	virtual FChildren* GetChildren() = 0;

	virtual void ArrangeChildren( const TSharedRef< ILayoutBlock >& Block, const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const = 0;
};