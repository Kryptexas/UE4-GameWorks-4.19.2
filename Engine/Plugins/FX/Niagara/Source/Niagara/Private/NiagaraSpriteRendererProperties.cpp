// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NiagaraSpriteRendererProperties.h"
#include "NiagaraRenderer.h"
#include "Internationalization.h"
#include "NiagaraConstants.h"

#define LOCTEXT_NAMESPACE "UNiagaraSpriteRendererProperties"

UNiagaraSpriteRendererProperties::UNiagaraSpriteRendererProperties()
	: Alignment(ENiagaraSpriteAlignment::Unaligned)
	, FacingMode(ENiagaraSpriteFacingMode::FaceCamera)
	, CustomFacingVectorMask(EForceInit::ForceInitToZero)
	, PivotInUVSpace(0.5f, 0.5f)
	, SortMode(ENiagaraSortMode::SortNone)
	, SubImageSize(1.0f, 1.0f)
	, bSubImageBlend(false)
	, bRemoveHMDRollInVR(false)
	, MinFacingCameraBlendDistance(0.0f)
	, MaxFacingCameraBlendDistance(0.0f)
{
}

NiagaraRenderer* UNiagaraSpriteRendererProperties::CreateEmitterRenderer(ERHIFeatureLevel::Type FeatureLevel)
{
	return new NiagaraRendererSprites(FeatureLevel, this);
}

void UNiagaraSpriteRendererProperties::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials) const
{
	OutMaterials.Add(Material);
}


#if WITH_EDITORONLY_DATA

const TArray<FNiagaraVariable>& UNiagaraSpriteRendererProperties::GetRequiredAttributes()
{
	static TArray<FNiagaraVariable> Attrs;

	if (Attrs.Num() == 0)
	{
		Attrs.Add(SYS_PARAM_PARTICLES_POSITION);
		Attrs.Add(SYS_PARAM_PARTICLES_VELOCITY);
		Attrs.Add(SYS_PARAM_PARTICLES_COLOR);
		Attrs.Add(SYS_PARAM_PARTICLES_SPRITE_ROTATION);
		Attrs.Add(SYS_PARAM_PARTICLES_NORMALIZED_AGE);
		Attrs.Add(SYS_PARAM_PARTICLES_SPRITE_SIZE);
	}

	return Attrs;
}


const TArray<FNiagaraVariable>& UNiagaraSpriteRendererProperties::GetOptionalAttributes()
{
	static TArray<FNiagaraVariable> Attrs;

	if (Attrs.Num() == 0)
	{
		Attrs.Add(SYS_PARAM_PARTICLES_SPRITE_FACING);
		Attrs.Add(SYS_PARAM_PARTICLES_SPRITE_ALIGNMENT);
		Attrs.Add(SYS_PARAM_PARTICLES_SUB_IMAGE_INDEX);
		Attrs.Add(SYS_PARAM_PARTICLES_DYNAMIC_MATERIAL_PARAM);
		Attrs.Add(SYS_PARAM_PARTICLES_CAMERA_OFFSET);
		Attrs.Add(SYS_PARAM_PARTICLES_UV_SCALE);
		Attrs.Add(SYS_PARAM_PARTICLES_MATERIAL_RANDOM);
	}

	return Attrs;
}


bool UNiagaraSpriteRendererProperties::IsMaterialValidForRenderer(UMaterial* InMaterial, FText& InvalidMessage)
{
	if (InMaterial->bUsedWithNiagaraSprites == false)
	{
		InvalidMessage = NSLOCTEXT("NiagaraSpriteRendererProperties", "InvalidMaterialMessage", "The material isn't marked as \"Used with particle sprites\"");
		return false;
	}
	return true;
}

void UNiagaraSpriteRendererProperties::FixMaterial(UMaterial* InMaterial)
{
	InMaterial->Modify();
	InMaterial->bUsedWithNiagaraSprites = true;
	InMaterial->ForceRecompileForRendering();
}

#undef LOCTEXT_NAMESPACE

#endif // WITH_EDITORONLY_DATA




