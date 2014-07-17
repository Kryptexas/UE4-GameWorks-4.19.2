// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

class ISlateRun;
class ILayoutBlock;

class SLATE_API ISlateRunRenderer : public IRunRenderer
{
public:
	virtual ~ISlateRunRenderer() {}

	virtual int32 OnPaint( const FTextLayout::FLineView& Line, const TSharedRef< ISlateRun >& Run, const TSharedRef< ILayoutBlock >& Block, const FTextBlockStyle& DefaultStyle, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const = 0;
};
