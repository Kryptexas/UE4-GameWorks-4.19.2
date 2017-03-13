// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "AssetTypeActions_NiagaraEmitter.h"
#include "NiagaraEmitterProperties.h"
#include "NiagaraEmitterToolkit.h"

class UNiagaraEmitterProperties;

void FAssetTypeActions_NiagaraEmitter::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Emitter = Cast<UNiagaraEmitterProperties>(*ObjIt);
		if (Emitter != nullptr)
		{
			TSharedRef<FNiagaraEmitterToolkit> EmitterToolkit(new FNiagaraEmitterToolkit());
			EmitterToolkit->Initialize(Mode, EditWithinLevelEditor, *Emitter);
		}
	}
}

UClass* FAssetTypeActions_NiagaraEmitter::GetSupportedClass() const
{
	return UNiagaraEmitterProperties::StaticClass();
}
