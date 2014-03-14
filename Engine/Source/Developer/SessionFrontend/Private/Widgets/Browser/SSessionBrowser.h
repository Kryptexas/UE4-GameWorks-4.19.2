// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionBrowser.h: Declares the SSessionBrowser class.
=============================================================================*/

#pragma once


/**
 * Implements a Slate widget for browsing active game sessions.
 */
class SSessionBrowser
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SSessionBrowser) { }
	SLATE_END_ARGS()

public:

	/**
	 * Destructor.
	 */
	~SSessionBrowser( );

public:

	/**
	 * Construct this widget.
	 *
	 * @param InArgs - The declaration data for this widget.
	 * @param InSessionManager - The session manager to use.
	 */
	void Construct( const FArguments& InArgs, ISessionManagerRef InSessionManager );

public:

	// Begin SWidget interface

	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) OVERRIDE;

	// End SWidget interface

protected:

	/**
	 * Filters the session list.
	 */
	void FilterSessions( );

	/**
	 * Reloads the sessions list.
	 */
	void ReloadSessions( );

private:

	// Callback for generating a context menu in the session list view.
	TSharedPtr<SWidget> HandleSessionListViewContextMenuOpening( );

	// Callback for creating a row widget in the session list view.
	TSharedRef<ITableRow> HandleSessionListViewGenerateRow( ISessionInfoPtr Item, const TSharedRef<STableViewBase>& OwnerTable );

	// Handles changing the selection in the session tree.
	void HandleSessionListViewSelectionChanged( ISessionInfoPtr Item, ESelectInfo::Type SelectInfo );

	// Handles changing the selected session in the session manager.
	void HandleSessionManagerSelectedSessionChanged( const ISessionInfoPtr& SelectedSession );

	// Handles updating the session list in the session manager.
	void HandleSessionManagerSessionsUpdated( );

private:

	// Holds an unfiltered list of available sessions.
	TArray<ISessionInfoPtr> AvailableSessions;

	// Holds the command bar.
	TSharedPtr<SSessionBrowserCommandBar> CommandBar;

	// Holds the instance list view.
	TSharedPtr<SSessionInstanceList> InstanceListView;

	// Holds the filtered list of sessions to be displayed.
	TArray<ISessionInfoPtr> SessionList;

	// Holds the session list view.
	TSharedPtr<SListView<ISessionInfoPtr> > SessionListView;

	// Holds a reference to the session manager.
	ISessionManagerPtr SessionManager;

	// Holds the splitter.
	TSharedPtr<SSplitter> Splitter;
};
