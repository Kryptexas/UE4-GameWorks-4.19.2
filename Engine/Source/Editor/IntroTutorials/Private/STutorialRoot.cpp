// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "IntroTutorialsPrivatePCH.h"
#include "STutorialRoot.h"
#include "SEditorTutorials.h"
#include "EditorTutorialSettings.h"

#define LOCTEXT_NAMESPACE "STutorialRoot"

void STutorialRoot::Construct(const FArguments& InArgs)
{
	CurrentTutorial = nullptr;
	CurrentTutorialStage = 0;

	ChildSlot
	[
		SNullWidget::NullWidget
	];
}

void STutorialRoot::MaybeAddOverlay(TSharedRef<SWindow> InWindow)
{
	if(InWindow->HasOverlay())
	{
		// check we dont already have a widget overlay for this window
		TWeakPtr<SEditorTutorials>* FoundWidget = TutorialWidgets.Find(InWindow);
		if(FoundWidget == nullptr)
		{
			TSharedPtr<SEditorTutorials> TutorialWidget = nullptr;
			InWindow->AddOverlaySlot()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.VAlign(VAlign_Fill)
				.HAlign(HAlign_Fill)
				[
					SAssignNew(TutorialWidget, SEditorTutorials)
					.ParentWindow(InWindow)
					.OnNextClicked(FOnNextClicked::CreateSP(this, &STutorialRoot::HandleNextClicked))
					.OnBackClicked(FSimpleDelegate::CreateSP(this, &STutorialRoot::HandleBackClicked))
					.OnHomeClicked(FSimpleDelegate::CreateSP(this, &STutorialRoot::HandleHomeClicked))
					.OnGetCurrentTutorial(FOnGetCurrentTutorial::CreateSP(this, &STutorialRoot::HandleGetCurrentTutorial))
					.OnGetCurrentTutorialStage(FOnGetCurrentTutorialStage::CreateSP(this, &STutorialRoot::HandleGetCurrentTutorialStage))
					.OnLaunchTutorial(FOnLaunchTutorial::CreateSP(this, &STutorialRoot::LaunchTutorial))
				]
			];

			FoundWidget = &TutorialWidgets.Add(InWindow, TutorialWidget);

			FoundWidget->Pin()->RebuildCurrentContent();
		}
	}

	TArray<TSharedRef<SWindow>> ChildWindows = InWindow->GetChildWindows();
	for(auto& ChildWindow : ChildWindows)
	{
		MaybeAddOverlay(ChildWindow);
	}
}

void STutorialRoot::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	TArray<TSharedRef<SWindow>> Windows = FSlateApplication::Get().GetInteractiveTopLevelWindows();
	for(auto& Window : Windows)
	{
		MaybeAddOverlay(Window);
	}

	if (CurrentTutorial != nullptr)
	{
		CurrentTutorial->HandleTickCurrentStage(CurrentTutorial->Stages[CurrentTutorialStage].Name);
	}
}

void STutorialRoot::SummonTutorialBrowser(TSharedRef<SWindow> InWindow, const FString& InFilter)
{
	TWeakPtr<SEditorTutorials>* FoundWidget = TutorialWidgets.Find(InWindow);
	if(FoundWidget != nullptr && FoundWidget->IsValid())
	{
		FoundWidget->Pin()->ShowBrowser(InFilter);
	}
}

void STutorialRoot::LaunchTutorial(UEditorTutorial* InTutorial, bool bInRestart, TWeakPtr<SWindow> InNavigationWindow)
{
	if(InTutorial != nullptr)
	{
		CurrentTutorial = InTutorial;

		bool bHaveSeenTutorial = false;
		CurrentTutorialStage = bInRestart ? 0 : GetDefault<UEditorTutorialSettings>()->GetProgress(CurrentTutorial, bHaveSeenTutorial);

		// launch tutorial for all windows we wrap - any tutorial can display over any window
		for(auto& TutorialWidget : TutorialWidgets)
		{
			if(TutorialWidget.Value.IsValid())
			{
				bool bIsNavigationWindow = false;
				if (!InNavigationWindow.IsValid())
				{
					bIsNavigationWindow = TutorialWidget.Value.Pin()->IsNavigationVisible();
				}
				else
				{
					bIsNavigationWindow = (TutorialWidget.Value.Pin()->GetParentWindow() == InNavigationWindow.Pin());
				}
				TutorialWidget.Value.Pin()->LaunchTutorial(bIsNavigationWindow);
			}
		}
	}
}

