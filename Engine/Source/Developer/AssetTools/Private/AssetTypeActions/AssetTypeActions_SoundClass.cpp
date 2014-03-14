// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AssetToolsPrivatePCH.h"
#include "SoundDefinitions.h"
#include "Editor/UnrealEd/Public/LinkedObjEditor.h"
#include "Editor/SoundClassEditor/Public/SoundClassEditorModule.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

UClass* FAssetTypeActions_SoundClass::GetSupportedClass() const
{
	return USoundClass::StaticClass();
}

void FAssetTypeActions_SoundClass::GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder )
{
	auto Classes = GetTypedWeakObjectPtrs<USoundClass>(InObjects);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("SoundClass_Edit", "Edit"),
		LOCTEXT("SoundClass_EditTooltip", "Opens the selected sound classes in the sound class editor."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_SoundClass::ExecuteEdit, Classes ),
			FCanExecuteAction()
			)
		);
}

void FAssetTypeActions_SoundClass::OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor )
{
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		USoundClass* SoundClass = Cast<USoundClass>(*ObjIt);
		if (SoundClass != NULL)
		{
			ISoundClassEditorModule* SoundClassEditorModule = &FModuleManager::LoadModuleChecked<ISoundClassEditorModule>( "SoundClassEditor" );
			SoundClassEditorModule->CreateSoundClassEditor(Mode, EditWithinLevelEditor, SoundClass);
		}
	}
}

void FAssetTypeActions_SoundClass::ExecuteEdit(TArray<TWeakObjectPtr<USoundClass>> Objects)
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