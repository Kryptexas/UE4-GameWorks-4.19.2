// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "LightMapHelpers.h"
#include "Components.h"
#include "LocalVertexFactory.h"
#include "SceneManagement.h"

/** Simple implementation for light cache to simulated lightmap behaviour (used for accessing prebaked ambient occlusion values) */
class FMeshRenderInfo : public FLightCacheInterface
{
public:
	FMeshRenderInfo(const FLightMap* InLightMap, const FShadowMap* InShadowMap, FUniformBufferRHIRef Buffer)
		: FLightCacheInterface(InLightMap, InShadowMap)
	{
		SetPrecomputedLightingBuffer(Buffer);
	}

	virtual FLightInteraction GetInteraction(const class FLightSceneProxy* LightSceneProxy) const override
	{
		return LIT_CachedLightMap;
	}
};