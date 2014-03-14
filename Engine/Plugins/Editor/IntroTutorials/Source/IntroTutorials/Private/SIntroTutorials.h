// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "InteractiveTutorials.h"
#include "IDocumentation.h"
#include "IDocumentationPage.h"

/** Delegate fired when the user presses the home button */
DECLARE_DELEGATE( FOnGoHome );

/** Delegate fired when the user presses the next button at the end of a tutorial */
DECLARE_DELEGATE_RetVal_OneParam( FString, FOnGotoNextTutorial, const FString& /*InCurrentPage*/ );

/**
 * The widget which holds the introductory engine tutorials
 */
class SIntroTutorials : public SCompoundWidget
{
	SLATE_BEGIN_ARGS( SIntroTutorials ) {}
		SLATE_ARGUMENT(TSharedPtr<SWindow>, ParentWindow)

		/** Whether to show the home button */
		SLATE_ATTRIBUTE(EVisibility, HomeButtonVisibility)

		/** Delegate fired when the user presses the next button at the end of a tutorial */
		SLATE_ARGUMENT(FOnGotoNextTutorial, OnGotoNextTutorial)
	SLATE_END_ARGS()


	/** Widget constructor */
	void Construct( const FArguments& Args );

	/** 
	 * Attempts to change the page to the specified path 
	 * @param	Path				The path to the page to be displayed
	 */
	void ChangePage(const FString& Path);

	FString GetCurrentTutorialName() const;
	FString GetCurrentExcerptTitle() const;
	int32 GetCurrentExcerptIndex() const;
	FString GetCurrentPagePath() const;
	float GetCurrentPageElapsedTime() const;

	void SetOnGoHome(const FOnGoHome& InDelegate);
	
private:
	/** Are we displaying a 'home' style page */
	bool IsHomeStyle() const;

	void SetCurrentExcerpt(int32 NewExcerptIdx);

	/** Sets the content area and any additional necessary calls */
	void SetContentArea();

	/** Play tutorial dialogue */
	void PlayDialogue(UDialogueWave* InDialogueWave);
	/** Play a sound specified by its path */
	void PlaySound(const FString& WavePath);

	/** Changes the page we are currently on, cleaning up appropriately */
	void GotoPreviousPage();
	void GotoNextPage();
	
	/** Called when a trigger event is completed */
	void TriggerCompleted();

	/** UI Callbacks */
	EVisibility GetNavigationVisibility() const;
	EVisibility GetBackButtonVisibility() const;
	EVisibility GetNextButtonVisibility() const;
	FText GetNextButtonText() const;
	FText GetBackButtonText() const;
	FString GetTimeRemaining() const;
	FReply OnHomeClicked();
	FReply OnPreviousClicked();
	bool OnPreviousIsEnabled() const;
	FReply OnNextClicked();
	bool OnNextIsEnabled() const;
	EVisibility GetContentVisibility() const;
	EVisibility GetHomeContentVisibility() const;
	TOptional<float> GetProgress() const;
	FString GetProgressString() const;
	const FSlateBrush* GetContentAreaBackground() const;

	bool IsLastPage() const;
	bool IsFirstPage() const;
	
private:
	/** Parent window that houses this widget */
	TWeakPtr<SWindow> ParentWindowPtr;

	/** The section where home page content is placed */
	TSharedPtr<SBorder> HomeContentArea;

	/** The section where new content is placed */
	TSharedPtr<SBorder> ContentArea;

	/** The index of the current section we're at */
	int32 CurrentExcerptIndex;
	
	/** A list of excerpts for the currently loaded tutorial */
	TArray<struct FExcerpt> Excerpts;

	/** The handler of all things interactive for the tutorials */
	TSharedPtr<class FInteractiveTutorials> InteractiveTutorials;

	/** Whether or not the current section will not allow progression without interaction */
	bool bCurrentSectionIsInteractive;

	/** The path of the current loaded udn file */
	FString CurrentPagePath;

	/** The current documentation page */
	TSharedPtr< class IDocumentationPage > CurrentPage;

	double CurrentPageStartTime;

	/** The configuration information for the documentation parser */
	TSharedPtr< class FParserConfiguration > ParserConfiguration;

	/** The home path for the root .udn file */
	static const FString HomePath;

	/** Style for the documentation */
	FDocumentationStyle DocumentationStyle;

	/** Context to use for dialogue */
	FDialogueContext TutorialDialogueContext;

	/** Audio component used for playing tutorial audio */
	UAudioComponent* DialogueAudioComponent;

	/** Delegate fired when Home is clicked */
	FOnGoHome OnGoHome;

	/** Whether to show the  home button */
	TAttribute<EVisibility> HomeButtonVisibility;

	/** Delegate fired when the user presses the next button at the end of a tutorial */
	FOnGotoNextTutorial OnGotoNextTutorial;
};

