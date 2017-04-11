// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "NiagaraSpriteRendererProperties.h"
#include "NiagaraEffectRenderer.h"

UNiagaraSpriteRendererProperties::UNiagaraSpriteRendererProperties()
	: SubImageSize(1.0f, 1.0f)
	, Alignment(ENiagaraSpriteAlignment::Unaligned)
	, FacingMode(ENiagaraSpriteFacingMode::FaceCamera)
	, CustomFacingVectorMask(EForceInit::ForceInitToZero)
	, SortMode(ENiagaraSortMode::SortNone)
{
}

NiagaraEffectRenderer* UNiagaraSpriteRendererProperties::CreateEffectRenderer(ERHIFeatureLevel::Type FeatureLevel)
{
	return new NiagaraEffectRendererSprites(FeatureLevel, this);
}

void UNiagaraSpriteRendererProperties::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials) const
{
	//OutMaterials.Add(Material);
	//Material should live here.
}

#if WITH_EDITORONLY_DATA

bool UNiagaraSpriteRendererProperties::IsMaterialValidForRenderer(UMaterial* Material, FText& InvalidMessage)
{
	if (Material->bUsedWithNiagaraSprites == false)
	{
		InvalidMessage = NSLOCTEXT("NiagaraSpriteRendererProperties", "InvalidMaterialMessage", "The material isn't marked as \"Used with particle sprites\"");
		return false;
	}
	return true;
}

void UNiagaraSpriteRendererProperties::FixMaterial(UMaterial* Material)
{
	Material->Modify();
	Material->bUsedWithNiagaraSprites = true;
	Material->ForceRecompileForRendering();
}

#endif // WITH_EDITORONLY_DATA




