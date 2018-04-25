// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AreaLightActor.cpp: Area light actor class implementation.
=============================================================================*/

#include "Engine/AreaLightActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"


#define LOCTEXT_NAMESPACE "AreaLightActor"

AAreaLightActor::AAreaLightActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, LightColor(FLinearColor::White)
	, LightIntensityDiffuse(1.0f)
	, LightIntensitySpecular(1.0f)
	, AttenuationRadius(1000.f)
	, TransitionLength(250.f)
	, bEnableLight(true)
	, LightTexture(nullptr)
{
#if WITH_GFSDK_VXGI
	VXGI::AreaLight Default;
	Quality = Default.quality;
	DirectionalSamplingRate = Default.directionalSamplingRate;
	MaxProjectedArea = Default.maxProjectedArea;
	LightSurfaceOffset = Default.lightSurfaceOffset;
	bEnableShadows = Default.enableOcclusion;
	bEnableScreenSpaceShadows = Default.enableScreenSpaceOcclusion;
	ScreenSpaceShadowQuality = Default.screenSpaceOcclusionQuality;
	TemporalWeight = Default.temporalReprojectionWeight;
	TemporalDetailReconstruction = Default.temporalReprojectionDetailReconstruction;
	bEnableNeighborhoodColorClamping = Default.enableNeighborhoodClamping;
	NeighborhoodClampingWidth = Default.neighborhoodClampingWidth;
	bTexturedShadows = Default.texturedShadows;
	bHighQualityTextureFilter = Default.highQualityTextureFilter;
#else
	// Some values to initialize the fields. They don't really matter.
	Quality = 0.5f;
	DirectionalSamplingRate = 1.f;
	MaxProjectedArea = 0.75f;
	LightSurfaceOffset = 0.5f;
	bEnableShadows = true;
	bEnableScreenSpaceShadows = false;
	ScreenSpaceShadowQuality = 0.5f;
	TemporalWeight = 0.5f;
	TemporalDetailReconstruction = 0.5f;
	bEnableNeighborhoodColorClamping = false;
	NeighborhoodClampingWidth = 1.0f;
	bTexturedShadows = true;
	bHighQualityTextureFilter = true;
#endif
}

#undef LOCTEXT_NAMESPACE

