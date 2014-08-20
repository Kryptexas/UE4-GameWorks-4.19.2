// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "STutorialsBrowser.h"

class STutorialsBrowser;
class STutorialOverlay;

/** Delegate fired when next button is clicked */
DECLARE_DELEGATE_OneParam(FOnNextClicked, TWeakPtr<SWindow> /*InNavigationWindow*/);

/** Delegate fired to retrieve current tutorial */
DECLARE_DELEGATE_RetVal(UEditorTutorial*, FOnGetCurrentTutorial);

/** Delegate fired to retrieve current tutorial stage */
DECLARE_DELEGATE_RetVal(int32, FOnGetCurrentTutorialStage);

/**
 * Container widget for all tutorial-related widgets
 */
class SEditorTutorials : public SCompoundWidget
{
	SLATE_BEGIN_ARGS( SEditorTutorials )
	{
		_Visibility = EVisibility::SelfHitTestInvisible;
	}

	SLATE_ARGUMENT(FSimpleDelegate, OnClosed)
	SLATE_ARGUMENT(TWeakPtr<SWindow>, ParentWindow)
	SLATE_ARGUMENT(FOnNextClicked, OnNextClicked)
	SLATE_ARGUMENT(FSimpleDelegate, OnBackClicked)
	SLATE_ARGUMENT(FSimpleDelegate, OnHomeClicked)
	SLATE_ARGUMENT(FOnGetCurrentTutorial, OnGetCurrentTutorial)
	SLATE_ARGUMENT(FOnGetCurrentTutorialStage, OnGetCurrentTutorialStage)
	SLATE_ARGUMENT(FOnLaunchTutorial, OnLaunchTutorial)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Launch a tutorial - will interrogate parent to get the tutorial data to display */
	void LaunchTutorial(bool bInIsNavigationWindow);

	/** Show the tutorials browser in this window */
	void ShowBrowser(const FString& InFilter);

	/** Get parent window for this widget */
	TSharedPtr<SWindow> GetParentWindow() const;

	/** Rebuild content according to the current tutorial state */
	void RebuildCurrentContent();
	
	/** @return whether the navigation controls are currently visible */
	bool IsNavigationVisible() const;

private:
	/** Handle whether we should display the browser */
	EVisibility GetBrowserVisibility() const;

	/** Handle whether we should display the navigation buttons */
	EVisibility GetNavigationVisibility() const;

	/** Handle hitting the close button on the browser or a standalone widget */
	void HandleCloseClicked();

	/** Handle hitting the home button in the navigation window */
	void HandleHomeClicked();

	/** Handle hitting the back button in the navigation window */
	void HandleBackClicked();

	/** Handle hitting the next button in the navigation window */
	void HandleNextClicked();

	/** Get enabled state of the back button */
	bool IsBackButtonEnabled() const;

	/** Get enabled state of the home button */
	bool IsHomeButtonEnabled() const;

	/** Get enabled state of the next button */
	bool IsNextButtonEnabled() const;

	/** Get the current progress to display in the progress bar */
	float GetProgress() const;

private:
	/** The root widget of the tutorial home page */
	TSharedPtr<STutorialsBrowser> TutorialHome;

	/** Navigation widget */
	TSharedPtr<SWidget> NavigationWidget;

	/** Box that contains varied content for current tutorial */
	TSharedPtr<SHorizontalBox> ContentBox;

	/** Content widget for current tutorial */
	TSharedPtr<STutorialOverlay> OverlayContent;

	/** Window that contains this widget */
	TWeakPtr<SWindow> ParentWindow;

	/** Whether the browser is visible */
	bool bBrowserVisible;

	/** Whether we should display navigation */
	bool bShowNavigation;

	/** Delegate fired when next button is clicked */
	FOnNextClicked OnNextClicked;

	/** Delegate fired when back button is clicked */
	FSimpleDelegate OnBackClicked;

	/** Delegate fired when home button is clicked */
	FSimpleDelegate OnHomeClicked;

	/** Delegate fired to retrive the current tutorial */
	FOnGetCurrentTutorial OnGetCurrentTutorial;

	/** Delegate fired to retrive the current tutorial stage */
	FOnGetCurrentTutorialStage OnGetCurrentTutorialStage;
};