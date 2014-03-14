// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	RendererPrivate.h: Renderer interface private definitions.
=============================================================================*/

#ifndef __RendererPrivate_h__
#define __RendererPrivate_h__

#include "EnginePrivate.h"
#include "ShaderCore.h"
#include "RHI.h"
#include "ScenePrivate.h"

DECLARE_LOG_CATEGORY_EXTERN(LogRenderer, Log, All);

/** The renderer module implementation. */
class FRendererModule : public IRendererModule
{
public:
	virtual bool SupportsDynamicReloading() OVERRIDE { return true; }

	virtual void BeginRenderingViewFamily(FCanvas* Canvas,const FSceneViewFamily* ViewFamily) OVERRIDE;
	virtual FSceneInterface* AllocateScene(UWorld* World, bool bInRequiresHitProxies) OVERRIDE;
	virtual void RemoveScene(FSceneInterface* Scene) OVERRIDE;
	virtual void UpdateStaticDrawListsForMaterials(const TArray<const FMaterial*>& Materials) OVERRIDE;
	virtual FSceneViewStateInterface* AllocateViewState() OVERRIDE;
	virtual uint32 GetNumDynamicLightsAffectingPrimitive(const FPrimitiveSceneInfo* PrimitiveSceneInfo,const FLightCacheInterface* LCI) OVERRIDE;
	virtual void ReallocateSceneRenderTargets() OVERRIDE;
	virtual void SceneRenderTargetsSetBufferSize(uint32 SizeX, uint32 SizeY) OVERRIDE;
	virtual void DrawTileMesh(const FSceneView& View, const FMeshBatch& Mesh, bool bIsHitTesting, const FHitProxyId& HitProxyId) OVERRIDE;
	virtual void RenderTargetPoolFindFreeElement(const FPooledRenderTargetDesc& Desc, TRefCountPtr<IPooledRenderTarget> &Out, const TCHAR* InDebugName) OVERRIDE;
	virtual void TickRenderTargetPool() OVERRIDE;
	virtual void DebugLogOnCrash() OVERRIDE;
	virtual void GPUBenchmark(FSynthBenchmarkResults& InOut, uint32 WorkScale, bool bDebugOut) OVERRIDE;
	virtual void QueryVisualizeTexture(FQueryVisualizeTexureInfo& Out) OVERRIDE;
	virtual void ExecVisualizeTextureCmd(const FString& Cmd) OVERRIDE;
	virtual void UpdateMapNeedsLightingFullyRebuiltState(UWorld* World) OVERRIDE;

	virtual const TSet<FSceneInterface*>& GetAllocatedScenes() OVERRIDE
	{
		return AllocatedScenes;
	}

private:
	TSet<FSceneInterface*> AllocatedScenes;
};

#endif
