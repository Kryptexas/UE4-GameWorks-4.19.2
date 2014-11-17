// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlatePrivatePCH.h"
#include "TextEditHelper.h"

/**
 * Ensure that text transactions are always completed.
 * i.e. never forget to call FinishChangingText();
 */
struct FScopedTextTransaction
{
	explicit FScopedTextTransaction( const TSharedRef< ITextEditorWidget >& InTextEditor )
	: TextEditor( InTextEditor )
	{
		TextEditor->StartChangingText();
	}

	~FScopedTextTransaction()
	{
		TextEditor->FinishChangingText();
	}

	const TSharedRef< ITextEditorWidget > TextEditor;
};

FReply FTextEditHelper::OnKeyChar( const FCharacterEvent& InCharacterEvent, const TSharedRef< ITextEditorWidget >& TextEditor )
{
	FReply Reply = FReply::Unhandled();

	// Check for special characters
	const TCHAR Character = InCharacterEvent.GetCharacter();

	switch( Character )
	{
		// Backspace
	case TCHAR( 8 ):
		{
			if( !TextEditor->GetIsReadOnly() )
			{
				FScopedTextTransaction TextTransaction(TextEditor);

				TextEditor->BackspaceChar();
				
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
			if (!TextEditor->GetIsReadOnly() && TextEditor->CanTypeCharacter(Character))
			{
				FScopedTextTransaction TextTransaction(TextEditor);
				TextEditor->TypeChar( Character );
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

FReply FTextEditHelper::OnKeyDown( const FKeyboardEvent& InKeyboardEvent, const TSharedRef< ITextEditorWidget >& TextEditor )
{
	const FKey Key = InKeyboardEvent.GetKey();

	if( Key == EKeys::Left )
	{
		return TextEditor->MoveCursor(
			// Ctrl moves a whole word instead of one character.	
			InKeyboardEvent.IsControlDown() ? ECursorMoveMethod::Word : ECursorMoveMethod::CharacterHorizontal,
			// Move left
			FVector2D( -1, 0 ),
			// Shift selects text.	
			InKeyboardEvent.IsShiftDown() ? ECursorAction::SelectText : ECursorAction::MoveCursor
		);
	}
	else if( Key == EKeys::Right )
	{
		return TextEditor->MoveCursor(
			// Ctrl moves a whole word instead of one character.	
			InKeyboardEvent.IsControlDown() ? ECursorMoveMethod::Word : ECursorMoveMethod::CharacterHorizontal,
			// Move right
			FVector2D( +1, 0 ),
			// Shift selects text.	
			InKeyboardEvent.IsShiftDown() ? ECursorAction::SelectText : ECursorAction::MoveCursor
		);
	}
	else if ( Key == EKeys::Up )
	{
		return TextEditor->MoveCursor(
			ECursorMoveMethod::CharacterVertical,
			// Move up
			FVector2D( 0, -1 ),
			// Shift selects text.	
			InKeyboardEvent.IsShiftDown() ? ECursorAction::SelectText : ECursorAction::MoveCursor
		);
	}
	else if ( Key == EKeys::Down )
	{
		return TextEditor->MoveCursor(
			ECursorMoveMethod::CharacterVertical,
			// Move down
			FVector2D( 0, +1 ),
			// Shift selects text.	
			InKeyboardEvent.IsShiftDown() ? ECursorAction::SelectText : ECursorAction::MoveCursor
		);
	}
	else if( Key == EKeys::Home )
	{
		// Go to the beginning of the document; select text if Shift is down.
		TextEditor->JumpTo(
			(InKeyboardEvent.IsControlDown() ) ? ETextLocation::BeginningOfDocument : ETextLocation::BeginningOfLine,
			(InKeyboardEvent.IsShiftDown()) ? ECursorAction::SelectText : ECursorAction::MoveCursor );

		return FReply::Handled();
	}
	else if ( Key == EKeys::End )
	{
		// Go to the end of the document; select text if Shift is down.
		TextEditor->JumpTo(
			(InKeyboardEvent.IsControlDown() ) ? ETextLocation::EndOfDocument : ETextLocation::EndOfLine,
			(InKeyboardEvent.IsShiftDown()) ? ECursorAction::SelectText : ECursorAction::MoveCursor );

		return FReply::Handled();
	}
	else if( Key == EKeys::Enter )
	{
		FScopedTextTransaction TextTransaction(TextEditor);

		TextEditor->OnEnter();

		return FReply::Handled();
	}
	else if( Key == EKeys::Delete && !TextEditor->GetIsReadOnly() )
	{
		// @Todo: Slate keybindings support more than one set of keys. 
		// Delete to next word boundary (Ctrl+Delete)
		if (InKeyboardEvent.IsControlDown() && !InKeyboardEvent.IsAltDown() && !InKeyboardEvent.IsShiftDown())
		{
			TextEditor->MoveCursor(
				ECursorMoveMethod::Word, 
				// Move right
				FVector2D(+1, 0),
				// selects text.	
				ECursorAction::SelectText
			);
		}

		FScopedTextTransaction TextTransaction(TextEditor);

		// Delete selected text
		TextEditor->DeleteChar();
		
		return FReply::Handled();
	}	
	else if( Key == EKeys::Escape )
	{
		return TextEditor->OnEscape();;
	}

	// @Todo: Slate keybindings support more than one set of keys. 
	//Alternate key for cut (Shift+Delete)
	else if( Key == EKeys::Delete && InKeyboardEvent.IsShiftDown() && TextEditor->CanExecuteCut() )
	{
		// Cut text to clipboard
		TextEditor->CutSelectedTextToClipboard();
		
		return FReply::Handled();
	}

	// @Todo: Slate keybindings support more than one set of keys. 
	// Alternate key for copy (Ctrl+Insert) 
	else if( Key == EKeys::Insert && InKeyboardEvent.IsControlDown() && TextEditor->CanExecuteCopy() ) 
	{
		// Copy text to clipboard
		TextEditor->CopySelectedTextToClipboard();
		
		return FReply::Handled();
	}


	// @Todo: Slate keybindings support more than one set of keys. 
	// Alternate key for paste (Shift+Insert) 
	else if( Key == EKeys::Insert && InKeyboardEvent.IsShiftDown() && TextEditor->CanExecutePaste() )
	{
		// Paste text from clipboard
		TextEditor->PasteTextFromClipboard();
		
		return FReply::Handled();
	}

	// @Todo: Slate keybindings support more than one set of keys. 
	//Alternate key for undo (Alt+Backspace)
	else if( Key == EKeys::BackSpace && InKeyboardEvent.IsAltDown() && !InKeyboardEvent.IsShiftDown() && TextEditor->CanExecuteUndo() )
	{
		// Undo
		TextEditor->Undo();
		
		return FReply::Handled();
	}

	// @Todo: Slate keybindings support more than one set of keys. 
	// Delete to previous word boundary (Ctrl+Backspace)
	else if( Key == EKeys::BackSpace && InKeyboardEvent.IsControlDown() && !InKeyboardEvent.IsAltDown() && !InKeyboardEvent.IsShiftDown() && !TextEditor->GetIsReadOnly() )
	{
		FScopedTextTransaction TextTransaction(TextEditor);

		TextEditor->MoveCursor(
			ECursorMoveMethod::Word, 
			// Move left
			FVector2D(-1, 0), 
			ECursorAction::SelectText
		);
		TextEditor->BackspaceChar();

		return FReply::Handled();
	}


	// Ctrl+Y (or Ctrl+Shift+Z, or Alt+Shift+Backspace) to redo
	else if( !TextEditor->GetIsReadOnly() && ( ( Key == EKeys::Y && InKeyboardEvent.IsControlDown() ) ||
		( Key == EKeys::Z && InKeyboardEvent.IsControlDown() && InKeyboardEvent.IsShiftDown() ) ||
		( Key == EKeys::BackSpace && InKeyboardEvent.IsAltDown() && InKeyboardEvent.IsShiftDown() ) ) )
	{
		// Redo
		TextEditor->Redo();
		
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
FReply FTextEditHelper::OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& InMouseEvent, const TSharedRef< ITextEditorWidget >& TextEditor )
{
	FReply Reply = FReply::Unhandled();

	// If the mouse is already captured, then don't allow a new action to be taken
	if( !TextEditor->GetWidget()->HasMouseCapture() )
	{
		if( InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton ||
			InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton )
		{
			// Am I getting focus right now?
			const bool bIsGettingFocus = !TextEditor->GetWidget()->HasKeyboardFocus();
			if( bIsGettingFocus )
			{
				// We might be receiving keyboard focus due to this event.  Because the keyboard focus received callback
				// won't fire until after this function exits, we need to make sure our widget's state is in order early

				// Assume we'll be given keyboard focus, so load text for editing
				TextEditor->LoadText();

				// Reset 'mouse has moved' state.  We'll use this in OnMouseMove to determine whether we
				// should reset the selection range to the caret's position.
				TextEditor->SetWasFocusedByLastMouseDown( true );
			}

			if( InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton )
			{
				if( InMouseEvent.IsShiftDown() )
				{
					TextEditor->MoveCursor( ECursorMoveMethod::ScreenPosition, MyGeometry.AbsoluteToLocal( InMouseEvent.GetScreenSpacePosition() ), ECursorAction::SelectText );
				}
				else
				{
					// Deselect any text that was selected
					TextEditor->ClearSelection();
					TextEditor->MoveCursor( ECursorMoveMethod::ScreenPosition, MyGeometry.AbsoluteToLocal( InMouseEvent.GetScreenSpacePosition() ), ECursorAction::MoveCursor );
				}

				// Start drag selection
				TextEditor->BeginDragSelection();
			}
			else if( InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton )
			{
				// If the user right clicked on a character that wasn't already selected, we'll clear
				// the selection
				if (TextEditor->AnyTextSelected() && !TextEditor->IsTextSelectedAt(MyGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition())))
				{
					// Deselect any text that was selected
					TextEditor->ClearSelection();
				}
			}

			// Right clicking to summon context menu, but we'll do that on mouse-up.
			Reply = FReply::Handled();
			Reply.CaptureMouse( TextEditor->GetWidget() );
			Reply.SetKeyboardFocus( TextEditor->GetWidget(), EKeyboardFocusCause::Mouse );
		}
	}

	return Reply;
}

FReply FTextEditHelper::OnMouseMove( const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent, const TSharedRef< ITextEditorWidget >& TextEditor )
{
	FReply Reply = FReply::Unhandled();

	if( TextEditor->IsDragSelecting() && TextEditor->GetWidget()->HasMouseCapture() )
	{
		TextEditor->MoveCursor( ECursorMoveMethod::ScreenPosition, InMyGeometry.AbsoluteToLocal( InMouseEvent.GetScreenSpacePosition() ), ECursorAction::SelectText );
		TextEditor->SetHasDragSelectedSinceFocused( true );
		Reply = FReply::Handled();
	}

	return Reply;
}

FReply FTextEditHelper::OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& InMouseEvent, const TSharedRef< ITextEditorWidget >& TextEditor )
{
	FReply Reply = FReply::Unhandled();

	// The mouse must have been captured by either left or right button down before we'll process mouse ups
	if( TextEditor->GetWidget()->HasMouseCapture() )
	{
		if( InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton &&
			TextEditor->IsDragSelecting() )
		{
			// No longer drag-selecting
			TextEditor->EndDragSelection();

			// If we received keyboard focus on this click, then we'll want to select all of the text
			// when the user releases the mouse button, unless the user actually dragged the mouse
			// while holding the button down, in which case they've already selected some text and
			// we'll leave things alone!
			if( TextEditor->WasFocusedByLastMouseDown() )
			{
				if( !TextEditor->HasDragSelectedSinceFocused() )
				{
					if( TextEditor->SelectAllTextWhenFocused() )
					{
						// User wasn't dragging the mouse, so go ahead and select all of the text now
						// that we've become focused
						TextEditor->SelectAllText();

						// Move the cursor to the end of the string
						TextEditor->JumpTo( ETextLocation::EndOfDocument, ECursorAction::MoveCursor );

						// @todo Slate: In this state, the caret should actually stay hidden (until the user interacts again), and we should not move the caret
					}
				}

				TextEditor->SetWasFocusedByLastMouseDown( false );
			}

			// Release mouse capture
			Reply = FReply::Handled();
			Reply.ReleaseMouseCapture();
		}
		else if( InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton )
		{
			if ( MyGeometry.IsUnderLocation( InMouseEvent.GetScreenSpacePosition() ) )
			{
				// Right clicked, so summon a context menu if the cursor is within the widget
				TextEditor->SummonContextMenu( InMouseEvent.GetScreenSpacePosition() );
			}

			// Release mouse capture
			Reply = FReply::Handled();
			Reply.ReleaseMouseCapture();
		}
	}

	return Reply;
}

FReply FTextEditHelper::OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent, const TSharedRef< ITextEditorWidget >& TextEditor)
{
	FReply Reply = FReply::Unhandled();

	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		TextEditor->SelectWordAt(InMyGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition()));

		Reply = FReply::Handled();
	}

	return Reply;
}

float FTextEditHelper::GetFontHeight(const FSlateFontInfo& FontInfo)
{
	const TSharedRef< FSlateFontMeasure > FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	return FontMeasure->GetMaxCharacterHeight(FontInfo);
}

float FTextEditHelper::CalculateCaretWidth(const float FontMaxCharHeight)
{
	// We adjust the width of the cursor because on smaller fonts the cursor would overlap too much of the characters it is next too.
	// Other programs get around this by inverting the colors under the cursor. This causes a single character to be rendered as two characters.

	// The caret with should be at least one pixel. For very small fonts the width could be < 1 which makes it not visible
	return FMath::Max(1.0f, EditableTextDefs::CaretWidthPercent * FontMaxCharHeight);
}
