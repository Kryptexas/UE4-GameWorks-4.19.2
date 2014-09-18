// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "IntroTutorialsPrivatePCH.h"
#include "SEditorTutorials.h"
#include "STutorialsBrowser.h"
#include "STutorialNavigation.h"
#include "STutorialOverlay.h"
#include "EditorTutorial.h"

#define LOCTEXT_NAMESPACE "TutorialsBrowser"

void SEditorTutorials::Construct(const FArguments& InArgs)
{
	bBrowserVisible = false;
	bIsNavigationWindow = false;
	ParentWindow = InArgs._ParentWindow;
	OnNextClicked = InArgs._OnNextClicked;
	OnBackClicked = InArgs._OnBackClicked;
	OnHomeClicked = InArgs._OnHomeClicked;
	OnCloseClicked = InArgs._OnCloseClicked;
	OnGetCurrentTutorial = InArgs._OnGetCurrentTutorial;
	OnGetCurrentTutorialStage = InArgs._OnGetCurrentTutorialStage;

	TutorialHome = SNew(STutorialsBrowser)
		.Visibility(this, &SEditorTutorials::GetBrowserVisibility)
		.OnClosed(FSimpleDelegate::CreateSP(this, &SEditorTutorials::HandleCloseClicked))
		.OnLaunchTutorial(InArgs._OnLaunchTutorial)
		.ParentWindow(InArgs._ParentWindow);

	NavigationWidget = SNew(STutorialNavigation)
		.Visibility(this, &SEditorTutorials::GetNavigationVisibility)
		.OnBackClicked(FSimpleDelegate::CreateSP(this, &SEditorTutorials::HandleBackClicked))
		.OnHomeClicked(FSimpleDelegate::CreateSP(this, &SEditorTutorials::HandleHomeClicked))
		.OnNextClicked(FSimpleDelegate::CreateSP(this, &SEditorTutorials::HandleNextClicked))
		.IsBackEnabled(this, &SEditorTutorials::IsBackButtonEnabled)
		.IsHomeEnabled(this, &SEditorTutorials::IsHomeButtonEnabled)
		.IsNextEnabled(this, &SEditorTutorials::IsNextButtonEnabled)
		.OnGetProgress(this, &SEditorTutorials::GetProgress)
		.ParentWindow(InArgs._ParentWindow);

	ContentBox = SNew(SHorizontalBox);

	ChildSlot
	[
		SNew(SOverlay)
		+SOverlay::Slot()
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				TutorialHome.ToSharedRef()
			]
		]
		+SOverlay::Slot()
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.FillHeight(0.75f)
			[
				SNew( SSpacer )
			]
			+SVerticalBox::Slot()
			.FillHeight(0.25f)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				NavigationWidget.ToSharedRef()
			]
		]
		+SOverlay::Slot()
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		[
			ContentBox.ToSharedRef()
		]
	];
}

void SEditorTutorials::LaunchTutorial(bool bInIsNavigationWindow, FSimpleDelegate InOnTutorialClosed, FSimpleDelegate InOnTutorialExited)
{
	bIsNavigationWindow = bInIsNavigationWindow;
	OnTutorialClosed = InOnTutorialClosed;
	OnTutorialExited = InOnTutorialExited;

	RebuildCurrentContent();
}


void SEditorTutorials::ShowBrowser(const FString& InFilter)
{
	TutorialHome->SetFilter(InFilter);
	HandleHomeClicked();
}

void SEditorTutorials::HideContent()
{
	HandleHomeClicked();
	bBrowserVisible = false;
	bIsNavigationWindow = false;
}

bool SEditorTutorials::IsNavigationVisible() const
{
	return bIsNavigationWindow;
}

