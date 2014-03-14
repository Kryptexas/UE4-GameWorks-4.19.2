// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


/**
 * Slate tool tip widget
 */
class SLATE_API SToolTip : public SBorder
{

public:

	SLATE_BEGIN_ARGS( SToolTip )
		: _Text()
		, _Content()
		, _Font( FCoreStyle::Get().GetFontStyle( "ToolTip.Font" ) )
		, _ColorAndOpacity( FSlateColor::UseForeground() )
		, _TextMargin( FMargin( 8.0f ) )
		, _BorderImage( FCoreStyle::Get().GetBrush( "ToolTip.Background" ) )
		, _IsInteractive( false )
		{}

		/** The text displayed in this tool tip */
		SLATE_TEXT_ATTRIBUTE( Text )

		/** Arbitrary content to be displayed in the tool tip; overrides any text that may be set. */
		SLATE_DEFAULT_SLOT( FArguments, Content )

		/** The font to use for this tool tip */
		SLATE_ATTRIBUTE( FSlateFontInfo, Font )
		
		/** Font color and opacity */
		SLATE_ATTRIBUTE( FSlateColor, ColorAndOpacity )
		
		/** Margin between the tool tip border and the text content */
		SLATE_ATTRIBUTE( FMargin, TextMargin )

		/** The background/border image to display */
		SLATE_ATTRIBUTE( const FSlateBrush*, BorderImage)

		/** Whether the tooltip should be considered interactive */
		SLATE_ATTRIBUTE( bool, IsInteractive )

	SLATE_END_ARGS()

	/** @return True if the tool tip has no content to display at this particular moment (remember, attribute callbacks can change this at any time!) */
	bool IsEmpty() const;

	/** @return True is the tool tip will remain in place once opened and allow the user to interact with it */
	bool IsInteractive() const;

	/**
	 * Construct this widget
	 *
	 * @param	InArgs	The declaration data for this widget
	 */
	void Construct( const FArguments& InArgs );


private:

	/** Text block widget */
	TAttribute< FString > TextContent;

	/** Content widget */
	TWeakPtr< SWidget > WidgetContent;

	/** Whether the tooltip should be considered interactive */
	TAttribute< bool > bIsInteractive;
};
