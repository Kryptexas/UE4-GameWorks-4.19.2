// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MaterialInterface.cpp: UMaterialInterface implementation.
=============================================================================*/

#include "EnginePrivate.h"
#include "EngineMaterialClasses.h"

UMaterialInterface::UMaterialInterface(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		InitDefaultMaterials();
		AssertDefaultMaterialsExist();
	}
}

void UMaterialInterface::PostLoad()
{
	Super::PostLoad();
	PostLoadDefaultMaterials();
}

FMaterialRelevance UMaterialInterface::GetRelevance_Internal(const UMaterial* Material) const
{
	if(Material)
	{
		const FMaterialResource* MaterialResource = Material->GetMaterialResource(GRHIFeatureLevel);
		const bool bIsTranslucent = IsTranslucentBlendMode((EBlendMode)GetBlendMode());
		const bool bIsLit = GetLightingModel() != MLM_Unlit;
		// Determine the material's view relevance.
		FMaterialRelevance MaterialRelevance;
		MaterialRelevance.bOpaque = !bIsTranslucent;
		MaterialRelevance.bMasked = Material->bIsMasked;
		MaterialRelevance.bDistortion = Material->bUsesDistortion && bIsTranslucent;
		MaterialRelevance.bUsesSceneColor = MaterialResource->UsesSceneColor();
		MaterialRelevance.bSeparateTranslucency = bIsTranslucent && Material->bEnableSeparateTranslucency;
		MaterialRelevance.bNormalTranslucency = bIsTranslucent && !Material->bEnableSeparateTranslucency;
		MaterialRelevance.bDisableDepthTest = bIsTranslucent && Material->bDisableDepthTest;
		return MaterialRelevance;
	}
	else
	{
		return FMaterialRelevance();
	}
}

FMaterialRelevance UMaterialInterface::GetRelevance() const
{
	// Find the interface's concrete material.
	const UMaterial* Material = GetMaterial();
	return GetRelevance_Internal(Material);
}

FMaterialRelevance UMaterialInterface::GetRelevance_Concurrent() const
{
	// Find the interface's concrete material.
	TMicRecursionGuard RecursionGuard;
	const UMaterial* Material = GetMaterial_Concurrent(RecursionGuard);
	return GetRelevance_Internal(Material);
}

int32 UMaterialInterface::GetWidth() const
{
	return ME_PREV_THUMBNAIL_SZ+(ME_STD_BORDER*2);
}

int32 UMaterialInterface::GetHeight() const
{
	return ME_PREV_THUMBNAIL_SZ+ME_CAPTION_HEIGHT+(ME_STD_BORDER*2);
}


void UMaterialInterface::SetForceMipLevelsToBeResident( bool OverrideForceMiplevelsToBeResident, bool bForceMiplevelsToBeResidentValue, float ForceDuration, int32 CinematicTextureGroups )
{
	TArray<UTexture*> Textures;
	
	GetUsedTextures(Textures, EMaterialQualityLevel::Num, false);
	for ( int32 TextureIndex=0; TextureIndex < Textures.Num(); ++TextureIndex )
	{
		UTexture2D* Texture = Cast<UTexture2D>(Textures[TextureIndex]);
		if ( Texture )
		{
			Texture->SetForceMipLevelsToBeResident( ForceDuration, CinematicTextureGroups );
			if (OverrideForceMiplevelsToBeResident)
			{
				Texture->bForceMiplevelsToBeResident = bForceMiplevelsToBeResidentValue;
			}
		}
	}
}

void UMaterialInterface::RecacheAllMaterialUniformExpressions()
{
	// For each interface, reacache its uniform parameters
	for( TObjectIterator<UMaterialInterface> MaterialIt; MaterialIt; ++MaterialIt )
	{
		MaterialIt->RecacheUniformExpressions();
	}
}

bool UMaterialInterface::IsReadyForFinishDestroy()
{
	bool bIsReady = Super::IsReadyForFinishDestroy();
	bIsReady = bIsReady && ParentRefFence.IsFenceComplete(); 
	return bIsReady;
}

void UMaterialInterface::BeginDestroy()
{
	ParentRefFence.BeginFence();

	Super::BeginDestroy();
}

void UMaterialInterface::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);

	SetLightingGuid();
}

#if WITH_EDITOR
void UMaterialInterface::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	// flush the lighting guid on all changes
	SetLightingGuid();

	LightmassSettings.EmissiveBoost = FMath::Max(LightmassSettings.EmissiveBoost, 0.0f);
	LightmassSettings.DiffuseBoost = FMath::Max(LightmassSettings.DiffuseBoost, 0.0f);
	LightmassSettings.ExportResolutionScale = FMath::Clamp(LightmassSettings.ExportResolutionScale, 0.0f, 16.0f);
	LightmassSettings.DistanceFieldPenumbraScale = FMath::Clamp(LightmassSettings.DistanceFieldPenumbraScale, 0.01f, 100.0f);

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif // WITH_EDITOR

