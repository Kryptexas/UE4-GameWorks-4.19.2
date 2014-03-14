// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AssetToolsPrivatePCH.h"
#include "Editor/FontEditor/Public/FontEditorModule.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

void FAssetTypeActions_Font::GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder )
{
	auto Fonts = GetTypedWeakObjectPtrs<UFont>(InObjects);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Font_Edit", "Edit"),
		LOCTEXT("Font_EditTooltip", "Opens the selected fonts in the font editor."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_Font::ExecuteEdit, Fonts ),
			FCanExecuteAction()
			)
		);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Font_Reimport", "Reimport"),
		LOCTEXT("Font_ReimportTooltip", "Reimports the selected fonts."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_Font::ExecuteReimport, Fonts ),
			FCanExecuteAction()
			)
		);
}

void FAssetTypeActions_Font::OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor )
{
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Font = Cast<UFont>(*ObjIt);
		if (Font != NULL)
		{
			IFontEditorModule* FontEditorModule = &FModuleManager::LoadModuleChecked<IFontEditorModule>("FontEditor");
			FontEditorModule->CreateFontEditor(Mode, EditWithinLevelEditor, Font);
		}
	}
}

void FAssetTypeActions_Font::ExecuteEdit(TArray<TWeakObjectPtr<UFont>> Objects)
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

void FAssetTypeActions_Font::ExecuteReimport(TArray<TWeakObjectPtr<UFont>> Objects)
{
	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Object = (*ObjIt).Get();
		if ( Object )
		{
			FReimportManager::Instance()->Reimport( Object );
		}
	}
}

#undef LOCTEXT_NAMESPACE