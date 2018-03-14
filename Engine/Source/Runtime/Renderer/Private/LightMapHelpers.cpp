// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "LightMapHelpers.h"
#include "LightMapRendering.h"

FUniformBufferRHIRef LightMapHelpers::CreateDummyPrecomputedLightingUniformBuffer(EUniformBufferUsage BufferUsage, ERHIFeatureLevel::Type FeatureLevel, const FLightCacheInterface* LCI /*= nullptr */)
{
	return CreatePrecomputedLightingUniformBuffer(BufferUsage, FeatureLevel, nullptr, nullptr, FVector(0, 0, 0), 0, nullptr, LCI);
}
