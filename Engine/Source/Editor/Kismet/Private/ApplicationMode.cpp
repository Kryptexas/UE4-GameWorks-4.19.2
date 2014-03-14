// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintEditorPrivatePCH.h"
#include "WorkflowOrientedApp/WorkflowTabFactory.h"
#include "WorkflowOrientedApp/ApplicationMode.h"

/////////////////////////////////////////////////////
// FApplicationMode

void FApplicationMode::DeactivateMode(TSharedPtr<FTabManager> InTabManager)
{
	// Save the layout to INI
	check(InTabManager.IsValid());
	FLayoutSaveRestore::SaveTheLayout(InTabManager->PersistLayout());
		
	// Unregister the tabs
	/*
	for (int32 Index = 0; Index < AllowableTabs.Num(); ++Index)
	{
		const FName TabName = AllowableTabs[Index];

		PinnedTabManager->UnregisterTabSpawner(TabName);
	}
	*/
}

TSharedRef<FTabManager::FLayout> FApplicationMode::ActivateMode(TSharedPtr<FTabManager> InTabManager)
{
	check(InTabManager.IsValid());
	RegisterTabFactories(InTabManager);

	/*
	for (int32 Index = 0; Index < AllowableTabs.Num(); ++Index)
	{
		const FName TabName = AllowableTabs[Index];

		PinnedTabManager->RegisterTabSpawner(TabName, FOnSpawnTab::CreateSP(this, &FApplicationMode::SpawnTab, TabName));
	}
	*/

	// Try loading the layout from INI
	check(TabLayout.IsValid());
	return FLayoutSaveRestore::LoadUserConfigVersionOf(TabLayout.ToSharedRef());
}
