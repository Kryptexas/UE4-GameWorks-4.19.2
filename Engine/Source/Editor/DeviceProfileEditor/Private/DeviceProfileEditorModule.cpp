// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	DeviceProfileEditorModule.cpp: Implements the FDeviceProfileEditorModule class.
=============================================================================*/

#include "DeviceProfileEditorPCH.h"
#include "WorkspaceMenuStructureModule.h"


#define LOCTEXT_NAMESPACE "DeviceProfileEditor"


IMPLEMENT_MODULE(FDeviceProfileEditorModule, DeviceProfileEditor);


static const FName DeviceProfileEditorName("DeviceProfileEditor");


void FDeviceProfileEditorModule::StartupModule()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(DeviceProfileEditorName, FOnSpawnTab::CreateStatic(&FDeviceProfileEditorModule::SpawnDeviceProfileEditorTab))
		.SetDisplayName( NSLOCTEXT("DeviceProfileEditor", "DeviceProfileEditorTitle", "Device Profile Editor") )
		.SetGroup( WorkspaceMenu::GetMenuStructure().GetToolsCategory() );
}


void FDeviceProfileEditorModule::ShutdownModule()
{
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(DeviceProfileEditorName);
}


TSharedRef<SDockTab> FDeviceProfileEditorModule::SpawnDeviceProfileEditorTab(const FSpawnTabArgs& SpawnTabArgs)
{
	const TSharedRef<SDockTab> MajorTab = SNew(SDockTab)
		.TabRole(ETabRole::MajorTab);

	const TSharedRef<SDeviceProfileEditor> DeviceProfileEditor = SNew(SDeviceProfileEditor);

	MajorTab->SetContent(DeviceProfileEditor);

	return MajorTab;
}


#undef LOCTEXT_NAMESPACE