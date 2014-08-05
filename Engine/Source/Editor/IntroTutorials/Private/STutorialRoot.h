// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class UEditorTutorial;
class SEditorTutorials;

/**
 * The widget which simply monitors windows in its tick function to see if we need to attach
 * a tutorial overlay.
 */
class STutorialRoot : public SCompoundWidget, public FGCObject
{
public:
	SLATE_BEGIN_ARGS( STutorialRoot ) 
	{
		_Visibility = EVisibility::HitTestInvisible;
	}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** SWidget implementation */
	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) override;

	/** FGCObject implementation */
	virtual void AddReferencedObjects( FReferenceCollector& Collector ) override;

	/** Launch the specified tutorial from the specified window */
	void LaunchTutorial(UEditorTutorial* InTutorial, bool bInRestart, TWeakPtr<SWindow> InNavigationWindow);

	/** Summon the browser widget for the specified window */
	void SummonTutorialBrowser(TSharedRef<SWindow> InWindow, const FString& InFilter = TEXT(""));

	/** Reload tutorials that we know about */
	void ReloadTutorials();

private:
	/** Handle when the next button is clicked - forward navigation to other overlays */
	void HandleNextClicked(TWeakPtr<SWindow> InNavigationWindow);

	/** Handle when the back button is clicked - forward navigation to other overlays */
	void HandleBackClicked();

	/** Handle when the home button is clicked - forward navigation to other overlays */
	void HandleHomeClicked();

	/** Handle retrieving the current tutorial */
	UEditorTutorial* HandleGetCurrentTutorial();

	/** Handle retrieving the current tutorial stage */
	int32 HandleGetCurrentTutorialStage();

	/** Got to the previous stage in the current tutorial */
	void GoToPreviousStage();

	/** Got to the next stage in the current tutorial */
	void GoToNextStage(TWeakPtr<SWindow> InNavigationWindow);

	/** Function called on Tick() to check active windows for whether they need an overlay adding */
	void MaybeAddOverlay(TSharedRef<SWindow> InWindow);

private:
	/** Container widgets, inserted into window overlays */
	TMap<TWeakPtr<SWindow>, TWeakPtr<SEditorTutorials>> TutorialWidgets;

	/** Tutorial we are currently viewing */
	UEditorTutorial* CurrentTutorial;

	/** Current stage of tutorial */
	int32 CurrentTutorialStage;
};