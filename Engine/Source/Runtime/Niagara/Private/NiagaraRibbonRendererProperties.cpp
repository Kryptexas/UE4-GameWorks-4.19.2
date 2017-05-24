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



#if WITH_EDITORONLY_DATA

bool UNiagaraRibbonRendererProperties::IsMaterialValidForRenderer(UMaterial* Material, FText& InvalidMessage)
{
	if (Material->bUsedWithNiagaraRibbons == false)
	{
		InvalidMessage = NSLOCTEXT("NiagaraRibbonRendererProperties", "InvalidMaterialMessage", "The material isn't marked as \"Used with Niagara ribbons\"");
		return false;
	}
	return true;
}

void UNiagaraRibbonRendererProperties::FixMaterial(UMaterial* Material)
{
	Material->Modify();
	Material->bUsedWithNiagaraRibbons = true;
	Material->ForceRecompileForRendering();
}

#endif // WITH_EDITORONLY_DATA
