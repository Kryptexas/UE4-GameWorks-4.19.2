// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "WorldBrowserPrivatePCH.h"

#include "ModuleManager.h"
#include "SWorldMainView.h"
#include "StreamingLevels/StreamingLevelEdMode.h"

#include "WorldBrowser.generated.inl"

IMPLEMENT_MODULE( FWorldBrowserModule, WorldBrowser );

void FWorldBrowserModule::StartupModule()
{
	FLevelCollectionCommands::Register();

	// register the editor mode
	TSharedRef<FStreamingLevelEdMode> NewEditorMode = MakeShareable(new FStreamingLevelEdMode);
	GEditorModeTools().RegisterMode(NewEditorMode);
	EdModeStreamingLevel = NewEditorMode;
}

void FWorldBrowserModule::ShutdownModule()
{
	FLevelCollectionCommands::Unregister();

	// unregister the editor mode
	GEditorModeTools().UnregisterMode(EdModeStreamingLevel.ToSharedRef());
	EdModeStreamingLevel = NULL;
}

TSharedRef<SWidget> FWorldBrowserModule::CreateWorldBrowser()
{
	return SNew(SWorldMainView);
}


