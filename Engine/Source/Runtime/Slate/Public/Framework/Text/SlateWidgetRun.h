// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

class SLATE_API FSlateWidgetRun : public ISlateRun, public TSharedFromThis< FSlateWidgetRun >
{
public:

	struct FWidgetRunInfo
	{
		TSharedRef< SWidget > Widget;
		int16 Baseline;
		TOptional< FVector2D > Size;

		FWidgetRunInfo( const TSharedRef< SWidget >& InWidget, int16 InBaseline, TOptional< FVector2D > InSize = TOptional< FVector2D >() )
			: Widget( InWidget )
			, Baseline( InBaseline )
			, Size( InSize )
		{

		}

		FWidgetRunInfo( const FWidgetRunInfo& Other )
			: Widget( Other.Widget )
			, Baseline( Other.Baseline )
			, Size( Other.Size )
		{

		}
	};

	static TSharedRef< FSlateWidgetRun > Create( const TSharedRef< const FString >& InText, const FWidgetRunInfo& InWidgetInfo );

	static TSharedRef< FSlateWidgetRun > Create( const TSharedRef< const FString >& InText, const FWidgetRunInfo& InWidgetInfo, const FTextRange& InRange );

	virtual ~FSlateWidgetRun() {}

	virtual FTextRange GetTextRange() const OVERRIDE;
	virtual void SetTextRange( const FTextRange& Value ) OVERRIDE;

	virtual int16 GetBaseLine( float Scale ) const OVERRIDE;
	virtual int16 GetMaxHeight( float Scale ) const OVERRIDE;

	virtual FVector2D Measure( int32 StartIndex, int32 EndIndex, float Scale ) const OVERRIDE;
	virtual int8 GetKerning(int32 CurrentIndex, float Scale) const OVERRIDE;

	virtual TSharedRef< ILayoutBlock > CreateBlock( int32 StartIndex, int32 EndIndex, FVector2D Size, const TSharedPtr< IRunHighlighter >& Highlighter ) OVERRIDE;

	virtual int32 OnPaint( const FTextLayout::FLineView& Line, const TSharedRef< ILayoutBlock >& Block, const FTextBlockStyle& DefaultStyle, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const OVERRIDE;

	virtual FChildren* GetChildren() OVERRIDE;

	virtual void ArrangeChildren( const TSharedRef< ILayoutBlock >& Block, const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const;

	virtual int32 GetTextIndexAt( const TSharedRef< ILayoutBlock >& Block, const FVector2D& Location, float Scale ) const OVERRIDE;

	virtual FVector2D GetLocationAt( const TSharedRef< ILayoutBlock >& Block, int32 Offset, float Scale ) const OVERRIDE;

	virtual void BeginLayout() OVERRIDE {}
	virtual void EndLayout() OVERRIDE {}

	virtual void Move(const TSharedRef<FString>& NewText, const FTextRange& NewRange) OVERRIDE;
	virtual TSharedRef<IRun> Clone() const OVERRIDE;

	virtual void AppendText(FString& Text) const OVERRIDE;
	virtual void AppendText(FString& AppendToText, const FTextRange& PartialRange) const OVERRIDE;

private:

	FSlateWidgetRun( const TSharedRef< const FString >& InText, const FWidgetRunInfo& InWidgetInfo );

	FSlateWidgetRun( const TSharedRef< const FString >& InText, const FWidgetRunInfo& InWidgetInfo, const FTextRange& InRange );

	FSlateWidgetRun( const FSlateWidgetRun& Run );

private:

	TSharedRef< const FString > Text;
	FTextRange Range;
	FWidgetRunInfo Info;
	TSlotlessChildren< SWidget > Children;
};