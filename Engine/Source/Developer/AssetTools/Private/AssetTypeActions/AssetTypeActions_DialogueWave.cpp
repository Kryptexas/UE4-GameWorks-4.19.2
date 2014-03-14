// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AssetToolsPrivatePCH.h"
#include "SoundDefinitions.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

void FAssetTypeActions_DialogueWave::GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder )
{
	auto DialogueWaves = GetTypedWeakObjectPtrs<UDialogueWave>(InObjects);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("DialogueWave_Edit", "Edit"),
		LOCTEXT("DialogueWave_EditTooltip", "Opens the selected Dialogue Waves in the Dialogue Wave editor."),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP( this, &FAssetTypeActions_DialogueWave::ExecuteEdit, DialogueWaves ),
		FCanExecuteAction()
		)
		);
}

void FAssetTypeActions_DialogueWave::OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor )
{
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto DialogueWave = Cast<UDialogueWave>(*ObjIt);
		if (DialogueWave != NULL)
		{
			FSimpleAssetEditor::CreateEditor(Mode, Mode == EToolkitMode::WorldCentric ? EditWithinLevelEditor : TSharedPtr<IToolkitHost>(), DialogueWave);
		}
	}
}

void FAssetTypeActions_DialogueWave::ExecuteEdit(TArray<TWeakObjectPtr<UDialogueWave>> Objects)
{
	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Object = (*ObjIt).Get();
		if ( Object )
		{
			FAssetEditorManager::Get().OpenEditorForAsset(Object);
		}
	}
}

#undef LOCTEXT_NAMESPACE