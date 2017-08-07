// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "LiveLinkRemapAssetActions.h"
//#include "EngineGlobals.h"
//#include "Engine/Engine.h"
#include "LiveLinkRemapAsset.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"

#define LOCTEXT_NAMESPACE "LiveLinkRemapAssetActions"

FLiveLinkRemapAssetActions::FLiveLinkRemapAssetActions()
{
}

uint32 FLiveLinkRemapAssetActions::GetCategories()
{
	return EAssetTypeCategories::Animation;
}


FText FLiveLinkRemapAssetActions::GetName() const
{
	return LOCTEXT("AssetTypeActionName", "Live Link Remap Asset");
}


UClass* FLiveLinkRemapAssetActions::GetSupportedClass() const
{
	return ULiveLinkRemapAsset::StaticClass();
}


FColor FLiveLinkRemapAssetActions::GetTypeColor() const
{
	return FColor(252, 227, 86);
}


void FLiveLinkRemapAssetActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	// forward to level sequence asset actions
	/*FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
	TWeakPtr<IAssetTypeActions> AssetTypeActions = AssetToolsModule.Get().GetAssetTypeActionsForClass(ULevelSequence::StaticClass());
	if (AssetTypeActions.IsValid())
	{
		AssetTypeActions.Pin()->OpenAssetEditor(InObjects, EditWithinLevelEditor);
	}*/
}

#undef LOCTEXT_NAMESPACE
