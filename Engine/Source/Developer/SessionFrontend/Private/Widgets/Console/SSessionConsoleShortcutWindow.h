// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionConsoleShortcutWindow.h: Declares the SSessionConsoleShortcutWindow class.
=============================================================================*/

#pragma once


struct FConsoleShortcutData
{
	/**
	 * Name of the shortcut
	 */
	FString Name;

	/**
	 * Commandline for the shortcut
	 */
	FString Command;
};

/**
 * Implements the console filter bar widget.
 */
class SSessionConsoleShortcutWindow
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SSessionConsoleShortcutWindow) { }

		/**
		 * Called when the filter settings have changed.
		 */
		SLATE_EVENT(FOnSessionConsoleCommandSubmitted, OnCommandSubmitted)

	SLATE_END_ARGS()

public:

	/**
	 * Construct this widget
	 *
	 * @param InArgs - The declaration data for this widget.
	 */
	void Construct( const FArguments& InArgs );


	/**
	 * Adds a new shortcut to the list of commands and saves the list
	 */
	void AddShortcut(const FString& InName, const FString& InCommandString);


private:

	/**
	 * Adds a new shortcut to the list of commands
	 */
	void AddShortcutInternal(const FString& InName, const FString& InCommandString);

	/** Generates a row widget for a shortcut */
	TSharedRef<ITableRow> OnGenerateWidgetForShortcut(TSharedPtr<FConsoleShortcutData> InItem, const TSharedRef<STableViewBase>& OwnerTable );

	/**
	 * Callback for when a shortcut is executed
	 */
	FReply ExecuteShortcut( TSharedPtr<FConsoleShortcutData> InShortcut);

	/**
	 * Callback for when a shortcut should be removed
	 */
	void RemoveShortcut( TSharedPtr<FConsoleShortcutData> InShortcut );

	/**
	 * Callback for when a shortcut name or command is being edited
	 */
	void EditShortcut( TSharedPtr<FConsoleShortcutData> InShortcut, bool bInEditCommand, FString InPromptTitle );

	/**
	 * Callback for committing changes to command
	 */
	void OnShortcutEditCommitted(const FText& CommandText, ETextCommit::Type CommitInfo);

	/**
	 * Rebuild UI or hide of there are no entries
	 */
	void RebuildUI();

	/**
	 * Loads commands from save file
	 */
	void LoadShortcuts();

	/**
	 * Saves commands to save file
	 */
	void SaveShortcuts() const;

	/**
	 * Gets the filename where the shortcuts are saved/loaded from
	 */
	FString GetShortcutFilename() const;

private:

	// Holds a delegate that is executed when a command is submitted.
	FOnSessionConsoleCommandSubmitted OnCommandSubmitted;

	/** List of all commands that are currently supported by shortcuts */
	TArray< TSharedPtr<FConsoleShortcutData> > Shortcuts;

	/** The list view for showing all commands */
	TSharedPtr< SListView< TSharedPtr<FConsoleShortcutData> > > ShortcutListView;

	/** Reference to owner of the current popup */
	TSharedPtr<class SWindow> NameEntryPopupWindow;

	/** Reference to owner of the current popup */
	TSharedPtr<FConsoleShortcutData> EditedShortcut;

	/** Whether to edit the name or command */
	bool bEditCommand;
};
