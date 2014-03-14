// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


/**
 * A single log message for the output log, holding a message and
 * a style, for color and bolding of the message.
 */
struct FLogMessage
{
	FString Message;
	FName Style;

	FLogMessage(const FString& NewMessage, FName NewStyle = NAME_None)
		: Message(NewMessage)
		, Style(NewStyle)
	{
	}
};


/**
 * Console input box with command-completion support
 */
class SConsoleInputBox
	: public SCompoundWidget
{

public:

	SLATE_BEGIN_ARGS( SConsoleInputBox )
		: _SuggestionListPlacement( MenuPlacement_BelowAnchor )
		{}

		/** Where to place the suggestion list */
		SLATE_ARGUMENT( EMenuPlacement, SuggestionListPlacement )

	SLATE_END_ARGS()

	/** Protected console input box widget constructor, called by Slate */
	SConsoleInputBox();

	/**
	 * Construct this widget.  Called by the SNew() Slate macro.
	 *
	 * @param	InArgs	Declaration used by the SNew() macro to construct this widget
	 */
	void Construct( const FArguments& InArgs );

	/** Returns the editable text box associated with this widget.  Used to set focus directly. */
	TSharedRef< SEditableTextBox > GetEditableTextBox()
	{
		return InputText.ToSharedRef();
	}

	/** SWidget interface */
	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) OVERRIDE;

protected:

	virtual bool SupportsKeyboardFocus() const { return true; }

	// e.g. Tab or Key_Up
	virtual FReply OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& KeyboardEvent );

	void OnKeyboardFocusLost( const FKeyboardFocusEvent& InKeyboardFocusEvent );

	/** Handles entering in a command */
	void OnTextCommitted(const FText& InText, ETextCommit::Type CommitInfo);

	void OnTextChanged(const FText& InText);

	/** Makes the widget for the suggestions messages in the list view */
	TSharedRef<ITableRow> MakeSuggestionListItemWidget(TSharedPtr<FString> Message, const TSharedRef<STableViewBase>& OwnerTable);

	void SuggestionSelectionChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo);
		
	void SetSuggestions(TArray<FString>& Elements, bool bInHistoryMode);

	void MarkActiveSuggestion();

	void ClearSuggestions();

	FString GetSelectionText() const;


private:

	/** Editable text widget */
	TSharedPtr< SEditableTextBox > InputText;

	/** history / auto completion elements */
	TSharedPtr< SMenuAnchor > SuggestionBox;

	/** All log messages stored in this widget for the list view */
	TArray< TSharedPtr<FString> > Suggestions;

	/** The list view for showing all log messages. Should be replaced by a full text editor */
	TSharedPtr< SListView< TSharedPtr<FString> > > SuggestionListView;

	/** -1 if not set, otherwise index into Suggestions */
	int32 SelectedSuggestion;

	/** to prevent recursive calls in UI callback */
	bool bIgnoreUIUpdate; 
};


/**
 * Widget which holds a list view of logs of the program output
 * as well as a combo box for entering in new commands
 */
class SOutputLog 
	: public SCompoundWidget, public FOutputDevice
{

public:

	SLATE_BEGIN_ARGS( SOutputLog )
		: _Messages()
		{}
		
		/** All messages captured before this log window has been created */
		SLATE_ARGUMENT( TArray< TSharedPtr<FLogMessage> >, Messages )

	SLATE_END_ARGS()

	/** Destructor for output log, so we can unregister from notifications */
	~SOutputLog();

	/**
	 * Construct this widget.  Called by the SNew() Slate macro.
	 *
	 * @param	InArgs	Declaration used by the SNew() macro to construct this widget
	 */
	void Construct( const FArguments& InArgs );

	/**
	 * Creates FLogMessage objects from FOutputDevice log callback
	 *
	 * @param	V Message text
	 * @param Verbosity Message verbosity
	 * @param Category Message category
	 * @param OutMessages Array to receive created FLogMessage messages
	 *
	 * @return true if any messages have been created, false otherwise
	 */
	static bool CreateLogMessages( const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category, TArray< TSharedPtr<FLogMessage> >& OutMessages );

	/**
	 * Called after a key is pressed when this widget has keyboard focus
	 *
	 * @param MyGeometry The Geometry of the widget receiving the event
	 * @param  InKeyboardEvent  Keyboard event
	 *
	 * @return  Returns whether the event was handled, along with other possible actions
	 */
	virtual FReply OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent ) OVERRIDE;

protected:

	virtual void Serialize( const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category ) OVERRIDE;

	/** Makes the widget for the log messages in the list view */
	TSharedRef<ITableRow> MakeLogListItemWidget(TSharedPtr<FLogMessage> Message, const TSharedRef<STableViewBase>& OwnerTable);

private:
	/**
	 * Creates a widget for the context menu that can be inserted into a pop-up window
	 *
	 * @return	Widget for this context menu
	 */
	TSharedPtr< SWidget > BuildMenuWidget();

	/**
	 * Called when copy is selected
	 */
	void OnCopy();

	/**
	 * Called to determine whether copy is currently a valid command
	 */
	bool CanCopy() const;

	/**
	 * Called when select all is selected
	 */
	void OnSelectAll();

	/**
	 * Called to determine whether select all is currently a valid command
	 */
	bool CanSelectAll() const;

	/**
	 * Called when select none is selected
	 */
	void OnSelectNone();

	/**
	 * Called to determine whether select none is currently a valid command
	 */
	bool CanSelectNone() const;

	/**
	 * Called when delete all is selected
	 */
	void OnClearLog();

	/**
	 * Called to determine whether delete all is currently a valid command
	 */
	bool CanClearLog() const;

	/** 
	 * Output log commands
	 */
	TSharedPtr<FUICommandList> OutputLogActions;

	/** All log messages stored in this widget for the list view */
	TArray< TSharedPtr<FLogMessage> > Messages;

	/** The list view for showing all log messages. Should be replaced by a full text editor */
	TSharedPtr< SListView< TSharedPtr<FLogMessage> > > MessageListView;

	/** Scroll bar for output log. */
	TSharedPtr< SScrollBar > OutputLogScrollBar;
};
