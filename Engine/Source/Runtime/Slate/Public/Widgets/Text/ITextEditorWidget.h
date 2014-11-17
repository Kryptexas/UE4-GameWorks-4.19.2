// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

namespace ECursorMoveMethod
{
	enum Type
	{
		/** Move one character at a time (e.g. arrow left, right). */
		CharacterHorizontal,
		/** Move vertically (e.g. arrow up, down) */
		CharacterVertical,
		/** Move one word at a time (e.g. ctrl is held down). */
		Word,
	};
}

namespace ECursorAction
{
	enum Type
	{
		/** Just relocate the cursor. */
		MoveCursor,
		/** Move the cursor and select any text that it passes over. */
		SelectText
	};
}

namespace ETextLocation
{
	enum Type
	{
		/** Jump to the beginning of text. (e.g. Ctrl+Home) */
		BeginningOfDocument,
		/** Jump to the end of text. (e.g. Ctrl+End) */
		EndOfDocument,
		/** Jump to the beginning of the line or beginning of text within a line (e.g. Home) */
		BeginningOfLine,
		/** Jump to the end of line (e.g. End) */
		EndOfLine
	};
}

class SLATE_API ITextEditorWidget
{
	public:

	/**
	 * Call this before the text is changed interactively by the user (handles Undo, etc.)
	 */
	virtual void StartChangingText() = 0;

	/**
	 * Called when text is finished being changed interactively by the user
	 */
	virtual void FinishChangingText() = 0;
	
	/**
	 * Can the user edit the text?
	 */
	virtual bool GetIsReadOnly() const = 0;

	/**
	 * Deletes the character to the left of the caret
	 */
	virtual void BackspaceChar() = 0;

	/**
	 * Deletes the character to the right of the caret
	 */
	virtual void DeleteChar() = 0;
	
	/**
	 * The user pressed this character key. Should we actually type it?
	 * NOT RELIABLE VALIDATION.
	 */
	virtual bool CanTypeCharacter(const TCHAR CharInQuestion) const = 0;

	/**
	 * Adds a character at the caret position
	 *
	 * @param  InChar  Character to add
	 */
	virtual void TypeChar( const int32 Character ) = 0;
	
	/**
	 * Move the cursors, left, right, up down, one character or word at a time.
	 * Optionally select text while moving.
	 * 
	 * @param  Method       Move horizontally, vertically, or whole words horizontally.
	 * @param  Direction    Move forward/backward by number of units specified; usuatlly +/-1.
	 * @param  Action       Just move or select text.
	 */
	virtual FReply MoveCursor( ECursorMoveMethod::Type Method, int8 Direction, ECursorAction::Type Action ) = 0;
	
	/**
	 * Jump the cursor to a special location: e.g. end of line, beginning of document, etc..
	 */
	virtual void JumpTo(ETextLocation::Type JumpLocation, ECursorAction::Type Action) = 0;

	/**
	 * Clears text selection
	 */
	virtual void ClearSelection() = 0;
	
	/**
	 * Selects all text
	 */
	virtual void SelectAllText() = 0;

	/**
	 * Executed when the user presses ESC
	 */
	virtual FReply OnEscape() = 0;

	/**
	 * Executed when the user presses ENTER
	 */
	virtual void OnEnter() = 0;

	/**
	 * Returns true if 'Cut' is currently a valid action
	 *
	 * @return	True to allow this action, otherwise false
	 */
	virtual bool CanExecuteCut() const = 0;
	
	/**
	 * Cuts selected text to the clipboard
	 */
	virtual void CutSelectedTextToClipboard() = 0;

	/**
	 * Returns true if 'Copy' is currently a valid action
	 *
	 * @return	True to allow this action, otherwise false
	 */
	virtual bool CanExecuteCopy() const = 0;

	/**
	 * Copies selected text to the clipboard
	 */
	virtual void CopySelectedTextToClipboard() = 0;

	/**
	 * Returns true if 'Paste' is currently a valid action
	 *
	 * @return	True to allow this action, otherwise false
	 */
	virtual bool CanExecutePaste() const = 0;

	/**
	 * Pastes text from the clipboard
	 */
	virtual void PasteTextFromClipboard() = 0;
	
	/**
	 * Returns true if 'Undo' is currently a valid action
	 *
	 * @return	True to allow this action, otherwise false
	 */
	virtual bool CanExecuteUndo() const = 0;

	/**
	 * Rolls back state to the most recent undo history's state
	 */
	virtual void Undo() = 0;

	/**
	 * Restores cached undo state that was previously rolled back
	 */
	virtual void Redo() = 0;
};
