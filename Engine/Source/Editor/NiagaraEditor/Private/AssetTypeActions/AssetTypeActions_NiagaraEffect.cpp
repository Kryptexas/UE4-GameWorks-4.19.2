// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "AssetTypeActions/AssetTypeActions_NiagaraEffect.h"
#include "NiagaraEffect.h"
#include "NiagaraEffectToolkit.h"

void FAssetTypeActions_NiagaraEffect::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Effect = Cast<UNiagaraEffect>(*ObjIt);
		if (Effect != NULL)
		{
			TSharedRef< FNiagaraEffectToolkit > NewNiagaraEffectToolkit(new FNiagaraEffectToolkit());
			NewNiagaraEffectToolkit->Initialize(Mode, EditWithinLevelEditor, Effect);
		}
	}
}

UClass* FAssetTypeActions_NiagaraEffect::GetSupportedClass() const
{
	return UNiagaraEffect::StaticClass();
}
