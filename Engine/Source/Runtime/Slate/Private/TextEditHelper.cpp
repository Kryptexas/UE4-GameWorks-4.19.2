// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"
#include "TextEditHelper.h"

/**
 * Ensure that text transactions are always completed.
 * i.e. never forget to call FinishChangingText();
 */
struct FScopedTextTransaction
{
	explicit FScopedTextTransaction( ITextEditorWidget& InTextEditor )
	: TextEditor( InTextEditor )
	{
		TextEditor.StartChangingText();
	}

	~FScopedTextTransaction()
	{
		TextEditor.FinishChangingText();
	}

	ITextEditorWidget& TextEditor;
};

FReply FTextEditHelper::OnKeyChar( const FGeometry& MyGeometry, const FCharacterEvent& InCharacterEvent, class ITextEditorWidget& TextEditor )
{
	FReply Reply = FReply::Unhandled();

	// Check for special characters
	const TCHAR Character = InCharacterEvent.GetCharacter();

	switch( Character )
	{
		// Backspace
	case TCHAR( 8 ):
		{
			if( !TextEditor.GetIsReadOnly() )
			{
				FScopedTextTransaction TextTransaction(TextEditor);

				TextEditor.BackspaceChar();
				
				Reply = FReply::Handled();
			}
		}
		break;


		// Tab
	case TCHAR( '\t' ):
		{
			Reply = FReply::Handled();
		}
		break;


		// Swallow OnKeyChar keys that we don't want to be entered into the buffer
	case 1:		// Swallow Ctrl+A, we handle that through OnKeyDown
	case 3:		// Swallow Ctrl+C, we handle that through OnKeyDown
	case 13:	// Swallow Enter, we handle that through OnKeyDown
	case 22:	// Swallow Ctrl+V, we handle that through OnKeyDown
	case 24:	// Swallow Ctrl+X, we handle that through OnKeyDown
	case 25:	// Swallow Ctrl+Y, we handle that through OnKeyDown
	case 26:	// Swallow Ctrl+Z, we handle that through OnKeyDown
	case 27:	// Swallow ESC, we handle that through OnKeyDown
	case 127:	// Swallow CTRL+Backspace, we handle that through OnKeyDown
		Reply = FReply::Handled();
		break;


		// Any other character!
	default:
		{
			// Type the character, but only if it is allowed.
			if (!TextEditor.GetIsReadOnly() && TextEditor.CanTypeCharacter(Character))
			{
				FScopedTextTransaction TextTransaction(TextEditor);
				TextEditor.TypeChar( Character );
				Reply = FReply::Handled();
			}
			else
			{
				Reply = FReply::Unhandled();
			}
		}
		break;

	}


	return Reply;
}

