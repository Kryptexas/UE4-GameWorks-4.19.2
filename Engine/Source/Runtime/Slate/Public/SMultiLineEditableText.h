// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ITextEditorWidget.h"

/** Information about the substring as it would appear as a result of soft-wrapping a line of text. */
struct FWrappedStringSlice
{
	/** 
	 * Create a substring info that sources InSourceString, begins at InFirstCharIndex and ends at InLastCharIndex (exclusive).
	 * The substring measures InSize.X by InSize.Y.
	 */
	FWrappedStringSlice( FString& InSourceString, int32 InFirstCharIndex, int32 InLastCharIndex, FVector2D InSize )
	: SourceString( InSourceString )
	, FirstCharIndex( InFirstCharIndex )
	, LastCharIndex( InLastCharIndex )
	, Size( InSize )
	{
	}

	/** The substring sources this string. */
	FString& SourceString;
	/** Index of the first character in the substring */
	int32 FirstCharIndex;
	/** Index of the last character (exclusive) */
	int32 LastCharIndex;
	/** The measured size of this substring given the current font and point size. */
	FVector2D Size;
};

/** An editable text widget that supports multiple lines and soft word-wrapping. */
class SLATE_API SMultiLineEditableText : public SLeafWidget, public ITextEditorWidget
{
public:
	SLATE_BEGIN_ARGS( SMultiLineEditableText )
	: _Text()
	, _ScrollBar()
	, _PreferredSize(FVector2D(100,100))
	{}
		/** The initial text that will appear in the widget. */
		SLATE_TEXT_ARGUMENT( Text )

		/** Scroll bar that is used to visualize and manipulate the scrolling of the text */
		SLATE_ARGUMENT( TSharedPtr<class SScrollBar>, ScrollBar )

		/** How big should this text field be for optimum operation. The value is irrelevant when the editable text fills all available space. */
		SLATE_ARGUMENT( FVector2D, PreferredSize)

	SLATE_END_ARGS()

	SMultiLineEditableText();

	void Construct( const FArguments& InArgs );

private:
	
	/** Location within the text model. */
	struct FTextLocation
	{
	public:
		FTextLocation( const int32 InLineIndex = 0, const int32 InOffset = 0 )
		: LineIndex( InLineIndex )
		, Offset( InOffset )
		{

		}

		bool operator==( const FTextLocation& Other ) const
		{
			return
				LineIndex == Other.LineIndex &&
				Offset == Other.Offset;
		}

		bool operator<( const FTextLocation& Other ) const
		{
			return this->LineIndex < Other.LineIndex || (this->LineIndex == Other.LineIndex && this->Offset < Other.Offset);
		}

		int32 GetLineIndex() const { return LineIndex; }
		int32 GetOffset() const { return Offset; }

		private:
		int32 LineIndex;
		int32 Offset;
	};

	/** Range between two locations. Guarantees that the First location is before the Last. */
	struct FTextRange
	{
		/**
		 * Construct a Range from two locations: A and B.
		 * Guarantees that First will be the earlier of the two.
		 */
		FTextRange( const FTextLocation& A, const FTextLocation& B )
		: First( (A < B) ? A : B )
		, Last( (A < B) ? B : A )
		{
		}

		const FTextLocation First;
		const FTextLocation Last;
	};

private:
	// BEGIN ITextEditorWidget interface
	virtual void StartChangingText() OVERRIDE;
	virtual void FinishChangingText() OVERRIDE;
	virtual bool GetIsReadOnly() const OVERRIDE;
	virtual void BackspaceChar() OVERRIDE;
	virtual void DeleteChar() OVERRIDE;
	virtual bool CanTypeCharacter(const TCHAR CharInQuestion) const OVERRIDE;
	virtual void TypeChar( const int32 Character ) OVERRIDE;
	virtual FReply MoveCursor( ECursorMoveMethod::Type Method, int8 Direction, ECursorAction::Type Action ) OVERRIDE;
	virtual void JumpTo(ETextLocation::Type JumpLocation, ECursorAction::Type Action) OVERRIDE;
	virtual void ClearSelection() OVERRIDE;
	virtual void SelectAllText() OVERRIDE;
	virtual FReply OnEscape() OVERRIDE;
	virtual void OnEnter() OVERRIDE;
	virtual bool CanExecuteCut() const OVERRIDE;
	virtual void CutSelectedTextToClipboard() OVERRIDE;
	virtual bool CanExecuteCopy() const OVERRIDE;
	virtual void CopySelectedTextToClipboard() OVERRIDE;
	virtual bool CanExecutePaste() const OVERRIDE;
	virtual void PasteTextFromClipboard() OVERRIDE;
	virtual bool CanExecuteUndo() const OVERRIDE;
	virtual void Undo() OVERRIDE;
	virtual void Redo() OVERRIDE;
	// END ITextEditorWidget interface

private:
	// BEGIN SWidget interface
	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) OVERRIDE;
	virtual int32 OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const OVERRIDE;
	virtual FVector2D ComputeDesiredSize() const OVERRIDE;
	virtual bool SupportsKeyboardFocus() const OVERRIDE;
	virtual FReply OnKeyChar( const FGeometry& MyGeometry,const FCharacterEvent& InCharacterEvent ) OVERRIDE;
	virtual FReply OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent ) OVERRIDE;
	virtual FReply OnKeyUp( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent ) OVERRIDE;
	// END SWidget interface

