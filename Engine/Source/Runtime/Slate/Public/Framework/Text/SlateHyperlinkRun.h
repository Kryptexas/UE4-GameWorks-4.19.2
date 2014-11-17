// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

class SLATE_API FSlateHyperlinkRun : public ISlateRun, public TSharedFromThis< FSlateHyperlinkRun >
{
public:

	typedef TMap< FString, FString > FMetadata;
	DECLARE_DELEGATE_OneParam( FOnClick, const FMetadata& /*Metadata*/ )

public:

	class FWidgetViewModel
	{
	public:

		bool IsPressed() const { return bIsPressed; }
		bool IsHovered() const { return bIsHovered; }

		void SetIsPressed( bool Value ) { bIsPressed = Value; }
		void SetIsHovered( bool Value ) { bIsHovered = Value; }

	private:

		bool bIsPressed;
		bool bIsHovered;
	};

public:

	static TSharedRef< FSlateHyperlinkRun > Create( const TSharedRef< const FString >& InText, const FHyperlinkStyle& InStyle, const FMetadata& Metadata, FOnClick NavigateDelegate );
																																	 
	static TSharedRef< FSlateHyperlinkRun > Create( const TSharedRef< const FString >& InText, const FHyperlinkStyle& InStyle, const FMetadata& Metadata, FOnClick NavigateDelegate, const FTextRange& InRange );

public:

	virtual ~FSlateHyperlinkRun() {}

	virtual FTextRange GetTextRange() const OVERRIDE;

	virtual int16 GetBaseLine( float Scale ) const OVERRIDE;

	virtual int16 GetMaxHeight( float Scale ) const OVERRIDE;

	virtual FVector2D Measure( int32 StartIndex, int32 EndIndex, float Scale ) const OVERRIDE;

	virtual uint8 GetKerning( int32 CurrentIndex, float Scale ) const OVERRIDE;

	virtual TSharedRef< ILayoutBlock > CreateBlock( int32 StartIndex, int32 EndIndex, FVector2D Size, const TSharedPtr< IRunHighlighter >& Highlighter ) OVERRIDE;

	virtual int32 OnPaint( const FTextLayout::FLineView& Line, const TSharedRef< ILayoutBlock >& Block, const FTextBlockStyle& DefaultStyle, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const OVERRIDE;

	virtual FChildren* GetChildren() OVERRIDE;

	virtual void ArrangeChildren( const TSharedRef< ILayoutBlock >& Block, const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const OVERRIDE;

	virtual void BeginLayout() OVERRIDE { Children.Empty(); }
	virtual void EndLayout() OVERRIDE {}

private:

	FSlateHyperlinkRun( const TSharedRef< const FString >& InText, const FHyperlinkStyle& InStyle, const FMetadata& InMetadata, FOnClick InNavigateDelegate );
																										 
	FSlateHyperlinkRun( const TSharedRef< const FString >& InText, const FHyperlinkStyle& InStyle, const FMetadata& InMetadata, FOnClick InNavigateDelegate, const FTextRange& InRange );

	FSlateHyperlinkRun( const FSlateHyperlinkRun& Run );

private:

	void OnNavigate();

private:

	TSharedRef< const FString > Text;
	FTextRange Range;
	FHyperlinkStyle Style;
	TMap<FString,FString> Metadata;
	FOnClick NavigateDelegate;

	TSharedRef< FWidgetViewModel > ViewModel;
	TSlotlessChildren< SWidget > Children;
};