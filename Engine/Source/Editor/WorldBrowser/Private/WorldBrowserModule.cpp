// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "WorldBrowserPrivatePCH.h"

#include "Engine/WorldComposition.h"
#include "ModuleManager.h"
#include "StreamingLevels/StreamingLevelEdMode.h"
#include "Tiles/WorldTileCollectionModel.h"
#include "StreamingLevels/StreamingLevelCollectionModel.h"
#include "SWorldHierarchy.h"
#include "SWorldDetails.h"
#include "Tiles/SWorldComposition.h"


IMPLEMENT_MODULE( FWorldBrowserModule, WorldBrowser );

#define LOCTEXT_NAMESPACE "WorldBrowser"

void FWorldBrowserModule::StartupModule()
{
	FLevelCollectionCommands::Register();

	// register the editor mode
	FEditorModeRegistry::Get().RegisterMode<FStreamingLevelEdMode>(
		FBuiltinEditorModes::EM_StreamingLevel,
		NSLOCTEXT("WorldBrowser", "StreamingLevelMode", "Level Transform Editing"));

	FWorldDelegates::OnPostWorldInitialization.AddRaw(this, &FWorldBrowserModule::OnWorldCreated);
	FWorldDelegates::OnWorldCleanup.AddRaw(this, &FWorldBrowserModule::OnWorldCleanup);
	UWorldComposition::OnWorldCompositionCreated.AddRaw(this, &FWorldBrowserModule::OnWorldCompositionChanged);
	UWorldComposition::OnWorldCompositionDestroyed.AddRaw(this, &FWorldBrowserModule::OnWorldCompositionChanged);
}

void FWorldBrowserModule::ShutdownModule()
{
	FWorldDelegates::OnPostWorldInitialization.RemoveAll(this);
	FWorldDelegates::OnWorldCleanup.RemoveAll(this);
	UWorldComposition::OnWorldCompositionCreated.RemoveAll(this);
	UWorldComposition::OnWorldCompositionDestroyed.RemoveAll(this);

	FLevelCollectionCommands::Unregister();

	// unregister the editor mode
	FEditorModeRegistry::Get().UnregisterMode(FBuiltinEditorModes::EM_StreamingLevel);
}

TSharedRef<SWidget> FWorldBrowserModule::CreateWorldBrowserHierarchy()
{
	UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
	return SNew(SWorldHierarchy).InWorld(EditorWorld);
}
	
TSharedRef<SWidget> FWorldBrowserModule::CreateWorldBrowserDetails()
{
	UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
	return SNew(SWorldDetails).InWorld(EditorWorld);
}

TSharedRef<SWidget> FWorldBrowserModule::CreateWorldBrowserComposition()
{
	UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
	return SNew(SWorldComposition).InWorld(EditorWorld);
}

void FWorldBrowserModule::OnWorldCreated(UWorld* InWorld, const UWorld::InitializationValues IVS)
{
	if (InWorld && 
		InWorld->WorldType == EWorldType::Editor)
	{
		OnBrowseWorld.Broadcast(InWorld);
	}
}

void FWorldBrowserModule::OnWorldCompositionChanged(UWorld* InWorld)
{
	if (InWorld && 
		InWorld->WorldType == EWorldType::Editor)
	{
		OnBrowseWorld.Broadcast(NULL);
		OnBrowseWorld.Broadcast(InWorld);
	}
}

void FWorldBrowserModule::OnWorldCleanup(UWorld* InWorld, bool bSessionEnded, bool bCleanupResources)
{
	if (bCleanupResources)
	{
		TSharedPtr<FLevelCollectionModel> SharedWorldModel = WorldModel.Pin();
		// Is there any editors alive?
		if (SharedWorldModel.IsValid())
		{
			UWorld* ManagedWorld = SharedWorldModel->GetWorld(/*bEvenIfPendingKill*/true);
			// Is it our world gets cleaned up?
			if (ManagedWorld == InWorld)
			{
				// Will reset all references to a shared world model
				OnBrowseWorld.Broadcast(NULL);
				// So we have to be the last owner of this model
				check(SharedWorldModel.IsUnique());
			}
		}
	}
}

TSharedPtr<FLevelCollectionModel> FWorldBrowserModule::SharedWorldModel(UWorld* InWorld)
{
	TSharedPtr<FLevelCollectionModel> SharedWorldModel = WorldModel.Pin();
	if (!SharedWorldModel.IsValid() || SharedWorldModel->GetWorld() != InWorld)
	{
		if (InWorld)
		{
			if (InWorld->WorldComposition)
			{
				SharedWorldModel = FWorldTileCollectionModel::Create(GEditor);
			}
			else
			{
				SharedWorldModel = FStreamingLevelCollectionModel::Create(GEditor);
			}
		}

		// Hold weak reference to shared world model
		WorldModel = SharedWorldModel;
	}
	
	return SharedWorldModel;
}




#undef LOCTEXT_NAMESPACE
