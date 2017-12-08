// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

/*=============================================================================
FlexFluidSurfaceSceneProxy.h:
=============================================================================*/

/*=============================================================================
FFlexFluidSurfaceTextures, Encapsulates the textures used for scene rendering. TODO, move to some better place
=============================================================================*/
struct FFlexFluidSurfaceTextures
{
	FFlexFluidSurfaceTextures() : BufferSize(0, 0)	{}

	~FFlexFluidSurfaceTextures() {}

	// Current buffer size
	FIntPoint BufferSize;

	// Intermediate results
	TRefCountPtr<IPooledRenderTarget> Depth;
	TRefCountPtr<IPooledRenderTarget> DepthStencil;
	TRefCountPtr<IPooledRenderTarget> Color;
	TRefCountPtr<IPooledRenderTarget> Thickness;
	TRefCountPtr<IPooledRenderTarget> DownSampledSceneDepth;
	TRefCountPtr<IPooledRenderTarget> SmoothDepth;
	TRefCountPtr<IPooledRenderTarget> UpSampledDepth;
};

/*=============================================================================
FFlexFluidSurfaceSceneProxy
=============================================================================*/

struct FFlexFluidSurfaceProperties
{
	~FFlexFluidSurfaceProperties();

	float SmoothingRadius;
	int32 MaxRadialSamples;
	float DepthEdgeFalloff;
	float ThicknessParticleScale;
	float DepthParticleScale;
	float ShadowParticleScale;
	float TexResScale;
	bool CastParticleShadows;
	bool ReceivePrecachedShadows;
	UMaterialInterface* Material;

	void* ParticleBuffer;
	void* ParticleAnisotropyBuffer;
	int32 NumParticles;
	int32 MaxParticles;
};

class FFlexFluidSurfaceSceneProxy : public FPrimitiveSceneProxy
{
public:
	FFlexFluidSurfaceSceneProxy(class UFlexFluidSurfaceComponent* Component);

	virtual ~FFlexFluidSurfaceSceneProxy();

	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;

	//this can't be used since maximal number of particles can't be passed 
	//to proxy before creation time (see UWorld::SpawnActor)
	//virtual void CreateRenderThreadResources() override;

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const;

	virtual uint32 GetMemoryFootprint(void) const { return(sizeof(*this) + FPrimitiveSceneProxy::GetAllocatedSize()); }

	void SetDynamicData_RenderThread(FFlexFluidSurfaceProperties* SurfaceProperties);

	virtual FFlexFluidSurfaceTextures& GetTextures() const;
	virtual FMeshBatch& GetMeshBatch() const;
	virtual FMeshBatch& GetParticleDepthMeshBatch() const;
	virtual FMeshBatch& GetParticleThicknessMeshBatch() const;

public:

	FFlexFluidSurfaceProperties* SurfaceProperties;

	// resources managed by game thread
	UMaterialInterface* SurfaceMaterial;

	// resources managed in render thread
	struct FFlexFluidSurfaceResources* Resources;

	static TSet<const FPrimitiveSceneProxy*> InstanceSet;
};

/*=============================================================================
FFlexFluidSurfaceShadowSceneProxy
=============================================================================*/

class FFlexFluidSurfaceShadowSceneProxy : public FPrimitiveSceneProxy
{
public:
	FFlexFluidSurfaceShadowSceneProxy(class UFlexFluidSurfaceShadowComponent* Component);
	virtual ~FFlexFluidSurfaceShadowSceneProxy();

	void SetDynamicData_RenderThread(FFlexFluidSurfaceProperties* NewSurfaceProperties);

	/*This is used for the particle based shadowing*/
	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const;

	virtual uint32 GetMemoryFootprint(void) const { return(sizeof(*this) + FPrimitiveSceneProxy::GetAllocatedSize()); }

public:
	UMaterialInterface* SurfaceMaterial;
	uint32 NumParticles;
	FVertexBuffer ParticleVertexBuffer;
	bool CastParticleShadows;
	class FFlexFluidSurfaceParticleVertexFactory* ParticleShadowVertexFactory;
};

/*=============================================================================
Helper
=============================================================================*/

inline const FTexture2DRHIRef& GetSurface(TRefCountPtr<IPooledRenderTarget>& RenderTarget)
{
	return (const FTexture2DRHIRef&)RenderTarget->GetRenderTargetItem().TargetableTexture;
}

inline const FTexture2DRHIRef& GetTexture(TRefCountPtr<IPooledRenderTarget>& RenderTarget)
{
	return (const FTexture2DRHIRef&)RenderTarget->GetRenderTargetItem().ShaderResourceTexture;
}