bool UMaterialInterface::GetVectorParameterValue(FName ParameterName, FLinearColor& OutValue) const
{
	return false;
}

bool UMaterialInterface::GetScalarParameterValue(FName ParameterName, float& OutValue) const
{
	return false;
}

bool UMaterialInterface::GetScalarCurveParameterValue(FName ParameterName, FInterpCurveFloat& OutValue) const
{
	return false;
}

bool UMaterialInterface::GetVectorCurveParameterValue(FName ParameterName, FInterpCurveVector& OutValue) const
{
	return false;
}

bool UMaterialInterface::GetLinearColorParameterValue(FName ParameterName, FLinearColor& OutValue) const
{
	return false;
}

bool UMaterialInterface::GetLinearColorCurveParameterValue(FName ParameterName, FInterpCurveLinearColor& OutValue) const
{
	return false;
}

bool UMaterialInterface::GetTextureParameterValue(FName ParameterName, UTexture*& OutValue) const
{
	return false;
}

bool UMaterialInterface::GetFontParameterValue(FName ParameterName,class UFont*& OutFontValue,int32& OutFontPage) const
{
	return false;
}

bool UMaterialInterface::GetRefractionSettings(float& OutBiasValue) const
{
	return false;
}

bool UMaterialInterface::GetParameterDesc(FName ParamaterName,FString& OutDesc) const
{
	return false;
}
bool UMaterialInterface::GetGroupName(FName ParamaterName,FName& OutDesc) const
{
	return false;
}

UMaterial* UMaterialInterface::GetBaseMaterial()
{
	return GetMaterial();
}

bool DoesMaterialUseTexture(const UMaterialInterface* Material,const UTexture* CheckTexture)
{
	//Do not care if we're running dedicated server
	if (FPlatformProperties::IsServerOnly())
	{
		return false;
	}

	TArray<UTexture*> Textures;
	Material->GetUsedTextures(Textures, EMaterialQualityLevel::Num, true);
	for (int32 i = 0; i < Textures.Num(); i++)
	{
		if (Textures[i] == CheckTexture)
		{
			return true;
		}
	}
	return false;
}

float UMaterialInterface::GetOpacityMaskClipValue()const
{
	if( IsInGameThread() )
	{
		return GetOpacityMaskClipValue_Internal();
	}

	//We're on the render thread so get it from the proxy.
	return GetRenderProxy(0)->GetMaterial(GRHIFeatureLevel)->GetOpacityMaskClipValue();
}

EBlendMode UMaterialInterface::GetBlendMode()const
{
	if( IsInGameThread() )
	{
		return GetBlendMode_Internal();
	}

	//We're on the render thread so get it from the proxy.
	return GetRenderProxy(0)->GetMaterial(GRHIFeatureLevel)->GetBlendMode();
}

bool UMaterialInterface::IsTwoSided()const
{
	if( IsInGameThread() )
	{
		return IsTwoSided_Internal();
	}

	//We're on the render thread so get it from the proxy.
	return GetRenderProxy(0)->GetMaterial(GRHIFeatureLevel)->IsTwoSided();
}

EMaterialLightingModel UMaterialInterface::GetLightingModel()const
{
	if( IsInGameThread() )
	{
		return GetLightingModel_Internal();
	}

	//We're on the render thread so get it from the proxy.
	return GetRenderProxy(0)->GetMaterial(GRHIFeatureLevel)->GetLightingModel();
}

float UMaterialInterface::GetOpacityMaskClipValue_Internal()const
{
	return 0.0f;
}

EBlendMode UMaterialInterface::GetBlendMode_Internal()const
{
	return BLEND_Opaque;
}

bool UMaterialInterface::IsTwoSided_Internal()const
{
	return false;
}

EMaterialLightingModel UMaterialInterface::GetLightingModel_Internal()const
{
	return MLM_DefaultLit;
}

void UMaterialInterface::SetFeatureLevelToCompile(ERHIFeatureLevel::Type FeatureLevel, bool bShouldCompile)
{
	uint32 FeatureLevelBit = (1 << FeatureLevel);
	if (bShouldCompile)
	{
		FeatureLevelsToForceCompile |= FeatureLevelBit;
	}
	else
	{
		FeatureLevelsToForceCompile &= (~FeatureLevelBit);
	}
}

uint32 UMaterialInterface::GetFeatureLevelsToCompileForRendering() const
{
	return FeatureLevelsToForceCompile | (1 << GRHIFeatureLevel);
}
