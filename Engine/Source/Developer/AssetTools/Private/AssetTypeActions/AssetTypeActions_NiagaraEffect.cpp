// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AssetToolsPrivatePCH.h"
#include "Engine/NiagaraEffect.h"

#include "Toolkits/AssetEditorManager.h"

#include "Editor/NiagaraEditor/Public/NiagaraEditorModule.h"
#include "Editor/NiagaraEditor/Public/INiagaraEffectEditor.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

void FAssetTypeActions_NiagaraEffect::GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder)
{
	auto Scripts = GetTypedWeakObjectPtrs<UNiagaraEffect>(InObjects);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("NiagaraEffect_Edit", "Edit"),
		LOCTEXT("NiagaraEffect_EditTooltip", "Opens the selected effect in editor."),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP(this, &FAssetTypeActions_NiagaraEffect::ExecuteEdit, Scripts),
		FCanExecuteAction()
		)
		);
}

void FAssetTypeActions_NiagaraEffect::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Script = Cast<UNiagaraEffect>(*ObjIt);
		if (Script != NULL)
		{
			FNiagaraEditorModule& NiagaraEditorModule = FModuleManager::LoadModuleChecked<FNiagaraEditorModule>("NiagaraEditor");
			TSharedRef< INiagaraEffectEditor > NewNiagaraEditor = NiagaraEditorModule.CreateNiagaraEffectEditor(EToolkitMode::Standalone, EditWithinLevelEditor, Script);
		}
	}
}

void FAssetTypeActions_NiagaraEffect::ExecuteEdit(TArray<TWeakObjectPtr<UNiagaraEffect>> Objects)
{
	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Object = (*ObjIt).Get();
		if (Object)
		{
			FAssetEditorManager::Get().OpenEditorForAsset(Object);
		}
	}
}

UClass* FAssetTypeActions_NiagaraEffect::GetSupportedClass() const
{
	return UNiagaraEffect::StaticClass();
}


#undef LOCTEXT_NAMESPACE
