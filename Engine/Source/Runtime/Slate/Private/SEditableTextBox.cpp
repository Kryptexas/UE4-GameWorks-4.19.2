// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"


/**
 * Construct this widget
 *
 * @param	InArgs	The declaration data for this widget
 */
void SEditableTextBox::Construct( const FArguments& InArgs )
{
	check (InArgs._Style);
	BorderImageNormal = &InArgs._Style->BackgroundImageNormal;
	BorderImageHovered = &InArgs._Style->BackgroundImageHovered;
	BorderImageFocused = &InArgs._Style->BackgroundImageFocused;
	BorderImageReadOnly = &InArgs._Style->BackgroundImageReadOnly;
	TAttribute<FMargin> Padding = InArgs._Padding.IsSet() ? InArgs._Padding : InArgs._Style->Padding;
	TAttribute<FSlateFontInfo> Font = InArgs._Font.IsSet() ? InArgs._Font : InArgs._Style->Font;
	TAttribute<FSlateColor> ForegroundColor = InArgs._ForegroundColor.IsSet() ? InArgs._ForegroundColor : InArgs._Style->ForegroundColor;
	TAttribute<FSlateColor> BackgroundColor = InArgs._BackgroundColor.IsSet() ? InArgs._BackgroundColor : InArgs._Style->BackgroundColor;
	ReadOnlyForegroundColor = InArgs._ReadOnlyForegroundColor.IsSet() ? InArgs._ReadOnlyForegroundColor : InArgs._Style->ReadOnlyForegroundColor;

	SBorder::Construct( SBorder::FArguments()
		.BorderImage( this, &SEditableTextBox::GetBorderImage )
		.BorderBackgroundColor( BackgroundColor )
		.ForegroundColor( ForegroundColor )
		.Padding( 0 )
		[
			SAssignNew( Box, SHorizontalBox)
			+SHorizontalBox::Slot()
			.VAlign(VAlign_Fill)
			.HAlign(HAlign_Fill)
			.FillWidth(1)
			[
				SAssignNew( EditableText, SEditableText )
				.Text( InArgs._Text )
				.HintText( InArgs._HintText )
				.Font( Font )
				.IsReadOnly( InArgs._IsReadOnly )
				.IsPassword( InArgs._IsPassword )
				.IsCaretMovedWhenGainFocus( InArgs._IsCaretMovedWhenGainFocus )
				.SelectAllTextWhenFocused( InArgs._SelectAllTextWhenFocused.Get() )
				.RevertTextOnEscape( InArgs._RevertTextOnEscape.Get() )
				.ClearKeyboardFocusOnCommit( InArgs._ClearKeyboardFocusOnCommit.Get() )
				.OnTextChanged( InArgs._OnTextChanged )
				.OnTextCommitted( InArgs._OnTextCommitted )
				.MinDesiredWidth( InArgs._MinDesiredWidth )
				.SelectAllTextOnCommit( InArgs._SelectAllTextOnCommit )
				.Padding( Padding )
			]
		]
	);

	ErrorReporting = InArgs._ErrorReporting;
	if ( ErrorReporting.IsValid() )
	{
		Box->AddSlot()
		.AutoWidth()
		.Padding(3,0)
		[
			ErrorReporting->AsWidget()
		];
	}

}


/**
 * Sets the text string currently being edited 
 *
 * @param  InNewText  The new text string
 */
void SEditableTextBox::SetText( const TAttribute< FText >& InNewText )
{
	EditableText->SetText( InNewText );
}

void SEditableTextBox::SetError( const FText& InError )
{
	SetError( InError.ToString() );
}

void SEditableTextBox::SetError( const FString& InError )
{
	const bool bHaveError = !InError.IsEmpty();

	if ( !ErrorReporting.IsValid() )
	{
		// No error reporting was specified; make a default one
		TSharedPtr<SPopupErrorText> ErrorTextWidget;
		Box->AddSlot()
		.AutoWidth()
		.Padding(3,0)
		[
			SAssignNew( ErrorTextWidget, SPopupErrorText )
		];
		ErrorReporting = ErrorTextWidget;
	}

	ErrorReporting->SetError( InError );
}

/** @return Border image for the text box based on the hovered and focused state */
const FSlateBrush* SEditableTextBox::GetBorderImage() const
{
	if ( EditableText->GetIsReadOnly() )
	{
		return BorderImageReadOnly;
	}
	else if ( EditableText->HasKeyboardFocus() )
	{
		return BorderImageFocused;
	}
	else
	{
		if ( EditableText->IsHovered() )
		{
			return BorderImageHovered;
		}
		else
		{
			return BorderImageNormal;
		}
	}
}

bool SEditableTextBox::SupportsKeyboardFocus() const
{
	return StaticCastSharedPtr<SWidget>(EditableText)->SupportsKeyboardFocus();
}

bool SEditableTextBox::HasKeyboardFocus() const
{
	// Since keyboard focus is forwarded to our editable text, we will test it instead
	return SBorder::HasKeyboardFocus() || EditableText->HasKeyboardFocus();
}

FReply SEditableTextBox::OnKeyboardFocusReceived( const FGeometry& MyGeometry, const FKeyboardFocusEvent& InKeyboardFocusEvent )
{
	FReply Reply = FReply::Handled();

	if ( InKeyboardFocusEvent.GetCause() != EKeyboardFocusCause::Cleared )
	{
		// Forward keyboard focus to our editable text widget
		Reply.SetKeyboardFocus( EditableText.ToSharedRef(), InKeyboardFocusEvent.GetCause() );
	}

	return Reply;
}

FReply SEditableTextBox::OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent )
{
	FKey Key = InKeyboardEvent.GetKey();

	if( Key == EKeys::Escape && EditableText->HasKeyboardFocus() )
	{
		// Clear selection
		EditableText->ClearSelection();
		return FReply::Handled().SetKeyboardFocus( SharedThis( this ), EKeyboardFocusCause::Cleared );
	}

	return FReply::Unhandled();
}