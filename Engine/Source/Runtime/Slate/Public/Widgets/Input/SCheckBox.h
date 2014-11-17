// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


/** Current state of the check box */
namespace ESlateCheckBoxState
{
	typedef uint8 Type;
	
		/** Unchecked */
	const Type Unchecked = 0;

		/** Checked */
	const Type Checked = 1;

		/** Neither checked nor unchecked */
	const Type Undetermined = 2;
	
};


/** Delegate that is executed when the check box state changes */
DECLARE_DELEGATE_OneParam( FOnCheckStateChanged, ESlateCheckBoxState::Type );


/**
 * Check box Slate control
 */
class SLATE_API SCheckBox : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS( SCheckBox )
		: _Content()
		, _Style( &FCoreStyle::Get().GetWidgetStyle< FCheckBoxStyle >("Checkbox") )
		, _Type()
		, _OnCheckStateChanged()
		, _IsChecked( ESlateCheckBoxState::Unchecked )
		, _HAlign( HAlign_Fill )
		, _Padding()
		, _ClickMethod( EButtonClickMethod::DownAndUp )
		, _ForegroundColor()
		, _BorderBackgroundColor ()
		, _ReadOnly( false )
		, _IsFocusable( true )
		{}

		/** Content to be placed next to the check box, or for a toggle button, the content to be placed inside the button */
		SLATE_DEFAULT_SLOT( FArguments, Content )

		/** The style structure for this checkbox' visual style */
		SLATE_STYLE_ARGUMENT( FCheckBoxStyle, Style )

		/** Type of check box (set by the Style arg but the Style can be overridden with this) */
		SLATE_ARGUMENT( TOptional<ESlateCheckBoxType::Type>, Type )

		/** Called when the checked state has changed */
		SLATE_EVENT( FOnCheckStateChanged, OnCheckStateChanged )

		/** Whether the check box is currently in a checked state */
		SLATE_ATTRIBUTE( ESlateCheckBoxState::Type, IsChecked )

		/** How the content of the toggle button should align within the given space*/
		SLATE_ARGUMENT( EHorizontalAlignment, HAlign )

		/** Spacing between the check box image and its content (set by the Style arg but the Style can be overridden with this) */
		SLATE_ATTRIBUTE( FMargin, Padding )

		/** Sets the rules to use for determining whether the button was clicked.  This is an advanced setting and generally should be left as the default. */
		SLATE_ATTRIBUTE( EButtonClickMethod::Type, ClickMethod )

		/** Foreground color for the checkbox's content and parts (set by the Style arg but the Style can be overridden with this) */
		SLATE_ATTRIBUTE( FSlateColor, ForegroundColor )

		/** The color of the background border (set by the Style arg but the Style can be overridden with this) */
		SLATE_ATTRIBUTE( FSlateColor, BorderBackgroundColor )

		SLATE_ATTRIBUTE( bool, ReadOnly )

		SLATE_ARGUMENT( bool, IsFocusable )
		
		SLATE_EVENT( FOnGetContent, OnGetMenuContent )

		/** The sound to play when the check box is checked */
		SLATE_ARGUMENT( TOptional<FSlateSound>, CheckedSoundOverride )

		/** The sound to play when the check box is unchecked */
		SLATE_ARGUMENT( TOptional<FSlateSound>, UncheckedSoundOverride )

		/** The sound to play when the check box is hovered */
		SLATE_ARGUMENT( TOptional<FSlateSound>, HoveredSoundOverride )

	SLATE_END_ARGS()


	/**
	 * Construct this widget
	 *
	 * @param	InArgs	The declaration data for this widget
	 */
	void Construct( const FArguments& InArgs );

	// SWidget interface
	virtual bool SupportsKeyboardFocus() const OVERRIDE;
	virtual FReply OnKeyUp( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent ) OVERRIDE;
	virtual FReply OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE;
	virtual FReply OnMouseButtonDoubleClick( const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent ) OVERRIDE;
	virtual FReply OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE;
	virtual void OnMouseEnter( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE;
	virtual void OnMouseLeave( const FPointerEvent& MouseEvent ) OVERRIDE;
	// End of SWidget interface

	/**
	 * Returns true if the checkbox is currently checked
	 *
	 * @return	True if checked, otherwise false
	 */
	bool IsChecked() const
	{
		return (IsCheckboxChecked.Get() == ESlateCheckBoxState::Checked);
	}

	/**
	 * Returns true if this button is currently pressed
	 *
	 * @return	True if pressed, otherwise false
	 */
	bool IsPressed() const
	{
		return bIsPressed;
	}

	/**
	 * Toggles the checked state for this check box, fire events as needed
	 */
	void ToggleCheckedState();

protected:

	/**
	 * Gets the check image to display for the current state of the check box
	 * @return	The name of the image to display
	 */
	const FSlateBrush* OnGetCheckImage() const;


protected:

	/** True if this check box is currently in a pressed state */
	bool bIsPressed;

	/** Are we checked */
	TAttribute<ESlateCheckBoxState::Type> IsCheckboxChecked;

	/** Delegate called when the check box changes state */
	FOnCheckStateChanged OnCheckStateChanged;

	/** Image to use when the checkbox is unchecked */
	const FSlateBrush* UncheckedImage;
	/** Image to use when the checkbox is unchecked and hovered*/
	const FSlateBrush* UncheckedHoveredImage;
	/** Image to use when the checkbox is unchecked and pressed*/
	const FSlateBrush* UncheckedPressedImage;
	/** Image to use when the checkbox is checked*/
	const FSlateBrush* CheckedImage;
	/** Image to use when the checkbox is checked and hovered*/
	const FSlateBrush* CheckedHoveredImage;
	/** Image to use when the checkbox is checked and pressed*/
	const FSlateBrush* CheckedPressedImage;
	/** Image to use when the checkbox is in an ambiguous state*/
	const FSlateBrush* UndeterminedImage;
	/** Image to use when the checkbox is in an ambiguous state and hovered*/
	const FSlateBrush* UndeterminedHoveredImage;
	/** Image to use when the checkbox is in an ambiguous state and pressed*/
	const FSlateBrush* UndeterminedPressedImage;

	/** Sets whether a click should be triggered on mouse down, mouse up, or that both a mouse down and up are required. */
	EButtonClickMethod::Type ClickMethod;

	/** When true, this checkbox will not be toggleable */
	bool bReadOnly;

	/** When true, this checkbox will be keyboard focusable. Defaults to true. */
	bool bIsFocusable;

	/** Delegate to execute to get the menu content of this button */
	FOnGetContent OnGetMenuContent;

	/** Play the checked sound */
	void PlayCheckedSound() const;

	/** Play the unchecked sound */
	void PlayUncheckedSound() const;

	/** Play the hovered sound */
	void PlayHoverSound() const;

	/** The Sound to play when the check box is hovered  */
	FSlateSound HoveredSound;

	/** The Sound to play when the check box is checked */
	FSlateSound CheckedSound;

	/** The Sound to play when the check box is unchecked */
	FSlateSound UncheckedSound;
};