private:
	
	/** Re-compute the wrapping information about the current text. */
	void ReWrap(const FGeometry& AllottedGeometry);

	/** Invoked when the user manipulates the scrollbar*/
	void OnScrollbarScrolled(float OffsetFraction);

	/** Remember where the cursor was when we started selecting. */
	void BeginSelecting( const FTextLocation& Endpoint );

	/** Given the current geometry, make the scrollbar reflect the current offset and fraction of the visible text. */
	void UpdateScrollbar( const FGeometry &AllottedGeometry );

	/**
	 * Given a location and a Direction to offset, return a new location.
	 *
	 * @param Location    Cursor location from which to offset
	 * @param Direction   Positive means right, negative means left.
	 */
	FTextLocation TranslatedLocation( const FTextLocation& CurrentLocation, int8 Direction ) const;

	/**
	 * Given a location and a Direction to offset, return a new location.
	 *
	 * @param Location    Cursor location from which to offset
	 * @param Direction   Positive means down, negative means up.
	 */
	FTextLocation TranslateLocationVertical( const FTextLocation& Location, int8 Direction ) const;

	/** Find the closest word boundary */
	FTextLocation ScanForWordBoundary( const FTextLocation& Location, int8 Direction ) const; 

	/** Get the character at Location */
	TCHAR GetCharacterAt( const FTextLocation& Location ) const;

	/** Are we at the beginning of all the text. */
	bool IsAtStartOfDoc( const FTextLocation& Location ) const;
	
	/** Are we at the end of all the text. */
	bool IsAtEndOfDoc( const FTextLocation& Location ) const;

	/** Is this location the beginning of a line */
	bool IsAtStartOfLine( const FTextLocation& Location ) const;

	/** Is this location the end of a line. */
	bool IsAtEndOfLine( const FTextLocation& Location ) const;

	/** Are we currently at the beginning of a word */
	bool IsAtWordStart( const FTextLocation& Location ) const;

	/** Given a location in unwrapped text, compute the location in wrapped line. */
	FTextLocation AbsoluteToWrapped( const FTextLocation& Location ) const;

	/** Given a range in unwrapped text, compute the range in wrapped text. */
	FTextRange AbsoluteToWrapped( const FTextRange& Range ) const;

	/** If we have a pointer to this scroll bar, use it to visualize and manipulate the scroll offset. */
	TWeakPtr<class SScrollBar> OptionalScrollBarPtr;

	/** We maintain our own version of the text as an array of strings. This is done for performance reasons. */
	TArray< TSharedRef<FString> > TextLines;
	
	/** The information about how the text is currently wrapped. */
	TArray< FWrappedStringSlice > WrappedText;

	/** Remember last frame's geometry for comparison and  */
	FGeometry CachedLastFrameGeometry;

	/** That start of the selection when there is a selection. The end is implicitly wherever the cursor happens to be. */
	TOptional<FTextLocation> SelectionStart;

	/** Current cursor position; there is always one. */
	FTextLocation CursorPosition;

	/** The font being used to render the text. */
	FSlateFontInfo FontInfo;

	/** Floating point offset in number of lines from the top of the document. */
	float NumLinesScrollOffset;

	/** The user probably wants the cursor where they last explicitly positioned it horizontally. */
	int32 PreferredCursorOffsetInLine;

	/** Last time the user did anything with the cursor.*/
	double LastCursorInteractionTime;

	/** How big should this text field be for optimum operation. The value is irrelevant when the editable text fills all available space. */
	FVector2D PreferredSize;
};