void STutorialRoot::HandleNextClicked(TWeakPtr<SWindow> InNavigationWindow)
{
	GoToNextStage(InNavigationWindow);

	for(auto& TutorialWidget : TutorialWidgets)
	{
		if(TutorialWidget.Value.IsValid())
		{
			TSharedPtr<SEditorTutorials> PinnedTutorialWidget = TutorialWidget.Value.Pin();
			PinnedTutorialWidget->RebuildCurrentContent();
		}
	}
}

void STutorialRoot::HandleBackClicked()
{
	GoToPreviousStage();

	for(auto& TutorialWidget : TutorialWidgets)
	{
		if(TutorialWidget.Value.IsValid())
		{
			TSharedPtr<SEditorTutorials> PinnedTutorialWidget = TutorialWidget.Value.Pin();
			PinnedTutorialWidget->RebuildCurrentContent();
		}
	}
}

void STutorialRoot::HandleHomeClicked()
{
	if(CurrentTutorial != nullptr)
	{
		GetMutableDefault<UEditorTutorialSettings>()->RecordProgress(CurrentTutorial, CurrentTutorialStage);
		GetMutableDefault<UEditorTutorialSettings>()->SaveProgress();
	}

	CurrentTutorial = nullptr;
	CurrentTutorialStage = 0;

	for(auto& TutorialWidget : TutorialWidgets)
	{
		if(TutorialWidget.Value.IsValid())
		{
			TSharedPtr<SEditorTutorials> PinnedTutorialWidget = TutorialWidget.Value.Pin();
			PinnedTutorialWidget->RebuildCurrentContent();
		}
	}
}

UEditorTutorial* STutorialRoot::HandleGetCurrentTutorial()
{
	return CurrentTutorial;
}

int32 STutorialRoot::HandleGetCurrentTutorialStage()
{
	return CurrentTutorialStage;
}

void STutorialRoot::AddReferencedObjects( FReferenceCollector& Collector )
{
	if(CurrentTutorial != nullptr)
	{
		Collector.AddReferencedObject(CurrentTutorial);
	}
}

void STutorialRoot::GoToPreviousStage()
{
	if(CurrentTutorial != nullptr)
	{
		int32 PreviousTutorialStage = CurrentTutorialStage;
		if (CurrentTutorialStage > 0)
		{
			CurrentTutorial->HandleTutorialStageEnded(CurrentTutorial->Stages[CurrentTutorialStage].Name);
		}

		CurrentTutorialStage = FMath::Max(0, CurrentTutorialStage - 1);

		if (PreviousTutorialStage != CurrentTutorialStage)
		{
			CurrentTutorial->HandleTutorialStageStarted(CurrentTutorial->Stages[CurrentTutorialStage].Name);
		}
	}
}

void STutorialRoot::GoToNextStage(TWeakPtr<SWindow> InNavigationWindow)
{
	if(CurrentTutorial != nullptr)
	{
		UEditorTutorial* PreviousTutorial = CurrentTutorial;
		int32 PreviousTutorialStage = CurrentTutorialStage;
		CurrentTutorial->HandleTutorialStageEnded(CurrentTutorial->Stages[CurrentTutorialStage].Name);

		if(CurrentTutorialStage + 1 >= CurrentTutorial->Stages.Num() && FName(*CurrentTutorial->NextTutorial.AssetLongPathname) != NAME_None)
		{
			TSubclassOf<UEditorTutorial> NextTutorialClass = LoadClass<UEditorTutorial>(NULL, *CurrentTutorial->NextTutorial.AssetLongPathname, NULL, LOAD_None, NULL);
			if(NextTutorialClass != nullptr)
			{
				LaunchTutorial(NextTutorialClass->GetDefaultObject<UEditorTutorial>(), true, InNavigationWindow);
			}
			else
			{
				FSlateNotificationManager::Get().AddNotification(FNotificationInfo(FText::Format(LOCTEXT("TutorialNotFound", "Could not start next tutorial {0}"), FText::FromString(CurrentTutorial->NextTutorial.AssetLongPathname))));
			}
		}
		else
		{
			CurrentTutorialStage = FMath::Min(CurrentTutorialStage + 1, CurrentTutorial->Stages.Num() - 1);
			GetMutableDefault<UEditorTutorialSettings>()->RecordProgress(CurrentTutorial, CurrentTutorialStage);
		}

		if (CurrentTutorial != nullptr && (CurrentTutorial != PreviousTutorial || CurrentTutorialStage != PreviousTutorialStage))
		{
			CurrentTutorial->HandleTutorialStageStarted(CurrentTutorial->Stages[CurrentTutorialStage].Name);
		}
	}
}

#undef LOCTEXT_NAMESPACE