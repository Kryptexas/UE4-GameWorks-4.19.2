// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "IntroTutorialsPrivatePCH.h"
#include "EditorTutorialSettings.h"

UEditorTutorialSettings::UEditorTutorialSettings(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{

}

void UEditorTutorialSettings::PostInitProperties()
{
	Super::PostInitProperties();

	for(const auto& Progress : TutorialsProgress)
	{
		TSubclassOf<UEditorTutorial> TutorialClass = LoadClass<UEditorTutorial>(NULL, *Progress.Tutorial.AssetLongPathname, NULL, LOAD_None, NULL);
		if(TutorialClass != nullptr)
		{
			UEditorTutorial* Tutorial = TutorialClass->GetDefaultObject<UEditorTutorial>();
			ProgressMap.Add(Tutorial, Progress);
		}
	}
}

int32 UEditorTutorialSettings::GetProgress(UEditorTutorial* InTutorial, bool& bOutHaveSeenTutorial) const
{
	const FTutorialProgress* FoundProgress = ProgressMap.Find(InTutorial);
	if(FoundProgress != nullptr)
	{
		bOutHaveSeenTutorial = true;
		return FoundProgress->CurrentStage;
	}

	bOutHaveSeenTutorial = false;
	return 0;
}

bool UEditorTutorialSettings::HaveSeenTutorial(UEditorTutorial* InTutorial) const
{
	return ProgressMap.Find(InTutorial) != nullptr;
}

void UEditorTutorialSettings::RecordProgress(UEditorTutorial* InTutorial, int32 CurrentStage)
{
	if(InTutorial != nullptr)
	{
		FTutorialProgress* FoundProgress = ProgressMap.Find(InTutorial);
		if(FoundProgress != nullptr)
		{
			FoundProgress->CurrentStage = FMath::Max(FoundProgress->CurrentStage, CurrentStage);
		}
		else
		{
			FTutorialProgress Progress;
			Progress.Tutorial = FStringClassReference(InTutorial->GetClass());
			Progress.CurrentStage = CurrentStage;

			ProgressMap.Add(InTutorial, Progress);
		}
	}
}

void UEditorTutorialSettings::SaveProgress()
{
	TutorialsProgress.Empty();

	for(const auto& ProgressMapEntry : ProgressMap)
	{
		TutorialsProgress.Add(ProgressMapEntry.Value);
	}

	SaveConfig();
}