FReply FTextEditHelper::OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent, ITextEditorWidget& TextEditor )
{
	const FKey Key = InKeyboardEvent.GetKey();

	if( Key == EKeys::Left )
	{
		return TextEditor.MoveCursor(
			// Ctrl moves a whole word instead of one character.	
			InKeyboardEvent.IsControlDown() ? ECursorMoveMethod::Word : ECursorMoveMethod::CharacterHorizontal,
			// Move left
			-1,
			// Shift selects text.	
			InKeyboardEvent.IsShiftDown() ? ECursorAction::SelectText : ECursorAction::MoveCursor
		);
	}
	else if( Key == EKeys::Right )
	{
		return TextEditor.MoveCursor(
			// Ctrl moves a whole word instead of one character.	
			InKeyboardEvent.IsControlDown() ? ECursorMoveMethod::Word : ECursorMoveMethod::CharacterHorizontal,
			// Move right
			+1,
			// Shift selects text.	
			InKeyboardEvent.IsShiftDown() ? ECursorAction::SelectText : ECursorAction::MoveCursor
		);
	}
	else if ( Key == EKeys::Up )
	{
		return TextEditor.MoveCursor(
			ECursorMoveMethod::CharacterVertical,
			// Move up
			-1,
			// Shift selects text.	
			InKeyboardEvent.IsShiftDown() ? ECursorAction::SelectText : ECursorAction::MoveCursor
		);
	}
	else if ( Key == EKeys::Down )
	{
		return TextEditor.MoveCursor(
			ECursorMoveMethod::CharacterVertical,
			// Move down
			+1,
			// Shift selects text.	
			InKeyboardEvent.IsShiftDown() ? ECursorAction::SelectText : ECursorAction::MoveCursor
		);
	}
	else if( Key == EKeys::Home )
	{
		// Go to the beginning of the document; select text if Shift is down.
		TextEditor.JumpTo(
			(InKeyboardEvent.IsControlDown() ) ? ETextLocation::BeginningOfDocument : ETextLocation::BeginningOfLine,
			(InKeyboardEvent.IsShiftDown()) ? ECursorAction::SelectText : ECursorAction::MoveCursor );

		return FReply::Handled();
	}
	else if ( Key == EKeys::End )
	{
		// Go to the end of the document; select text if Shift is down.
		TextEditor.JumpTo(
			(InKeyboardEvent.IsControlDown() ) ? ETextLocation::EndOfDocument : ETextLocation::EndOfLine,
			(InKeyboardEvent.IsShiftDown()) ? ECursorAction::SelectText : ECursorAction::MoveCursor );

		return FReply::Handled();
	}
	else if( Key == EKeys::Enter )
	{
		TextEditor.OnEnter();

		return FReply::Handled();
	}
	else if( Key == EKeys::Delete && !TextEditor.GetIsReadOnly() )
	{
		// @Todo: Slate keybindings support more than one set of keys. 
		// Delete to next word boundary (Ctrl+Delete)
		if (InKeyboardEvent.IsControlDown() && !InKeyboardEvent.IsAltDown() && !InKeyboardEvent.IsShiftDown())
		{
			TextEditor.MoveCursor(ECursorMoveMethod::Word, +1, ECursorAction::SelectText);
		}

		TextEditor.DeleteChar();
		
		return FReply::Handled();
	}	
	else if( Key == EKeys::Escape )
	{
		return TextEditor.OnEscape();;
	}

	// @Todo: Slate keybindings support more than one set of keys. 
	//Alternate key for cut (Shift+Delete)
	else if( Key == EKeys::Delete && InKeyboardEvent.IsShiftDown() && TextEditor.CanExecuteCut() )
	{
		// Cut text to clipboard
		TextEditor.CutSelectedTextToClipboard();
		
		return FReply::Handled();
	}

	// @Todo: Slate keybindings support more than one set of keys. 
	// Alternate key for copy (Ctrl+Insert) 
	else if( Key == EKeys::Insert && InKeyboardEvent.IsControlDown() && TextEditor.CanExecuteCopy() ) 
	{
		// Copy text to clipboard
		TextEditor.CopySelectedTextToClipboard();
		
		return FReply::Handled();
	}


	// @Todo: Slate keybindings support more than one set of keys. 
	// Alternate key for paste (Shift+Insert) 
	else if( Key == EKeys::Insert && InKeyboardEvent.IsShiftDown() && TextEditor.CanExecutePaste() )
	{
		// Paste text from clipboard
		TextEditor.PasteTextFromClipboard();
		
		return FReply::Handled();
	}

	// @Todo: Slate keybindings support more than one set of keys. 
	//Alternate key for undo (Alt+Backspace)
	else if( Key == EKeys::BackSpace && InKeyboardEvent.IsAltDown() && !InKeyboardEvent.IsShiftDown() && TextEditor.CanExecuteUndo() )
	{
		// Undo
		TextEditor.Undo();
		
		return FReply::Handled();
	}

	// @Todo: Slate keybindings support more than one set of keys. 
	// Delete to previous word boundary (Ctrl+Backspace)
	else if( Key == EKeys::BackSpace && InKeyboardEvent.IsControlDown() && !InKeyboardEvent.IsAltDown() && !InKeyboardEvent.IsShiftDown() && !TextEditor.GetIsReadOnly() )
	{
		FScopedTextTransaction TextTransaction(TextEditor);

		TextEditor.MoveCursor(ECursorMoveMethod::Word, -1, ECursorAction::SelectText);
		TextEditor.BackspaceChar();

		return FReply::Handled();
	}


	// Ctrl+Y (or Ctrl+Shift+Z, or Alt+Shift+Backspace) to redo
	else if( !TextEditor.GetIsReadOnly() && ( ( Key == EKeys::Y && InKeyboardEvent.IsControlDown() ) ||
		( Key == EKeys::Z && InKeyboardEvent.IsControlDown() && InKeyboardEvent.IsShiftDown() ) ||
		( Key == EKeys::BackSpace && InKeyboardEvent.IsAltDown() && InKeyboardEvent.IsShiftDown() ) ) )
	{
		// Redo
		TextEditor.Redo();
		
		return FReply::Handled();
	}
	else if( !InKeyboardEvent.IsAltDown() && !InKeyboardEvent.IsControlDown() && InKeyboardEvent.GetKey() != EKeys::Tab && InKeyboardEvent.GetCharacter() != 0 )
	{
		// Shift and a character was pressed or a single character was pressed.  We will type something in an upcoming OnKeyChar event.  
		// Absorb this event so it is not bubbled and handled by other widgets that could have something bound to the key press.

		//Note: Tab can generate a character code but not result in a typed character in this box so we ignore it.
		return FReply::Handled();
	}
	else
	{
		return FReply::Unhandled();
	}
}