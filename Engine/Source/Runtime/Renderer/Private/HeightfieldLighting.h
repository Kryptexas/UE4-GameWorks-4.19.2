// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	HeightfieldLighting.h
=============================================================================*/

#pragma once

class FHeightfieldLightingAtlas : public FRenderResource
{
public:

	TRefCountPtr<IPooledRenderTarget> Height;
	TRefCountPtr<IPooledRenderTarget> Normal;
	TRefCountPtr<IPooledRenderTarget> DirectionalLightShadowing;
	TRefCountPtr<IPooledRenderTarget> Lighting;

	FHeightfieldLightingAtlas() :
		AtlasSize(FIntPoint(0, 0))
	{}

	virtual void InitDynamicRHI();
	virtual void ReleaseDynamicRHI();

	void InitializeForSize(FIntPoint InAtlasSize);

	FIntPoint GetAtlasSize() const { return AtlasSize; }

private:

	FIntPoint AtlasSize;
};

class FHeightfieldDescription
{
public:
	FIntRect Rect;
	int32 DownsampleFactor;
	FIntRect DownsampledRect;

	TMap<UTexture2D*, TArray<FHeightfieldComponentDescription>> ComponentDescriptions;

	FHeightfieldDescription() :
		Rect(FIntRect(0, 0, 0, 0)),
		DownsampleFactor(1),
		DownsampledRect(FIntRect(0, 0, 0, 0))
	{}
};

class FHeightfieldLightingViewInfo
{
public:

	FHeightfieldLightingViewInfo()
	{}

	void SetupVisibleHeightfields(const FViewInfo& View, FRHICommandListImmediate& RHICmdList);

	void ClearShadowing(const FViewInfo& View, FRHICommandListImmediate& RHICmdList, const FLightSceneInfo& LightSceneInfo) const;

	void ComputeShadowMapShadowing(const FViewInfo& View, FRHICommandListImmediate& RHICmdList, const FProjectedShadowInfo* ProjectedShadowInfo) const;

	void ComputeRayTracedShadowing(
		const FViewInfo& View, 
		FRHICommandListImmediate& RHICmdList, 
		const FProjectedShadowInfo* ProjectedShadowInfo, 
		FLightTileIntersectionResources* TileIntersectionResources,
		class FDistanceFieldObjectBufferResource& CulledObjectBuffers) const;

	void ComputeLighting(const FViewInfo& View, FRHICommandListImmediate& RHICmdList, const FLightSceneInfo& LightSceneInfo) const;

	void ComputeOcclusionForSamples(
		const FViewInfo& View, 
		FRHICommandListImmediate& RHICmdList, 
		const class FTemporaryIrradianceCacheResources& TemporaryIrradianceCacheResources,
		int32 DepthLevel,
		const class FDistanceFieldAOParameters& Parameters) const;

	void ComputeIrradianceForSamples(
		const FViewInfo& View, 
		FRHICommandListImmediate& RHICmdList, 
		const class FTemporaryIrradianceCacheResources& TemporaryIrradianceCacheResources,
		int32 DepthLevel,
		const class FDistanceFieldAOParameters& Parameters) const;

private:

	FHeightfieldDescription Heightfield;
};

