// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AssetToolsPrivatePCH.h"
#include "Toolkits/AssetEditorManager.h"

#include "Editor/NiagaraEditor/Public/NiagaraEditorModule.h"
#include "Editor/NiagaraEditor/Public/INiagaraEditor.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

void FAssetTypeActions_NiagaraScript::GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder )
{
	auto Scripts = GetTypedWeakObjectPtrs<UNiagaraScript>(InObjects);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("NiagaraScript_Edit", "Edit"),
		LOCTEXT("NiagaraScript_EditTooltip", "Opens the selected script in editor."),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP( this, &FAssetTypeActions_NiagaraScript::ExecuteEdit, Scripts ),
		FCanExecuteAction()
		)
		);
}

void FAssetTypeActions_NiagaraScript::OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor )
{
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Script = Cast<UNiagaraScript>(*ObjIt);
		if (Script != NULL)
		{
			FNiagaraEditorModule& NiagaraEditorModule = FModuleManager::LoadModuleChecked<FNiagaraEditorModule>( "NiagaraEditor" );
			TSharedRef< INiagaraEditor > NewNiagaraEditor = NiagaraEditorModule.CreateNiagaraEditor( EToolkitMode::Standalone, EditWithinLevelEditor, Script );
		}
	}
}

void FAssetTypeActions_NiagaraScript::ExecuteEdit(TArray<TWeakObjectPtr<UNiagaraScript>> Objects)
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

UClass* FAssetTypeActions_NiagaraScript::GetSupportedClass() const
{ 
	return UNiagaraScript::StaticClass(); 
}


#undef LOCTEXT_NAMESPACE
