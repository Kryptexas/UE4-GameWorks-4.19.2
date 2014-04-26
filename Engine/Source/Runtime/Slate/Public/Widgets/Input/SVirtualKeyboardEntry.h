// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

// @todo - hook up keyboard types
enum EKeyboardType
{
	Keyboard_Default,
	Keyboard_Number,
	Keyboard_Web,
	Keyboard_Email,
	Keyboard_Password,
};

class SLATE_API SVirtualKeyboardEntry : public SLeafWidget
{

public:

	SLATE_BEGIN_ARGS( SVirtualKeyboardEntry )
		: _Text()
		, _HintText()
		, _Font( FCoreStyle::Get().GetFontStyle("NormalFont") )
		, _ColorAndOpacity( FSlateColor::UseForeground() )
		, _IsReadOnly( false )
		, _ClearKeyboardFocusOnCommit( true )
		, _MinDesiredWidth( 0.0f )
		, _KeyboardType ( EKeyboardType::Keyboard_Default )
		{}

		/** Sets the text content for this editable text widget */
		SLATE_ATTRIBUTE( FText, Text )

		/** The text that appears when there is nothing typed into the search box */
		SLATE_ATTRIBUTE( FText, HintText )

		/** Sets the font used to draw the text */
		SLATE_ATTRIBUTE( FSlateFontInfo, Font )

		/** Text color and opacity */
		SLATE_ATTRIBUTE( FSlateColor, ColorAndOpacity )

		/** Sets whether this text box can actually be modified interactively by the user */
		SLATE_ATTRIBUTE( bool, IsReadOnly )

		/** Whether to clear keyboard focus when pressing enter to commit changes */
		SLATE_ATTRIBUTE( bool, ClearKeyboardFocusOnCommit )

		/** Called whenever the text is changed interactively by the user */
		SLATE_EVENT( FOnTextChanged, OnTextChanged )

		/** Called whenever the text is committed.  This happens when the user presses enter or the text box loses focus. */
		SLATE_EVENT( FOnTextCommitted, OnTextCommitted )

		/** Minimum width that a text block should be */
		SLATE_ATTRIBUTE( float, MinDesiredWidth )

		/** Sets the text content for this editable text widget */
		SLATE_ATTRIBUTE( EKeyboardType, KeyboardType )

		SLATE_END_ARGS()


	/** Constructor */
	SVirtualKeyboardEntry();
	
	/**
	 * Construct this widget
	 *
	 * @param	InArgs	The declaration data for this widget
	 */
	void Construct( const FArguments& InArgs );

	/**
	 * Returns the text.
	 *
	 * @return  Text
	 */
	const FText& GetText() const
	{
		return Text.Get();
	}

	/**
	 * Sets the text currently being edited 
	 *
	 * @param  InNewText  The new text
	 */
	void SetText( const TAttribute< FText >& InNewText );

	/**
	 * Returns the hint text.
	 *
	 * @return  HintText
	 */
	const FText& GetHintText() const
	{
		return HintText.Get();
	}

	/**
	 * Returns the keyboard type.
	 *
	 * @return  KeyboardType
	 */
	EKeyboardType GetKeyboardType() const
	{
		return KeyboardType.Get();
	}

	/**
	 * Restores the text to the original state
	 */
	void RestoreOriginalText();

	/**
	 *	Returns whether the current text varies from the original
	 */
	bool HasTextChangedFromOriginal() const;

	/**
	 * Sets the font used to draw the text
	 *
	 * @param  InNewFont	The new font to use
	 */
	void SetFont( const TAttribute< FSlateFontInfo >& InNewFont );

	/**
	 * Can the user edit the text?
	 */
	bool GetIsReadOnly() const;


protected:

	/**
	 * Gets the height of the largest character in the font
	 *
	 * @return  The fonts height
	 */
	virtual float GetFontHeight() const;

	// SWidget interface
	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) OVERRIDE;
	virtual FVector2D ComputeDesiredSize() const OVERRIDE;
	virtual int32 OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const OVERRIDE;
	virtual bool SupportsKeyboardFocus() const OVERRIDE;
	virtual FReply OnKeyboardFocusReceived( const FGeometry& MyGeometry, const FKeyboardFocusEvent& InKeyboardFocusEvent ) OVERRIDE;
	virtual void OnKeyboardFocusLost( const FKeyboardFocusEvent& InKeyboardFocusEvent ) OVERRIDE;
	// End of SWidget interface

private:

	/**
	 * @return  the string that needs to be rendered
	 */
	FString GetStringToRender() const;

private:

	/** The text content for this editable text widget */
	TAttribute< FText > Text;

	TAttribute< FText > HintText;

	/** The font used to draw the text */
	TAttribute< FSlateFontInfo > Font;

	/** Text color and opacity */
	TAttribute<FSlateColor> ColorAndOpacity;

	/** Sets whether this text box can actually be modified interactively by the user */
	TAttribute< bool > IsReadOnly;

	/** Whether to clear keyboard focus when pressing enter to commit changes */
	TAttribute< bool > ClearKeyboardFocusOnCommit;

	/** Called whenever the text is changed interactively by the user */
	FOnTextChanged OnTextChanged;

	/** Called whenever the text is committed.  This happens when the user presses enter or the text box loses focus. */
	FOnTextCommitted OnTextCommitted;

	/** Text string currently being edited */
	FText EditedText;

	/** Original text prior to user edits.  This is used to restore the text if the user uses the escape key. */
	FText OriginalText;

	/** Current scrolled position */
	FScrollHelper ScrollHelper;

	/** True if the last mouse down caused us to receive keyboard focus */
	bool bWasFocusedByLastMouseDown;

	/** True if we're currently (potentially) changing the text string */
	bool bIsChangingText;

	/** Prevents the editabletext from being smaller than desired in certain cases (e.g. when it is empty) */
	TAttribute<float> MinDesiredWidth;

	TAttribute<EKeyboardType> KeyboardType;

	bool bNeedsUpdate;

};
