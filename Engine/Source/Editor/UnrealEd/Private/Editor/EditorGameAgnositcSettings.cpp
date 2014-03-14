// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"

UEditorGameAgnosticSettings::UEditorGameAgnosticSettings(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bCopyStarterContentPreference = true;
}

void UEditorGameAgnosticSettings::PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent)
{
	UProperty* PropertyThatChanged = PropertyChangedEvent.Property;

	const FName Name = PropertyThatChanged ? PropertyThatChanged->GetFName() : NAME_None;
	if (Name == FName(TEXT("bLoadTheMostRecentlyLoadedProjectAtStartup")))
	{
		const FString& AutoLoadProjectFileName = IProjectManager::Get().GetAutoLoadProjectFileName();
		if ( GEditor->GetGameAgnosticSettings().bLoadTheMostRecentlyLoadedProjectAtStartup )
		{
			// Form or overwrite the file that is read at load to determine the most recently loaded project file
			FFileHelper::SaveStringToFile(FPaths::GetProjectFilePath(), *AutoLoadProjectFileName);
		}
		else
		{
			// Remove the file. It's possible for bLoadTheMostRecentlyLoadedProjectAtStartup to be set before FPaths::GetProjectFilePath() is valid, so we need to distinguish the two cases.
			IFileManager::Get().Delete(*AutoLoadProjectFileName);
		}
	}

	GEditor->SaveGameAgnosticSettings();
}