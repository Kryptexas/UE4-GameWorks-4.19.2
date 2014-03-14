// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"

#include "SpriteAssetTypeActions.h"
#include "SpriteEditor/SpriteEditor.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

//////////////////////////////////////////////////////////////////////////
// FSpriteAssetTypeActions

FText FSpriteAssetTypeActions::GetName() const
{
	return LOCTEXT("FSpriteAssetTypeActionsName", "Sprite");
}

FColor FSpriteAssetTypeActions::GetTypeColor() const
{
	return FColor(0, 255, 255);
}

UClass* FSpriteAssetTypeActions::GetSupportedClass() const
{
	return UPaperSprite::StaticClass();
}

void FSpriteAssetTypeActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor)
{
	const EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		if (UPaperSprite* Sprite = Cast<UPaperSprite>(*ObjIt))
		{
			TSharedRef<FSpriteEditor> NewSpriteEditor(new FSpriteEditor());
			NewSpriteEditor->InitSpriteEditor(Mode, EditWithinLevelEditor, Sprite);
		}
	}
}

uint32 FSpriteAssetTypeActions::GetCategories()
{
	return EAssetTypeCategories::Misc;
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE