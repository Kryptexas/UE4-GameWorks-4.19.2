// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#if WITH_FANCY_TEXT

class SLATE_API FSlateTextLayout : public FTextLayout
{
public:

	static TSharedRef< FSlateTextLayout > Create();

	FChildren* GetChildren();

	void ArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const;

	int32 OnPaint( const FPaintArgs& Args, const FTextBlockStyle& DefaultTextStyle, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const;

	//void AddLine( const TSharedRef< FString >& Text, const FTextBlockStyle& TextStyle )
	//{
	//	FLineModel LineModel( Text );
	//	LineModel.Runs.Add( FLineModel::FRun( FSlateTextRun::Create( LineModel.Text, TextStyle ) ) );
	//	LineModels.Add( LineModel );
	//}

	virtual void EndLayout() override;


protected:

	FSlateTextLayout();

	int32 OnPaintHighlights(const FPaintArgs& Args, const FTextLayout::FLineView& LineView, const TArray<FLineViewHighlight>& Highlights, const FTextBlockStyle& DefaultTextStyle, const FGeometry& AllottedGeometry, const FSlateRect& ClippingRect, FSlateWindowElementList& OutDrawElements, const int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const;

	void AggregateChildren();

private:

	TSlotlessChildren< SWidget > Children;

	friend class FSlateTextLayoutFactory;
};

#endif //WITH_FANCY_TEXT