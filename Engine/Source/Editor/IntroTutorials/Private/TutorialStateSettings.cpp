// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "IntroTutorialsPrivatePCH.h"
#include "TutorialStateSettings.h"

UTutorialStateSettings::UTutorialStateSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UTutorialStateSettings::PostInitProperties()
{
	Super::PostInitProperties();

	for (const auto& Progress : TutorialsProgress)
	{
		TSubclassOf<UEditorTutorial> TutorialClass = LoadClass<UEditorTutorial>(NULL, *Progress.Tutorial.AssetLongPathname, NULL, LOAD_None, NULL);
		if(TutorialClass != nullptr)
		{
			UEditorTutorial* Tutorial = TutorialClass->GetDefaultObject<UEditorTutorial>();
			ProgressMap.Add(Tutorial, Progress);
		}
	}
}

int32 UTutorialStateSettings::GetProgress(UEditorTutorial* InTutorial, bool& bOutHaveSeenTutorial) const
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

bool UTutorialStateSettings::HaveSeenTutorial(UEditorTutorial* InTutorial) const
{
	return ProgressMap.Find(InTutorial) != nullptr;
}

bool UTutorialStateSettings::HaveCompletedTutorial(UEditorTutorial* InTutorial) const
{
	const FTutorialProgress* FoundProgress = ProgressMap.Find(InTutorial);
	if (FoundProgress != nullptr)
	{
		return FoundProgress->CurrentStage >= InTutorial->Stages.Num() - 1;
	}
	return false;
}

void UTutorialStateSettings::RecordProgress(UEditorTutorial* InTutorial, int32 CurrentStage)
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
			Progress.bUserDismissed = false;
			Progress.bUserDismissedThisSession = false;

			ProgressMap.Add(InTutorial, Progress);
		}
	}
}

void UTutorialStateSettings::DismissTutorial(UEditorTutorial* InTutorial, bool bDismissAcrossSessions)
{
	if (InTutorial != nullptr)
	{
		FTutorialProgress* FoundProgress = ProgressMap.Find(InTutorial);
		if (FoundProgress != nullptr)
		{
			FoundProgress->bUserDismissed = bDismissAcrossSessions;
			FoundProgress->bUserDismissedThisSession = true;
		}
		else
		{
			FTutorialProgress Progress;
			Progress.Tutorial = FStringClassReference(InTutorial->GetClass());
			Progress.CurrentStage = 0;
			Progress.bUserDismissed = bDismissAcrossSessions;
			Progress.bUserDismissedThisSession = true;

			ProgressMap.Add(InTutorial, Progress);
		}
	}
}

bool UTutorialStateSettings::IsTutorialDismissed(UEditorTutorial* InTutorial) const
{
	if (GetMutableDefault<UTutorialStateSettings>()->AreAllTutorialsDismissed())
	{
		return true;
	}

	const FTutorialProgress* FoundProgress = ProgressMap.Find(InTutorial);
	if (FoundProgress != nullptr)
	{
		return FoundProgress->bUserDismissed || FoundProgress->bUserDismissedThisSession;
	}
	return false;
}

void UTutorialStateSettings::DismissAllTutorials()
{
	bDismissedAllTutorials = true;
}

bool UTutorialStateSettings::AreAllTutorialsDismissed()
{
	return bDismissedAllTutorials;
}

void UTutorialStateSettings::SaveProgress()
{
	TutorialsProgress.Empty();

	for(const auto& ProgressMapEntry : ProgressMap)
	{
		TutorialsProgress.Add(ProgressMapEntry.Value);
	}

	SaveConfig();
}

void UTutorialStateSettings::ClearProgress()
{
	ProgressMap.Empty();
	TutorialsProgress.Empty();
	bDismissedAllTutorials = false;

	SaveConfig();
}

void UTutorialStateSettings::SaveConfig(uint64 Flags /*= CPF_Config*/, const TCHAR* Filename /*= nullptr*/, FConfigCacheIni* Config /*= GConfig*/)
{
	// This class saves to an engine config and a per-project config.
	// The engine-level config is there so that the user won't be bothered by the same tutorial in multiple projects.
	// The per-project config is there so that the user won't be bothered when changing engine versions.
	// The only way to see a repeated tutorial is to upgrade engine versions and then start a new project.
	Super::SaveConfig(Flags, Filename, Config);
	Super::SaveConfig(Flags, *GEditorPerProjectIni, Config);
}

void UTutorialStateSettings::LoadConfig(UClass* ConfigClass /*= nullptr*/, const TCHAR* Filename /*= nullptr*/, uint32 PropagationFlags /*= UE4::LCPF_None*/, class UProperty* PropertyToLoad /*= NULL*/)
{
	// This will trigger the post-load-config callback twice, but this class doesn't use it.
	// We load from both engine and project so that we'll catch leftover project settings after upgrading the engine, and so that starting a new project doesn't miss the engine settings.
	Super::LoadConfig(ConfigClass, *GEditorPerProjectIni, PropagationFlags, PropertyToLoad);
	Super::LoadConfig(ConfigClass, Filename, PropagationFlags, PropertyToLoad);
}
