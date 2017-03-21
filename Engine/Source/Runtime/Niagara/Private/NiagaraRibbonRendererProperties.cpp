// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "NiagaraRibbonRendererProperties.h"
#include "NiagaraEffectRendererRibbon.h"

NiagaraEffectRenderer* UNiagaraRibbonRendererProperties::CreateEffectRenderer(ERHIFeatureLevel::Type FeatureLevel)
{
	return new NiagaraEffectRendererRibbon(FeatureLevel, this);
}

void UNiagaraRibbonRendererProperties::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials) const
{
	//OutMaterials.Add(Material);
	//Material should live here.
}