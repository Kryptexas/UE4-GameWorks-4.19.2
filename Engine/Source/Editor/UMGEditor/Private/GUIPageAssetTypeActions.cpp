// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "GUIPageAssetTypeActions.h"
#include "UMGEditor.h"

#define LOCTEXT_NAMESPACE "FGUIPageAssetTypeActions"

//////////////////////////////////////////////////////////////////////////
// FGUIPageAssetTypeActions

FText FGUIPageAssetTypeActions::GetName() const
{
	return LOCTEXT("FGUIPageAssetTypeActionsName", "GUIPage");
}

FColor FGUIPageAssetTypeActions::GetTypeColor() const
{
	return FColor(0, 255, 255);
}

UClass* FGUIPageAssetTypeActions::GetSupportedClass() const
{
	return UGUIPage::StaticClass();
}

void FGUIPageAssetTypeActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor)
{
	const EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		if ( UGUIPage* GUIPage = Cast<UGUIPage>(*ObjIt) )
		{
			TSharedRef<FUMGEditor> NewGUIPageEditor(new FUMGEditor());
			NewGUIPageEditor->Initialize(Mode, EditWithinLevelEditor, GUIPage);
		}
	}
}

uint32 FGUIPageAssetTypeActions::GetCategories()
{
	return EAssetTypeCategories::Misc;
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE