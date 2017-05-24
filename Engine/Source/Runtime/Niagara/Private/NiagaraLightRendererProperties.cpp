// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "NiagaraLightRendererProperties.h"
#include "NiagaraEffectRenderer.h"


UNiagaraLightRendererProperties::UNiagaraLightRendererProperties()
	: RadiusScale(1.0f), ColorAdd(FVector(0.0f, 0.0f, 0.0f))
{
}

NiagaraEffectRenderer* UNiagaraLightRendererProperties::CreateEffectRenderer(ERHIFeatureLevel::Type FeatureLevel)
{
	return new NiagaraEffectRendererLights(FeatureLevel, this);
}

void UNiagaraLightRendererProperties::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials) const
{
	//OutMaterials.Add(Material);
	//Material should live here.
}

#if WITH_EDITORONLY_DATA

bool UNiagaraLightRendererProperties::IsMaterialValidForRenderer(UMaterial* Material, FText& InvalidMessage)
{
	return true;
}

void UNiagaraLightRendererProperties::FixMaterial(UMaterial* Material)
{
}

#endif // WITH_EDITORONLY_DATA

