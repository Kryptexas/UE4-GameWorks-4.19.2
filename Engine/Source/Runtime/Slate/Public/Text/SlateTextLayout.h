// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

class SLATE_API FSlateTextLayout : public FTextLayout
{
public:

	static TSharedRef< FSlateTextLayout > Create();

	FChildren* GetChildren();

	void ArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const;

	int32 OnPaint( const FTextBlockStyle& DefaultTextStyle, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const;

	//void AddLine( const TSharedRef< FString >& Text, const FTextBlockStyle& TextStyle )
	//{
	//	FLineModel LineModel( Text );
	//	LineModel.Runs.Add( FLineModel::FRun( FSlateTextRun::Create( LineModel.Text, TextStyle ) ) );
	//	LineModels.Add( LineModel );
	//}

	virtual void EndLayout() OVERRIDE;


protected:

	FSlateTextLayout();

	void AggregateChildren();

private:

	TSlotlessChildren< SWidget > Children;

	friend class FSlateTextLayoutFactory;
};