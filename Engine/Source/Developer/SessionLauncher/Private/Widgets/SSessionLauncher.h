// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionLauncher.h: Declares the SSessionLauncher class.
=============================================================================*/

#pragma once


/**
 * Implements a Slate widget for the launcher user interface.
 */
class SSessionLauncher
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SSessionLauncher) { }

		/** Exposes a delegate to be invoked when the launcher has closed. */
		SLATE_EVENT(FSimpleDelegate, OnClosed)

	SLATE_END_ARGS()

public:

	/**
	 * Destructor.
	 */
	~SSessionLauncher( );

public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs - The Slate argument list.
	 * @param ConstructUnderMajorTab - The major tab which will contain the session front-end.
	 * @param ConstructUnderWindow - The window in which this widget is being constructed.
	 * @param InModel - The view model to use.
	 */
	void Construct( const FArguments& InArgs, const TSharedRef<SDockTab>& ConstructUnderMajorTab, const TSharedPtr<SWindow>& ConstructUnderWindow, const FSessionLauncherModelRef& InModel );

	void ShowWidget(int32 Index);

protected:

	/**
	 * Launches the selected profile.
	 */
	void LaunchSelectedProfile( );

private:

	// Callback for clicking the 'Cancel' button.
	FReply HandleCancelButtonClicked( );

	// Callback for determining the visibility of the 'Cancel' button.
	EVisibility HandleCancelButtonVisibility( ) const;

	// Callback for determining the visibility of the 'Canceling' throbber.
	EVisibility HandleCancelingThrobberVisibility( ) const;

	// Callback for clicking the 'Done' button.
	FReply HandleDoneButtonClicked( );

	// Callback for determining the visibility of the 'Done' button.
	EVisibility HandleDoneButtonVisibility( ) const;

	// Callback for clicking the 'Edit' button.
	FReply HandleEditButtonClicked( );

	// Callback for determining the visibility of the 'Edit' button.
	EVisibility HandleEditButtonVisibility( ) const;

	// Callback for clicking the 'Finish' button.
	FReply HandleFinishButtonClicked( );

	// Callback for determining the enabled state of the 'Finish' button.
	bool HandleFinishButtonIsEnabled( ) const;

	// Callback for determining the visibility of the 'Finish' button.
	EVisibility HandleFinishButtonVisibility( ) const;

	// Callback for when the owner tab's visual state is being persisted.
	void HandleMajorTabPersistVisualState( );

	// Callback for clicking the 'Preview' button.
	FReply HandlePreviewButtonClicked( );

	// Callback for determining the visibility of the 'Preview' button.
	EVisibility HandlePreviewButtonVisibility( ) const;

	// Callback for determining whether the preview page can be shown.
	bool HandlePreviewPageCanShow( ) const;

	// Callback for changing the selected profile in the profile manager.
	void HandleProfileManagerProfileSelected( const ILauncherProfilePtr& SelectedProfile, const ILauncherProfilePtr& PreviousProfile );

	// Callback for determining the enabled state of the profile selector.
	bool HandleProfileSelectorIsEnabled( ) const;

private:

	// Holds the currently selected task.
	ESessionLauncherTasks::Type CurrentTask;

	// Holds the current launcher worker, if any.
	ILauncherWorkerPtr LauncherWorker;

	// Holds a pointer to the view model.
	FSessionLauncherModelPtr Model;

	// Holds the progress panel.
	TSharedPtr<SSessionLauncherProgress> ProgressPanel;

	// Holds the toolbar widget.
	TSharedPtr<SSessionLauncherToolbar> Toolbar;

	// Holds the widget switcher.
	TSharedPtr<SWidgetSwitcher> WidgetSwitcher;

	// Holds the wizard widget.
	TSharedPtr<SWizard> Wizard;
};
