// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "AssetEditorModeManager.h"
#include "PreviewScene.h"

//////////////////////////////////////////////////////////////////////////
// FAssetEditorModeManager

FAssetEditorModeManager::FAssetEditorModeManager()
	: PreviewScene(nullptr)
{
	ActorSet = NewObject<USelection>();
	ActorSet->SetFlags(RF_Transactional);
	ActorSet->AddToRoot();

	ObjectSet = NewObject<USelection>();
	ObjectSet->SetFlags(RF_Transactional);
	ObjectSet->AddToRoot();
}

FAssetEditorModeManager::~FAssetEditorModeManager()
{
	ActorSet->RemoveFromRoot();
	ActorSet = nullptr;
	ObjectSet->RemoveFromRoot();
	ObjectSet = nullptr;
}

USelection* FAssetEditorModeManager::GetSelectedActors() const
{
	return ActorSet;
}

USelection* FAssetEditorModeManager::GetSelectedObjects() const
{
	return ObjectSet;
}

UWorld* FAssetEditorModeManager::GetWorld() const
{
	return (PreviewScene != nullptr) ? PreviewScene->GetWorld() : GEditor->GetEditorWorldContext().World();
}

void FAssetEditorModeManager::SetPreviewScene(class FPreviewScene* NewPreviewScene)
{
	PreviewScene = NewPreviewScene;
}

