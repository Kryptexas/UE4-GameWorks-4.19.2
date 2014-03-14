// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "IntroTutorialsPrivatePCH.h"
#include "InteractiveTutorial.h"
#include "K2Node_TutorialExcerptComplete.h"

#define LOCTEXT_NAMESPACE "IntroTutorials"


void FInteractiveTutorial::SetCurrentExcerpt(const FString& NewExcerpt)
{
	if (CurrentExcerpt != NewExcerpt)
	{
		CurrentExcerpt = NewExcerpt;

		// run the "onbegin" delegate if bound
		FOnBeginExcerptDelegate const* const OnBeginDelegate = BeginExcerptDelegates.Find(NewExcerpt);
		if (OnBeginDelegate && OnBeginDelegate->IsBound())
		{
			OnBeginDelegate->Execute();
		}
	}
}

const FString& FInteractiveTutorial::GetCurrentExcerpt() const
{
	return CurrentExcerpt;
}

bool FInteractiveTutorial::IsExcerptInteractive(const FString& ExcerptName)
{
	if(TriggerExcerpts.Find(ExcerptName) != NULL)
	{
		return true;
	}

	if(UK2Node_TutorialExcerptComplete::IsExcerptInteractive(ExcerptName))
	{
		return true;
	}

	return false;
}

bool FInteractiveTutorial::CanManuallyAdvanceExcerpt(const FString& ExcerptName) const
{
	if ( ExcerptsWithDisabledAdvancement.Find(ExcerptName) != INDEX_NONE )
	{
		// now see if it's already been triggered
		if (CompletedExcerpts.Find(ExcerptName) == INDEX_NONE)
		{
			// manual advancement disabled and untriggered, disallow advancement
			return false;
		}
	}

	// allow in all other cases
	return true;
}

bool FInteractiveTutorial::CanManuallyReverseExcerpt(const FString& ExcerptName) const
{
	// always allow going back
	return true;
}

TSharedPtr<FBaseTriggerDelegate> FInteractiveTutorial::FindCurrentTrigger() const
{
	const TSharedPtr<FBaseTriggerDelegate>* Trigger = TriggerExcerpts.Find(CurrentExcerpt);
	if(Trigger != NULL)
	{
		return *Trigger;
	}

	return TSharedPtr<FBaseTriggerDelegate>();
}

void FInteractiveTutorial::AddHighlightWidget(const FString& InExcerptName, const FName& InWidgetName)
{
	ExcerptWidgets.Add(InExcerptName, InWidgetName);
}

bool FInteractiveTutorial::IsWidgetHighlighted(const FName& InWidgetName) const
{
	TArray<FName> WidgetNames;
	ExcerptWidgets.MultiFind(CurrentExcerpt, WidgetNames);
	for(auto It(WidgetNames.CreateConstIterator()); It; ++It)
	{
		if(*It == InWidgetName)
		{
			return true;
		}
	}

	return false;
}

UDialogueWave* FInteractiveTutorial::FindDialogueAudio(const FString& ExcerptName)
{
	UDialogueWave* Wave = NULL;
	// See if we have dialogue for this excerpt
	FString* DialogueNamePtr = ExcerptDialogue.Find(ExcerptName);
	if(DialogueNamePtr != NULL)
	{
		FString DialogueName = *DialogueNamePtr;
		// Try FindObject first
		Wave = FindObject<UDialogueWave>(ANY_PACKAGE, *DialogueName);
		if(Wave == NULL)
		{
			// If not, load it now
			Wave = LoadObject<UDialogueWave>(NULL, *DialogueName);
			// If we tried to load and failed, pop up a warning
			if(Wave == NULL)
			{
				FNotificationInfo Info( FText::Format( LOCTEXT("DialogueLoadFailed", "Unable to load dialogue audio \"{0}\"."), FText::FromString(*DialogueName) ) );
				Info.ExpireDuration = 3.0f;
				Info.bUseLargeFont = false;
				TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
				if ( Notification.IsValid() )
				{
					Notification->SetCompletionState( SNotificationItem::CS_Fail );
				}
			}
		}
	}
	return Wave;
}


const FShouldSkipExcerptDelegate* FInteractiveTutorial::FindSkipDelegate(const FString& InExcerptName) const
{
	return ExcerptSkipDelegates.Find(InExcerptName);
}

void FInteractiveTutorial::OnExcerptCompleted(const FString& InExcerpt)
{
	CompletedExcerpts.AddUnique(InExcerpt);
}


#undef LOCTEXT_NAMESPACE
