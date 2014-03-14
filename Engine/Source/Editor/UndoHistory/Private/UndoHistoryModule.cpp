// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	UndoHistoryModule.cpp: Implements the FUndoHistoryModule class.
=============================================================================*/

#include "UndoHistoryPrivatePCH.h"
#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructureModule.h"


#define LOCTEXT_NAMESPACE "FUndoHistoryModule"


static const FName UndoHistoryTabName("UndoHistory");


/**
 * Implements the UndoHistory module.
 */
class FUndoHistoryModule
	: public IUndoHistoryModule
{
public:

	// Begin IModuleInterface interface

	virtual void StartupModule( ) OVERRIDE
	{
		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(UndoHistoryTabName, FOnSpawnTab::CreateRaw(this, &FUndoHistoryModule::HandleSpawnSettingsTab))
			.SetDisplayName(NSLOCTEXT("FUndoHistoryModule", "UndoHistoryTabTitle", "Undo History"))
			.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory())
			.SetTooltipText(NSLOCTEXT("FUndoHistoryModule", "UndoHistoryTooltipText", "Open the Undo History tab."))
			.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "UndoHistory.TabIcon"));
	}

	virtual void ShutdownModule( ) OVERRIDE
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(UndoHistoryTabName);
	}

	virtual bool SupportsDynamicReloading( ) OVERRIDE
	{
		return true;
	}

	// End IModuleInterface interface

private:

	// Handles creating the project settings tab.
	TSharedRef<SDockTab> HandleSpawnSettingsTab( const FSpawnTabArgs& SpawnTabArgs )
	{
		const TSharedRef<SDockTab> DockTab = SNew(SDockTab)
			.TabRole(ETabRole::NomadTab);

		DockTab->SetContent(SNew(SUndoHistory));

		return DockTab;
	}
};


IMPLEMENT_MODULE(FUndoHistoryModule, UndoHistory);


#undef LOCTEXT_NAMESPACE
