// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "NiagaraSpriteRendererProperties.h"
#include "NiagaraEffectRenderer.h"

UNiagaraSpriteRendererProperties::UNiagaraSpriteRendererProperties()
	: SubImageInfo(1.0f, 1.0f)
{
}

NiagaraEffectRenderer* UNiagaraSpriteRendererProperties::CreateEffectRenderer(ERHIFeatureLevel::Type FeatureLevel)
{
	return new NiagaraEffectRendererSprites(FeatureLevel, this);
}

#if WITH_EDITORONLY_DATA

bool UNiagaraSpriteRendererProperties::IsMaterialValidForRenderer(UMaterial* Material, FText& InvalidMessage)
{
	if (Material->bUsedWithParticleSprites == false)
	{
		InvalidMessage = NSLOCTEXT("NiagaraSpriteRendererProperties", "InvalidMaterialMessage", "The material isn't marked as \"Used with particle sprites\"");
		return false;
	}
	return true;
}

void UNiagaraSpriteRendererProperties::FixMaterial(UMaterial* Material)
{
	Material->Modify();
	Material->bUsedWithParticleSprites = true;
	Material->ForceRecompileForRendering();
}

#endif // WITH_EDITORONLY_DATA

