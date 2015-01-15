// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
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

	if (ensure(GEngine))
	{
		GEngine->OnWorldAdded().AddRaw(this, &FWorldBrowserModule::OnWorldCreated);
		GEngine->OnWorldDestroyed().AddRaw(this, &FWorldBrowserModule::OnWorldDestroyed);
	}

	UWorldComposition::EnableWorldCompositionEvent.BindRaw(this, &FWorldBrowserModule::EnableWorldComposition);
}

void FWorldBrowserModule::ShutdownModule()
{
	if (GEngine)
	{
		GEngine->OnWorldAdded().RemoveAll(this);
		GEngine->OnWorldDestroyed().RemoveAll(this);
	}
	
	UWorldComposition::EnableWorldCompositionEvent.Unbind();

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

void FWorldBrowserModule::OnWorldCreated(UWorld* InWorld)
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

void FWorldBrowserModule::OnWorldDestroyed(UWorld* InWorld)
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

TSharedPtr<FLevelCollectionModel> FWorldBrowserModule::SharedWorldModel(UWorld* InWorld)
{
	TSharedPtr<FLevelCollectionModel> SharedWorldModel = WorldModel.Pin();
	if (!SharedWorldModel.IsValid() || SharedWorldModel->GetWorld() != InWorld)
	{
		if (InWorld)
		{
			if (InWorld->WorldComposition)
			{
				SharedWorldModel = FWorldTileCollectionModel::Create(GEditor, InWorld);
			}
			else
			{
				SharedWorldModel = FStreamingLevelCollectionModel::Create(GEditor, InWorld);
			}
		}

		// Hold weak reference to shared world model
		WorldModel = SharedWorldModel;
	}
	
	return SharedWorldModel;
}

bool FWorldBrowserModule::EnableWorldComposition(UWorld* InWorld, bool bEnable)
{
	if (InWorld == nullptr || InWorld->WorldType != EWorldType::Editor)
	{
		return false;
	}
		
	if (!bEnable)
	{
		if (InWorld->WorldComposition != nullptr)
		{
			InWorld->FlushLevelStreaming();
			InWorld->WorldComposition->MarkPendingKill();
			InWorld->WorldComposition = nullptr;
			OnWorldCompositionChanged(InWorld);
		}

		return false;
	}
	
	if (InWorld->WorldComposition == nullptr)
	{
		FString RootPackageName = InWorld->GetOutermost()->GetName();

		// Map should be saved to disk
		if (!FPackageName::DoesPackageExist(RootPackageName))
		{
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("EnableWorldCompositionNotSaved_Message", "Please save your level to disk before enabling World Composition"));
			return false;
		}
			
		// All existing sub-levels on this map should be removed
		int32 NumExistingSublevels = InWorld->StreamingLevels.Num();
		if (NumExistingSublevels > 0)
		{
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("EnableWorldCompositionExistingSublevels_Message", "World Composition cannot be enabled because there are already sub-levels manually added to the persistent level. World Composition uses auto-discovery so you must first remove any manually added sub-levels from the Levels window"));
			return false;
		}
			
		UWorldComposition* WorldCompostion = ConstructObject<UWorldComposition>(UWorldComposition::StaticClass(), InWorld);
		// All map files found in the same and folder and all sub-folders will be added ass sub-levels to this map
		// Make sure user understands this
		int32 NumFoundSublevels = WorldCompostion->GetTilesList().Num();
		if (NumFoundSublevels)
		{
			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("NumSubLevels"), NumFoundSublevels);
			Arguments.Add(TEXT("FolderLocation"), FText::FromString(FPackageName::GetLongPackagePath(RootPackageName)));
			const FText Message = FText::Format(LOCTEXT("EnableWorldCompositionPrompt_Message", "World Composition auto-discovers sub-levels by scanning the folder the level is saved in, and all sub-folders. {NumSubLevels} level files were found in {FolderLocation} and will be added as sub-levels. Do you want to continue?"), Arguments);
			
			auto AppResult = FMessageDialog::Open(EAppMsgType::OkCancel, Message);
			if (AppResult != EAppReturnType::Ok)
			{
				WorldCompostion->MarkPendingKill();
				return false;
			}
		}
			
		// 
		InWorld->WorldComposition = WorldCompostion;
		OnWorldCompositionChanged(InWorld);
	}
	
	return true;
}

#undef LOCTEXT_NAMESPACE
