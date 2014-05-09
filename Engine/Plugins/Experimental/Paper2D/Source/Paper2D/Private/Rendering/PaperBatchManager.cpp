// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DPrivatePCH.h"
#include "PaperBatchManager.h"
#include "PaperBatchComponent.h"

namespace FPaperBatchManagerNS
{
	static TMap< UWorld*, UPaperBatchComponent* > WorldToBatcherMap;
	static FWorldDelegates::FWorldInitializationEvent::FDelegate OnWorldCreatedDelegate;
	static FWorldDelegates::FWorldEvent::FDelegate OnWorldDestroyedDelegate;
}

//////////////////////////////////////////////////////////////////////////
// FPaperBatchManager

void FPaperBatchManager::Initialize()
{
	FPaperBatchManagerNS::OnWorldCreatedDelegate = FWorldDelegates::FWorldInitializationEvent::FDelegate::CreateStatic(&FPaperBatchManager::OnWorldCreated);
	FPaperBatchManagerNS::OnWorldDestroyedDelegate = FWorldDelegates::FWorldEvent::FDelegate::CreateStatic(&FPaperBatchManager::OnWorldDestroyed);
	FWorldDelegates::OnPostWorldInitialization.Add(FPaperBatchManagerNS::OnWorldCreatedDelegate);
	FWorldDelegates::OnPreWorldFinishDestroy.Add(FPaperBatchManagerNS::OnWorldDestroyedDelegate);
}

void FPaperBatchManager::Shutdown()
{
	FWorldDelegates::OnPostWorldInitialization.Remove(FPaperBatchManagerNS::OnWorldCreatedDelegate);
	FWorldDelegates::OnPreWorldFinishDestroy.Remove(FPaperBatchManagerNS::OnWorldDestroyedDelegate);
}

void FPaperBatchManager::OnWorldCreated(UWorld* World, const UWorld::InitializationValues IVS)
{
	if (IVS.bInitializeScenes)
	{
		UPaperBatchComponent* Batcher = NewObject<UPaperBatchComponent>();
		Batcher->AddToRoot();
		FPaperBatchManagerNS::WorldToBatcherMap.Add(World, Batcher);

		World->Scene->AddPrimitive(Batcher);
	}
}

void FPaperBatchManager::OnWorldDestroyed(UWorld* World)
{
	UPaperBatchComponent* Batcher = FPaperBatchManagerNS::WorldToBatcherMap.FindChecked(World);
	FPaperBatchManagerNS::WorldToBatcherMap.Remove(World);

	World->Scene->RemovePrimitive(Batcher);
	Batcher->RemoveFromRoot();
}

UPaperBatchComponent* FPaperBatchManager::GetBatchComponent(UWorld* World)
{
	UPaperBatchComponent* Batcher = FPaperBatchManagerNS::WorldToBatcherMap.FindChecked(World);
	return Batcher;
}

FPaperBatchSceneProxy* FPaperBatchManager::GetBatcher(FSceneInterface* Scene)
{
	UPaperBatchComponent* BatchComponent = GetBatchComponent(Scene->GetWorld());
	return static_cast<FPaperBatchSceneProxy*>(BatchComponent->SceneProxy);
}
