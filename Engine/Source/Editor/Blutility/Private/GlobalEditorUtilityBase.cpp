// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlutilityPrivatePCH.h"
#include "GlobalEditorUtilityBase.h"
#include "ScopedTransaction.h"
#include "ContentBrowserModule.h"
#include "AssetData.h"
#include "AssetToolsModule.h"

/////////////////////////////////////////////////////

UGlobalEditorUtilityBase::UGlobalEditorUtilityBase(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

TArray<AActor*> UGlobalEditorUtilityBase::GetSelectionSet()
{
	TArray<AActor*> Result;
	for (FSelectionIterator It(GEditor->GetSelectedActorIterator()); It; ++It)
	{
		if (AActor* Actor = Cast<AActor>(*It))
		{
			Result.Add(Actor);
		}
	}

	return Result;
}

void UGlobalEditorUtilityBase::GetSelectionBounds(FVector& Origin, FVector& BoxExtent, float& SphereRadius)
{
	bool bFirstItem = true;

	FBoxSphereBounds Extents;
	for (FSelectionIterator It(GEditor->GetSelectedActorIterator()); It; ++It)
	{
		if (AActor* Actor = Cast<AActor>(*It))
		{
			if (bFirstItem)
			{
				Extents = Actor->GetRootComponent()->Bounds;
			}
			else
			{
				Extents = Extents + Actor->GetRootComponent()->Bounds;
			}

			bFirstItem = false;
		}
	}

	Origin = Extents.Origin;
	BoxExtent = Extents.BoxExtent;
	SphereRadius = Extents.SphereRadius;
}

void UGlobalEditorUtilityBase::ForEachSelectedActor()
{
	TArray<AActor*> SelectionSetCache;
	for (FSelectionIterator It(GEditor->GetSelectedActorIterator()); It; ++It)
	{
		if (AActor* Actor = Cast<AActor>(*It))
		{
			SelectionSetCache.Add(Actor);
		}
	}

	int32 Index = 0;
	for (auto ActorIt = SelectionSetCache.CreateIterator(); ActorIt; ++ActorIt)
	{
		AActor* Actor = *ActorIt;
		OnEachSelectedActor.Broadcast(Actor, Index);
		++Index;
	}
}

void UGlobalEditorUtilityBase::ForEachSelectedAsset()
{
	//@TODO: Blocking load, no slow dialog
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	TArray<FAssetData> SelectedAssets;
	ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

	int32 Index = 0;
	for (auto AssetIt = SelectedAssets.CreateIterator(); AssetIt; ++AssetIt)
	{
		const FAssetData& AssetData = *AssetIt;
		if (UObject* Asset = AssetData.GetAsset())
		{
			OnEachSelectedAsset.Broadcast(Asset, Index);
			++Index;
		}
	}
}

UEditorUserSettings* UGlobalEditorUtilityBase::GetEditorUserSettings()
{
	return &(GEditor->AccessEditorUserSettings());
}

void UGlobalEditorUtilityBase::ClearActorSelectionSet()
{
	GEditor->GetSelectedActors()->DeselectAll();
	bDirtiedSelectionSet = true;
}

void UGlobalEditorUtilityBase::SetActorSelectionState(AActor* Actor, bool bShouldBeSelected)
{
	GEditor->SelectActor(Actor, bShouldBeSelected, /*bNotify=*/ false);
	bDirtiedSelectionSet = true;
}

void UGlobalEditorUtilityBase::PostExecutionCleanup()
{
	if (bDirtiedSelectionSet)
	{
		GEditor->NoteSelectionChange();
		bDirtiedSelectionSet = false;
	}

	OnEachSelectedActor.Clear();
	OnEachSelectedAsset.Clear();
}

void UGlobalEditorUtilityBase::ExecuteDefaultAction()
{
	check(bAutoRunDefaultAction);

	FScopedTransaction Transaction( NSLOCTEXT("UnrealEd", "BlutilityAction", "Blutility Action") );
	FEditorScriptExecutionGuard ScriptGuard;

	OnDefaultActionClicked();
	PostExecutionCleanup();
}

void UGlobalEditorUtilityBase::RenameAsset(UObject* Asset, const FString& NewName)
{
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");

	TArray<FAssetRenameData> AssetsAndNames;
	const FString PackagePath = FPackageName::GetLongPackagePath(Asset->GetOutermost()->GetName());
	new (AssetsAndNames) FAssetRenameData(Asset, PackagePath, NewName);

	AssetToolsModule.Get().RenameAssets(AssetsAndNames);
}