EVisibility SEditorTutorials::GetBrowserVisibility() const
{
	return bBrowserVisible && OnGetCurrentTutorial.Execute() == nullptr ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SEditorTutorials::GetNavigationVisibility() const
{
	return EVisibility::Collapsed;
}

void SEditorTutorials::HandleCloseClicked()
{
	OnCloseClicked.ExecuteIfBound();
	OnTutorialClosed.ExecuteIfBound();

	HandleHomeClicked();
	bBrowserVisible = false;
}

void SEditorTutorials::HandleBackClicked()
{
	// forward to other overlays so they can rebuild their widgets as well
	OnBackClicked.ExecuteIfBound();
}

void SEditorTutorials::HandleHomeClicked()
{
	OnHomeClicked.ExecuteIfBound();

	ContentBox->ClearChildren();
	bBrowserVisible = true;
	TutorialHome->ReloadTutorials();
	OnTutorialExited.ExecuteIfBound();
}

void SEditorTutorials::HandleNextClicked()
{
	// forward to other overlays so they can rebuild their widgets as well
	OnNextClicked.ExecuteIfBound(ParentWindow);
}

bool SEditorTutorials::IsBackButtonEnabled() const
{
	UEditorTutorial* CurrentTutorial = OnGetCurrentTutorial.Execute();
	int32 CurrentTutorialStage = OnGetCurrentTutorialStage.Execute();

	return CurrentTutorial != nullptr && CurrentTutorialStage > 0;
}

bool SEditorTutorials::IsHomeButtonEnabled() const
{
	return true;
}

bool SEditorTutorials::IsNextButtonEnabled() const
{
	UEditorTutorial* CurrentTutorial = OnGetCurrentTutorial.Execute();
	int32 CurrentTutorialStage = OnGetCurrentTutorialStage.Execute();

	const bool bStepsRemaining = CurrentTutorial != nullptr && CurrentTutorialStage + 1 < CurrentTutorial->Stages.Num();
	const bool bNextTutorialAvailable = CurrentTutorial != nullptr && CurrentTutorialStage + 1 >= CurrentTutorial->Stages.Num() && CurrentTutorial->NextTutorial != nullptr;
	return bStepsRemaining || bNextTutorialAvailable;
}

float SEditorTutorials::GetProgress() const
{
	UEditorTutorial* CurrentTutorial = OnGetCurrentTutorial.Execute();
	int32 CurrentTutorialStage = OnGetCurrentTutorialStage.Execute();

	return (CurrentTutorial != nullptr && CurrentTutorial->Stages.Num() > 0) ? (float)(CurrentTutorialStage + 1) / (float)CurrentTutorial->Stages.Num() : 0.0f;
}

void SEditorTutorials::RebuildCurrentContent()
{
	UEditorTutorial* CurrentTutorial = OnGetCurrentTutorial.Execute();
	int32 CurrentTutorialStage = OnGetCurrentTutorialStage.Execute();

	OverlayContent = nullptr;
	ContentBox->ClearChildren();
	if(CurrentTutorial != nullptr && CurrentTutorialStage < CurrentTutorial->Stages.Num())
	{
		ContentBox->AddSlot()
		[
			SAssignNew(OverlayContent, STutorialOverlay, CurrentTutorial, &CurrentTutorial->Stages[CurrentTutorialStage])
			.OnClosed(FSimpleDelegate::CreateSP(this, &SEditorTutorials::HandleCloseClicked))
			.IsStandalone(CurrentTutorial->bIsStandalone)
			.ParentWindow(ParentWindow)
			.AllowNonWidgetContent(bIsNavigationWindow)
			.OnBackClicked(FSimpleDelegate::CreateSP(this, &SEditorTutorials::HandleBackClicked))
			.OnHomeClicked(FSimpleDelegate::CreateSP(this, &SEditorTutorials::HandleHomeClicked))
			.OnNextClicked(FSimpleDelegate::CreateSP(this, &SEditorTutorials::HandleNextClicked))
			.IsBackEnabled(this, &SEditorTutorials::IsBackButtonEnabled)
			.IsHomeEnabled(this, &SEditorTutorials::IsHomeButtonEnabled)
			.IsNextEnabled(this, &SEditorTutorials::IsNextButtonEnabled)
		];
	}
	else
	{
		// create 'empty' overlay, as we may need this for picking visualization
		ContentBox->AddSlot()
		[
			SAssignNew(OverlayContent, STutorialOverlay, nullptr, nullptr)
			.OnClosed(FSimpleDelegate::CreateSP(this, &SEditorTutorials::HandleCloseClicked))
			.IsStandalone(false)
			.ParentWindow(ParentWindow)
			.AllowNonWidgetContent(false)
		];	
	}
}

TSharedPtr<SWindow> SEditorTutorials::GetParentWindow() const
{
	return ParentWindow.IsValid() ? ParentWindow.Pin() : TSharedPtr<SWindow>();
}

#undef LOCTEXT_NAMESPACE