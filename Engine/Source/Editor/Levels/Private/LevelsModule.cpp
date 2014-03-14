// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "LevelsPrivatePCH.h"

#include "ModuleManager.h"

IMPLEMENT_MODULE( FLevelsModule, Levels );

void FLevelsModule::StartupModule()
{
	FLevelsViewCommands::Register();

	// register the editor mode
	TSharedRef<FEdModeLevel> NewEditorMode = MakeShareable(new FEdModeLevel);
	GEditorModeTools().RegisterMode(NewEditorMode);
	EdModeLevel = NewEditorMode;
}


void FLevelsModule::ShutdownModule()
{
	FLevelsViewCommands::Unregister();

	// unregister the editor mode
	GEditorModeTools().UnregisterMode(EdModeLevel.ToSharedRef());
	EdModeLevel = NULL;
}


TSharedRef< SWidget > FLevelsModule::CreateLevelBrowser()
{
	return SNew( SLevelBrowser );
}


