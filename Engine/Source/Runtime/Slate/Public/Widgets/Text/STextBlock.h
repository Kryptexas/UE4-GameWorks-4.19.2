// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
 
#pragma once

#include "WordWrapper.h"

namespace ETextRole
{
	enum Type
	{
		Custom,
		ButtonText,
		ComboText
	};
}


/**
 * A simple static text widget
 */
class SLATE_API STextBlock : public SLeafWidget
{

public:

	SLATE_BEGIN_ARGS( STextBlock )
		: _Text()
		, _TextStyle( &FCoreStyle::Get().GetWidgetStyle<FTextBlockStyle>( "NormalText" ) )
		, _Font()
		, _ColorAndOpacity()
		, _ShadowOffset()
		, _ShadowColorAndOpacity()
		, _HighlightColor()
		, _HighlightShape()
		, _HighlightText()
		, _WrapTextAt(0.0f)
		, _AutoWrapText(false)
		{}

		/** The text displayed in this text block */
		SLATE_TEXT_ATTRIBUTE( Text )

		/** Pointer to a style of the text block, which dictates the font, color, and shadow options. */
		SLATE_STYLE_ARGUMENT( FTextBlockStyle, TextStyle )

		/** Sets the font used to draw the text */
		SLATE_ATTRIBUTE( FSlateFontInfo, Font )

		/** Text color and opacity */
		SLATE_ATTRIBUTE( FSlateColor, ColorAndOpacity )

		/** Drop shadow offset in pixels */
		SLATE_ATTRIBUTE( FVector2D, ShadowOffset )

		/** Shadow color and opacity */
		SLATE_ATTRIBUTE( FLinearColor, ShadowColorAndOpacity )

		/** The color used to highlight the specified text */
		SLATE_ATTRIBUTE( FLinearColor, HighlightColor )

		/** The brush used to highlight the specified text*/
		SLATE_ATTRIBUTE( const FSlateBrush*, HighlightShape )

		/** Highlight this text in the text block */
		SLATE_ATTRIBUTE( FText, HighlightText )

		/** Whether text wraps onto a new line when it's length exceeds this width; if this value is zero or negative, no wrapping occurs. */
		SLATE_ATTRIBUTE( float, WrapTextAt )

		/** Whether to wrap text automatically based on the widget's computed horizontal space.  IMPORTANT: Using automatic wrapping can result
		    in visual artifacts, as the the wrapped size will computed be at least one frame late!  Consider using WrapTextAt instead.  The initial 
			desired size will not be clamped.  This works best in cases where the text block's size is not affecting other widget's layout. */
		SLATE_ATTRIBUTE( bool, AutoWrapText )

		/** Called when this text is double clicked */
		SLATE_EVENT( FOnClicked, OnDoubleClicked )

	SLATE_END_ARGS()

	/**
	 * Construct this widget
	 *
	 * @param	InArgs	The declaration data for this widget
	 */
	void Construct( const FArguments& InArgs );

	/**
	 * Gets the text assigned to this text block
	 *
	 * @return	This text block's text string
	 */
	const FString& GetText() const
	{
		return Text.Get();
	}


	/**
	 * Sets the text for this text block
	 *
	 * @param	InText	The new text to display
	 */
	void SetText( const TAttribute< FString >& InText )
	{
		Text = InText;
		bRequestCache = true;
	}

	void SetText( const FString& InText )
	{
		Text = InText;
		bRequestCache = true;
	}

	static FString PassThroughAttribute( TAttribute< FText > TextAttribute )
	{
		return TextAttribute.Get( FText::GetEmpty() ).ToString();
	}

	/**
	 * Sets the text for this text block
	 *
	 * @param	InText	The new text to display
	 */
	void SetText( const TAttribute< FText >& InText );

	void SetText( const FText& InText )
	{
		Text = InText.ToString();
		bRequestCache = true;
	}

	/**
	 * Sets the font used to draw the text
	 *
	 * @param	InFont	The new font to use
	 */
	void SetFont( const TAttribute< FSlateFontInfo >& InFont )
	{
		Font = InFont;
		bRequestCache = true;
	}

	/** Set the color and opacity of the text */
	void SetForegroundColor( const TAttribute<FSlateColor>& InSlateColor );

	// SWidget interface
	virtual int32 OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const OVERRIDE;
	virtual FReply OnMouseButtonDoubleClick( const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent ) OVERRIDE;
	virtual void CacheDesiredSize() OVERRIDE;
	virtual FVector2D ComputeDesiredSize() const OVERRIDE;
	// End of SWidget interface

private:
	/** Caches string size to avoid remeasuring if possible */
	void CacheStringSizeIfNeeded();

private:
	/** The text displayed in this text block */
	TAttribute< FString > Text;

	/** Sets the font used to draw the text */
	TAttribute< FSlateFontInfo > Font;

	/** Text color and opacity */
	TAttribute<FSlateColor> ForegroundColor;

	/** Drop shadow offset in pixels */
	TAttribute< FVector2D > ShadowOffset;

	/** Shadow color and opacity */
	TAttribute<FLinearColor> ShadowColorAndOpacity;

	TAttribute<FLinearColor> HighlightColor;

	TAttribute< const FSlateBrush* > HighlightShape;

	/** Highlight this text in the textblock */
	TAttribute<FText> HighlightText;

	/** Whether text wraps onto a new line when it's length exceeds this width; if this value is zero or negative, no wrapping occurs. */
	TAttribute<float> WrapTextAt;

	/** True if we're wrapping text automatically based on the computed horizontal space for this widget */
	TAttribute<bool> AutoWrapText;

	/** The delegate to execute when this text is double clicked */
	FOnClicked OnDoubleClicked;

	/** Cached Font that this text is using. This is used when determining if the cached string size should be updated */
	FSlateFontInfo CachedFont;

	/** Cached wrap width that this text is using. This is used when determining if the cached string size should be updated */
	float CachedWrapTextWidth;

	/** Cached auto-wrap width that this text is using. This is used when determining if the cached string size should be updated */
	mutable float CachedAutoWrapTextWidth;

	/** Text that was used to generate CachedWrappedString */
	FString CachedOriginalString;

	/** Cached wrapped text. This gets cached once in tick so it does not have to be re-generated at paint time every frame */
	FString CachedWrappedString;

	/** Line break data for the original string -> cached string */
	FWordWrapper::FWrappedLineData CachedWrappedLineData;

	/** If a request to cache the string size occurred and we should recache the text */
	mutable bool bRequestCache;
};
